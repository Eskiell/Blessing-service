#include "ezcheats/overlay/overlay_manager.hpp"

#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <machine/param.h>
#include <machine/reg.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>

#include <ps5/kernel.h>
#include <ps5/nid.h>

#include "ezcheats/assets/embedded_overlay.hpp"

namespace ezcheats::overlay {
namespace {

constexpr uint64_t kPtraceAuthId = 0x4800000000010003ULL;
constexpr const char* kShellUiProcess = "SceShellUI";
constexpr const char* kReadyPath = "/system_tmp/blessing/overlay-ready";
constexpr size_t kMaxOverlaySize = 4 * 1024 * 1024;
constexpr size_t kMaxRemoteSteps = 100000;
constexpr int kReadyAttempts = 20;
constexpr useconds_t kReadyDelayUs = 250 * 1000;

#ifndef IPV6_2292PKTOPTIONS
#define IPV6_2292PKTOPTIONS 25
#endif

extern "C" int sceKernelGetProcessName(int pid, char* name);

extern "C" {
extern const uint8_t blessing_overlay_stager_start[];
extern const uint8_t blessing_overlay_stager_end[];
}

// rdi points to RemoteStart. The bootstrap creates the overlay thread and
// traps so the injector can restore the interrupted ShellUI register frame.
__asm__(
    ".text\n"
    ".intel_syntax noprefix\n"
    ".global blessing_overlay_stager_start\n"
    ".type blessing_overlay_stager_start, @function\n"
    "blessing_overlay_stager_start:\n"
    "mov r11, rdi\n"
    "mov rax, [r11]\n"
    "lea rdi, [r11 + 32]\n"
    "xor esi, esi\n"
    "mov rdx, [r11 + 8]\n"
    "mov rcx, [r11 + 16]\n"
    "sub rsp, 8\n"
    "call rax\n"
    "add rsp, 8\n"
    "int3\n"
    ".global blessing_overlay_stager_end\n"
    "blessing_overlay_stager_end:\n"
    ".att_syntax prefix\n");

bool checked_add(size_t left, size_t right, size_t& output) noexcept {
  if (right > SIZE_MAX - left) return false;
  output = left + right;
  return true;
}

bool checked_multiply(size_t left, size_t right, size_t& output) noexcept {
  if (left != 0 && right > SIZE_MAX / left) return false;
  output = left * right;
  return true;
}

bool range_valid(size_t offset, size_t length, size_t total) noexcept {
  size_t end = 0;
  return checked_add(offset, length, end) && end <= total;
}

bool table_valid(size_t offset, size_t count, size_t element_size,
                 size_t total) noexcept {
  size_t length = 0;
  return checked_multiply(count, element_size, length) &&
         range_valid(offset, length, total);
}

bool valid_overlay_elf(const uint8_t* elf, size_t size) noexcept {
  if (elf == nullptr || size < sizeof(Elf64_Ehdr) || size > kMaxOverlaySize) {
    return false;
  }
  const auto* header = reinterpret_cast<const Elf64_Ehdr*>(elf);
  if (memcmp(header->e_ident, ELFMAG, SELFMAG) != 0 ||
      header->e_ident[EI_CLASS] != ELFCLASS64 ||
      header->e_ident[EI_DATA] != ELFDATA2LSB ||
      header->e_machine != EM_X86_64 || header->e_type != ET_DYN ||
      header->e_phentsize != sizeof(Elf64_Phdr) || header->e_phnum == 0 ||
      !table_valid(header->e_phoff, header->e_phnum, sizeof(Elf64_Phdr),
                   size)) {
    return false;
  }
  if (header->e_shnum != 0 &&
      (header->e_shentsize != sizeof(Elf64_Shdr) ||
       !table_valid(header->e_shoff, header->e_shnum, sizeof(Elf64_Shdr),
                    size))) {
    return false;
  }
  const auto* programs = reinterpret_cast<const Elf64_Phdr*>(
      elf + static_cast<size_t>(header->e_phoff));
  bool has_load = false;
  for (size_t index = 0; index < header->e_phnum; ++index) {
    if (programs[index].p_type != PT_LOAD) continue;
    has_load = true;
    if (programs[index].p_filesz > programs[index].p_memsz ||
        !range_valid(programs[index].p_offset, programs[index].p_filesz,
                     size)) {
      return false;
    }
  }
  return has_load;
}

class AuthIdGuard final {
 public:
  AuthIdGuard() noexcept
      : original_(kernel_get_ucred_authid(getpid())),
        active_(original_ != 0 &&
                kernel_set_ucred_authid(getpid(), kPtraceAuthId) == 0) {}

  ~AuthIdGuard() {
    if (active_ && kernel_set_ucred_authid(getpid(), original_) != 0) {
      printf("Blessing overlay: failed to restore injector authid\n");
    }
  }

  bool active() const noexcept { return active_; }

 private:
  uint64_t original_ = 0;
  bool active_ = false;
};

class RemoteProcess final {
 public:
  explicit RemoteProcess(pid_t pid) noexcept : pid_(pid) {}
  ~RemoteProcess() { detach(); }

  bool attach() noexcept {
    if (pid_ <= 1 || attached_) return false;
    if (ptrace(PT_ATTACH, pid_, nullptr, 0) != 0) return false;
    attached_ = true;
    int status = 0;
    return waitpid(pid_, &status, WUNTRACED) == pid_ && WIFSTOPPED(status);
  }

  bool detach() noexcept {
    if (!attached_) return true;
    const bool ok = ptrace(PT_DETACH, pid_, nullptr, 0) == 0;
    if (ok) attached_ = false;
    return ok;
  }

  intptr_t resolve(const char* symbol) const noexcept {
    char nid[12]{};
    nid_encode(symbol, nid);
    intptr_t address = kernel_dynlib_resolve(pid_, 1, nid);
    if (address == 0) address = kernel_dynlib_resolve(pid_, 0x2001, nid);
    return address;
  }

  intptr_t resolve_nid(const char* nid) const noexcept {
    intptr_t address = kernel_dynlib_resolve(pid_, 1, nid);
    if (address == 0) address = kernel_dynlib_resolve(pid_, 0x2001, nid);
    return address;
  }

  bool copy_in(const void* source, intptr_t destination,
               size_t size) const noexcept {
    struct ptrace_io_desc descriptor {
      PIOD_WRITE_D, reinterpret_cast<void*>(destination),
          const_cast<void*>(source), size
    };
    return ptrace(PT_IO, pid_, reinterpret_cast<caddr_t>(&descriptor), 0) == 0;
  }

  bool copy_out(intptr_t source, void* destination, size_t size) const noexcept {
    struct ptrace_io_desc descriptor {
      PIOD_READ_D, reinterpret_cast<void*>(source), destination, size
    };
    return ptrace(PT_IO, pid_, reinterpret_cast<caddr_t>(&descriptor), 0) == 0;
  }

  bool call(intptr_t address, const uint64_t arguments[6],
            int64_t& result) noexcept {
    if (address == 0) return false;
    struct reg backup {};
    if (ptrace(PT_GETREGS, pid_, reinterpret_cast<caddr_t>(&backup), 0) != 0) {
      return false;
    }
    struct reg invocation = backup;
    const uintptr_t entry_stack =
        (static_cast<uintptr_t>(backup.r_rsp) & ~uintptr_t{0xf}) - 0x108;
    invocation.r_rip = static_cast<uint64_t>(address);
    invocation.r_rsp = entry_stack;
    invocation.r_rdi = arguments[0];
    invocation.r_rsi = arguments[1];
    invocation.r_rdx = arguments[2];
    invocation.r_rcx = arguments[3];
    invocation.r_r8 = arguments[4];
    invocation.r_r9 = arguments[5];
    if (ptrace(PT_SETREGS, pid_, reinterpret_cast<caddr_t>(&invocation), 0) !=
        0) {
      return false;
    }
    bool completed = false;
    for (size_t step = 0; step < kMaxRemoteSteps; ++step) {
      int status = 0;
      if (ptrace(PT_STEP, pid_, reinterpret_cast<caddr_t>(1), 0) != 0 ||
          waitpid(pid_, &status, 0) != pid_ || !WIFSTOPPED(status) ||
          ptrace(PT_GETREGS, pid_, reinterpret_cast<caddr_t>(&invocation), 0) !=
              0) {
        break;
      }
      if (static_cast<uintptr_t>(invocation.r_rsp) > entry_stack) {
        completed = true;
        break;
      }
    }
    result = static_cast<int64_t>(invocation.r_rax);
    const bool restored =
        ptrace(PT_SETREGS, pid_, reinterpret_cast<caddr_t>(&backup), 0) == 0;
    return completed && restored;
  }

  bool remote_syscall(int number, const uint64_t arguments[6],
                      int64_t& result) noexcept {
    const intptr_t getpid_address = resolve_nid("HoLVWNanBBc");
    if (getpid_address == 0) return false;
    struct reg backup {};
    if (ptrace(PT_GETREGS, pid_, reinterpret_cast<caddr_t>(&backup), 0) != 0) {
      return false;
    }
    struct reg invocation = backup;
    const uintptr_t entry_stack =
        (static_cast<uintptr_t>(backup.r_rsp) & ~uintptr_t{0xf}) - 0x108;
    invocation.r_rip = static_cast<uint64_t>(getpid_address + 0xa);
    invocation.r_rsp = entry_stack;
    invocation.r_rax = static_cast<uint64_t>(number);
    invocation.r_rdi = arguments[0];
    invocation.r_rsi = arguments[1];
    invocation.r_rdx = arguments[2];
    invocation.r_r10 = arguments[3];
    invocation.r_r8 = arguments[4];
    invocation.r_r9 = arguments[5];
    if (ptrace(PT_SETREGS, pid_, reinterpret_cast<caddr_t>(&invocation), 0) !=
        0) {
      return false;
    }
    bool completed = false;
    for (size_t step = 0; step < kMaxRemoteSteps; ++step) {
      int status = 0;
      if (ptrace(PT_STEP, pid_, reinterpret_cast<caddr_t>(1), 0) != 0 ||
          waitpid(pid_, &status, 0) != pid_ || !WIFSTOPPED(status) ||
          ptrace(PT_GETREGS, pid_, reinterpret_cast<caddr_t>(&invocation), 0) !=
              0) {
        break;
      }
      if (static_cast<uintptr_t>(invocation.r_rsp) > entry_stack) {
        completed = true;
        break;
      }
    }
    result = static_cast<int64_t>(invocation.r_rax);
    const bool restored =
        ptrace(PT_SETREGS, pid_, reinterpret_cast<caddr_t>(&backup), 0) == 0;
    return completed && restored;
  }

  intptr_t map(size_t size, int protection) noexcept {
    const uint64_t arguments[6] = {0, size, static_cast<uint64_t>(protection),
                                   MAP_ANONYMOUS | MAP_PRIVATE,
                                   static_cast<uint64_t>(-1), 0};
    int64_t result = -1;
    return remote_syscall(SYS_mmap, arguments, result)
               ? static_cast<intptr_t>(result)
               : -1;
  }

  bool protect(intptr_t address, size_t size, int protection) noexcept {
    const uint64_t arguments[6] = {
        static_cast<uint64_t>(address), size,
        static_cast<uint64_t>(protection), 0, 0, 0};
    int64_t result = -1;
    return remote_syscall(SYS_mprotect, arguments, result) && result == 0;
  }

  bool sync(intptr_t address, size_t size) noexcept {
    const uint64_t arguments[6] = {static_cast<uint64_t>(address), size,
                                   MS_SYNC, 0, 0, 0};
    int64_t result = -1;
    return remote_syscall(SYS_msync, arguments, result) && result == 0;
  }

  bool start_thread(intptr_t entry, intptr_t arguments) noexcept {
    struct RemoteStart {
      intptr_t pthread_create;
      intptr_t entry;
      intptr_t arguments;
      uint64_t reserved;
      uint64_t thread;
    };
    static_assert(offsetof(RemoteStart, thread) == 32);

    const size_t stager_size = static_cast<size_t>(
        blessing_overlay_stager_end - blessing_overlay_stager_start);
    if (stager_size == 0 || stager_size > 256) return false;
    const intptr_t stager = map(PAGE_SIZE, PROT_READ | PROT_WRITE);
    const intptr_t parameters = map(PAGE_SIZE, PROT_READ | PROT_WRITE);
    if (stager <= 0 || parameters <= 0 ||
        !copy_in(blessing_overlay_stager_start, stager, stager_size) ||
        kernel_mprotect(pid_, stager, PAGE_SIZE,
                        PROT_READ | PROT_EXEC) != 0) {
      return false;
    }
    const RemoteStart start{resolve("pthread_create"), entry, arguments, 0, 0};
    if (start.pthread_create == 0 ||
        !copy_in(&start, parameters, sizeof(start))) {
      return false;
    }

    struct reg backup {};
    if (ptrace(PT_GETREGS, pid_, reinterpret_cast<caddr_t>(&backup), 0) != 0) {
      return false;
    }
    struct reg invocation = backup;
    invocation.r_rip = static_cast<uint64_t>(stager);
    invocation.r_rsp =
        (static_cast<uintptr_t>(backup.r_rsp) & ~uintptr_t{0xf}) - 0x108;
    invocation.r_rdi = static_cast<uint64_t>(parameters);
    if (ptrace(PT_SETREGS, pid_, reinterpret_cast<caddr_t>(&invocation), 0) !=
            0 ||
        ptrace(PT_CONTINUE, pid_, reinterpret_cast<caddr_t>(1), 0) != 0) {
      (void)ptrace(PT_SETREGS, pid_, reinterpret_cast<caddr_t>(&backup), 0);
      return false;
    }
    int status = 0;
    const bool trapped = waitpid(pid_, &status, 0) == pid_ &&
                         WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP;
    const bool restored =
        ptrace(PT_SETREGS, pid_, reinterpret_cast<caddr_t>(&backup), 0) == 0;
    return trapped && restored;
  }

  pid_t pid() const noexcept { return pid_; }

 private:
  pid_t pid_ = -1;
  bool attached_ = false;
};

int protection_for(uint32_t flags) noexcept {
  return ((flags & PF_R) != 0 ? PROT_READ : 0) |
         ((flags & PF_W) != 0 ? PROT_WRITE : 0) |
         ((flags & PF_X) != 0 ? PROT_EXEC : 0);
}

intptr_t load_elf(RemoteProcess& remote, const uint8_t* elf,
                  size_t elf_size) noexcept {
  if (!valid_overlay_elf(elf, elf_size)) return 0;
  const auto* header = reinterpret_cast<const Elf64_Ehdr*>(elf);
  const auto* programs = reinterpret_cast<const Elf64_Phdr*>(
      elf + static_cast<size_t>(header->e_phoff));

  size_t maximum = 0;
  for (size_t index = 0; index < header->e_phnum; ++index) {
    if (programs[index].p_type != PT_LOAD) continue;
    size_t end = 0;
    if (!checked_add(programs[index].p_vaddr, programs[index].p_memsz, end)) {
      return 0;
    }
    if (end > maximum) maximum = end;
  }
  if (maximum == 0 || maximum > SIZE_MAX - (PAGE_SIZE - 1)) return 0;
  const size_t mapping_size = (maximum + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
  auto* mirror = static_cast<uint8_t*>(calloc(mapping_size, 1));
  if (mirror == nullptr) return 0;
  const intptr_t base = remote.map(mapping_size, PROT_READ | PROT_WRITE);
  if (base <= 0) {
    free(mirror);
    return 0;
  }

  bool valid = true;
  for (size_t index = 0; index < header->e_phnum && valid; ++index) {
    if (programs[index].p_type != PT_LOAD || programs[index].p_filesz == 0) {
      continue;
    }
    valid = range_valid(programs[index].p_vaddr,
                        programs[index].p_filesz, mapping_size);
    if (valid) {
      memcpy(mirror + programs[index].p_vaddr,
             elf + programs[index].p_offset, programs[index].p_filesz);
    }
  }

  if (valid && header->e_shnum != 0) {
    const auto* sections = reinterpret_cast<const Elf64_Shdr*>(
        elf + static_cast<size_t>(header->e_shoff));
    for (size_t section = 0; section < header->e_shnum && valid; ++section) {
      if (sections[section].sh_type != SHT_RELA) continue;
      if (sections[section].sh_entsize != sizeof(Elf64_Rela) ||
          !range_valid(sections[section].sh_offset, sections[section].sh_size,
                       elf_size)) {
        valid = false;
        break;
      }
      const auto* relocations = reinterpret_cast<const Elf64_Rela*>(
          elf + sections[section].sh_offset);
      const size_t count = sections[section].sh_size / sizeof(Elf64_Rela);
      for (size_t relocation = 0; relocation < count; ++relocation) {
        // The PS5 CRT resolves imported GLOB_DAT entries from payload_args.
        // Only base-relative entries belong to this mapping phase.
        if (ELF64_R_TYPE(relocations[relocation].r_info) !=
            R_X86_64_RELATIVE) {
          continue;
        }
        if (!range_valid(relocations[relocation].r_offset, sizeof(intptr_t),
                         mapping_size)) {
          valid = false;
          break;
        }
        const intptr_t value = base + relocations[relocation].r_addend;
        memcpy(mirror + relocations[relocation].r_offset, &value,
               sizeof(value));
      }
    }
  }

  valid = valid && remote.copy_in(mirror, base, mapping_size);
  free(mirror);
  for (size_t index = 0; index < header->e_phnum && valid; ++index) {
    if (programs[index].p_type != PT_LOAD || programs[index].p_memsz == 0) {
      continue;
    }
    const intptr_t address = base + programs[index].p_vaddr;
    const size_t size =
        (programs[index].p_memsz + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    const int protection = protection_for(programs[index].p_flags);
    valid = (programs[index].p_flags & PF_X) != 0
                ? kernel_mprotect(remote.pid(), address, size, protection) == 0
                : remote.protect(address, size, protection);
  }
  if (!valid || !remote.sync(base, mapping_size)) return 0;
  return base + header->e_entry;
}

intptr_t create_payload_arguments(RemoteProcess& remote) noexcept {
  const intptr_t buffer = remote.map(PAGE_SIZE, PROT_READ | PROT_WRITE);
  if (buffer <= 0) return 0;

  auto syscall_remote = [&remote](int number, const uint64_t args[6],
                                  int64_t& result) {
    return remote.remote_syscall(number, args, result);
  };
  const uint64_t socket_args[6] = {AF_INET6, SOCK_DGRAM, IPPROTO_UDP, 0, 0, 0};
  int64_t master = -1;
  int64_t victim = -1;
  if (!syscall_remote(SYS_socket, socket_args, master) || master < 0 ||
      !syscall_remote(SYS_socket, socket_args, victim) || victim < 0) {
    return 0;
  }

  const int master_options[6] = {20, IPPROTO_IPV6, IPV6_TCLASS, 0, 0, 0};
  const int victim_options[5] = {0, 0, 0, 0, 0};
  if (!remote.copy_in(master_options, buffer, sizeof(master_options))) return 0;
  const uint64_t master_setopt[6] = {
      static_cast<uint64_t>(master), IPPROTO_IPV6, IPV6_2292PKTOPTIONS,
      static_cast<uint64_t>(buffer), sizeof(master_options), 0};
  int64_t result = -1;
  if (!syscall_remote(SYS_setsockopt, master_setopt, result) || result != 0 ||
      !remote.copy_in(victim_options, buffer, sizeof(victim_options))) {
    return 0;
  }
  const uint64_t victim_setopt[6] = {
      static_cast<uint64_t>(victim), IPPROTO_IPV6, IPV6_PKTINFO,
      static_cast<uint64_t>(buffer), sizeof(victim_options), 0};
  if (!syscall_remote(SYS_setsockopt, victim_setopt, result) || result != 0 ||
      kernel_overlap_sockets(remote.pid(), static_cast<int>(master),
                             static_cast<int>(victim)) != 0) {
    return 0;
  }

  const intptr_t pipe_function = remote.resolve_nid("-Jp7F+pXxNg");
  const uint64_t pipe_args[6] = {static_cast<uint64_t>(buffer), 0, 0, 0, 0, 0};
  if (!remote.call(pipe_function, pipe_args, result) || result != 0) return 0;
  int pipes[2]{};
  if (!remote.copy_out(buffer, pipes, sizeof(pipes))) return 0;

  const intptr_t arguments = buffer;
  const intptr_t rwpipe = buffer + 0x100;
  const intptr_t rwpair = buffer + 0x200;
  const intptr_t payload_output = buffer + 0x300;
  const intptr_t process_file =
      kernel_get_proc_file(remote.pid(), pipes[0]);
  const intptr_t getpid_address = remote.resolve_nid("HoLVWNanBBc");
  const intptr_t fields[6] = {getpid_address, rwpipe, rwpair, process_file,
                              KERNEL_ADDRESS_DATA_BASE, payload_output};
  const int socket_pair[2] = {static_cast<int>(master),
                              static_cast<int>(victim)};
  const int zero = 0;
  if (process_file == 0 || getpid_address == 0 ||
      !remote.copy_in(fields, arguments, sizeof(fields)) ||
      !remote.copy_in(pipes, rwpipe, sizeof(pipes)) ||
      !remote.copy_in(socket_pair, rwpair, sizeof(socket_pair)) ||
      !remote.copy_in(&zero, payload_output, sizeof(zero))) {
    return 0;
  }
  return arguments;
}

pid_t find_shellui_pid() noexcept {
  int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PROC, 0};
  size_t size = 0;
  if (sysctl(mib, 4, nullptr, &size, nullptr, 0) != 0 || size == 0) return -1;
  auto* table = static_cast<uint8_t*>(malloc(size));
  if (table == nullptr) return -1;
  if (sysctl(mib, 4, table, &size, nullptr, 0) != 0) {
    free(table);
    return -1;
  }
  pid_t found = -1;
  uint8_t* cursor = table;
  const uint8_t* end = table + size;
  while (cursor < end) {
    auto* process = reinterpret_cast<struct kinfo_proc*>(cursor);
    if (process->ki_structsize <= 0 ||
        static_cast<size_t>(end - cursor) <
            static_cast<size_t>(process->ki_structsize)) {
      break;
    }
    cursor += process->ki_structsize;
    char name[64]{};
    if (sceKernelGetProcessName(process->ki_pid, name) == 0 &&
        strcmp(name, kShellUiProcess) == 0) {
      found = process->ki_pid;
      break;
    }
  }
  free(table);
  return found;
}

bool ready_for(pid_t pid) noexcept {
  const int file = open(kReadyPath, O_RDONLY);
  if (file < 0) return false;
  char value[32]{};
  const ssize_t length = read(file, value, sizeof(value) - 1);
  close(file);
  if (length <= 0) return false;
  char* end = nullptr;
  errno = 0;
  const long recorded = strtol(value, &end, 10);
  return errno == 0 && end != value && *end == '\0' && recorded == pid;
}

bool inject(pid_t pid) noexcept {
  const uint8_t* elf = assets::embedded_overlay_data();
  const size_t elf_size = assets::embedded_overlay_size();
  if (!valid_overlay_elf(elf, elf_size)) {
    printf("Blessing overlay: embedded ELF validation failed\n");
    return false;
  }
  AuthIdGuard authid;
  if (!authid.active()) {
    printf("Blessing overlay: ptrace authid unavailable\n");
    return false;
  }
  RemoteProcess remote{pid};
  if (!remote.attach()) {
    printf("Blessing overlay: attach failed pid=%d errno=%d\n", pid, errno);
    return false;
  }
  const intptr_t entry = load_elf(remote, elf, elf_size);
  const intptr_t arguments =
      entry > 0 ? create_payload_arguments(remote) : 0;
  const bool started = entry > 0 && arguments > 0 &&
                       remote.start_thread(entry, arguments);
  const bool detached = remote.detach();
  if (!started || !detached) {
    printf("Blessing overlay: injection failed pid=%d started=%s detached=%s\n",
           pid, started ? "yes" : "no", detached ? "yes" : "no");
    return false;
  }
  for (int attempt = 0; attempt < kReadyAttempts; ++attempt) {
    if (ready_for(pid)) return true;
    usleep(kReadyDelayUs);
  }
  printf("Blessing overlay: ready timeout pid=%d\n", pid);
  return false;
}

void* injection_worker(void*) {
  const pid_t pid = find_shellui_pid();
  if (pid <= 1) {
    printf("Blessing overlay: SceShellUI not found; tile remains available\n");
    return nullptr;
  }
  if (ready_for(pid)) {
    printf("Blessing overlay: already loaded pid=%d\n", pid);
    return nullptr;
  }
  unlink(kReadyPath);
  const bool loaded = inject(pid);
  printf("Blessing overlay: injection=%s pid=%d\n",
         loaded ? "ready" : "failed", pid);
  return nullptr;
}

}  // namespace

bool start_overlay_injection() noexcept {
  pthread_t thread{};
  if (pthread_create(&thread, nullptr, injection_worker, nullptr) != 0) {
    return false;
  }
  pthread_detach(thread);
  return true;
}

}  // namespace ezcheats::overlay
