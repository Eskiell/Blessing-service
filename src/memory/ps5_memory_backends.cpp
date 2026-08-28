#include "ezcheats/memory/memory_backend_factory.hpp"

#include <errno.h>
#include <machine/reg.h>
#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/wait.h>

#include <ps5/kernel.h>
#include <ps5/mdbg.h>

namespace ezcheats::memory {
namespace {

extern "C" int sceKernelGetProsperoSystemSwVersion(void* version);

constexpr uint64_t kPageSize = 0x4000;
constexpr uint64_t kInvalidPhysicalAddress = UINT64_MAX;

struct KernelVersion {
  uint64_t padding0;
  char version_string[0x1c];
  uint32_t version;
  uint64_t padding1;
};

class PtraceAttachment final {
 public:
  explicit PtraceAttachment(int pid) noexcept : pid_(pid) {
    if (pid_ < 0 ||
        syscall(SYS_ptrace, PT_ATTACH, pid_, nullptr, 0) == -1) {
      return;
    }
    attached_ = true;
    int status = 0;
    ready_ = waitpid(pid_, &status, WUNTRACED) == pid_ && WIFSTOPPED(status);
  }

  ~PtraceAttachment() {
    if (attached_) syscall(SYS_ptrace, PT_DETACH, pid_, nullptr, 0);
  }

  PtraceAttachment(const PtraceAttachment&) = delete;
  PtraceAttachment& operator=(const PtraceAttachment&) = delete;
  bool ready() const noexcept { return ready_; }

 private:
  int pid_;
  bool attached_ = false;
  bool ready_ = false;
};

class RemoteRegisters final {
 public:
  explicit RemoteRegisters(int pid) noexcept : pid_(pid) {
    valid_ = ptrace(PT_GETREGS, pid_, reinterpret_cast<caddr_t>(&backup_), 0) ==
             0;
  }

  ~RemoteRegisters() { restore(); }
  RemoteRegisters(const RemoteRegisters&) = delete;
  RemoteRegisters& operator=(const RemoteRegisters&) = delete;
  bool valid() const noexcept { return valid_; }

  bool invoke_syscall(int syscall_number, const uint64_t arguments[6],
                      int64_t& result) noexcept {
    if (!valid_) return false;
    const intptr_t resolved = kernel_dynlib_resolve(pid_, 1, "HoLVWNanBBc");
    const intptr_t fallback =
        resolved != 0 ? resolved
                      : kernel_dynlib_resolve(pid_, 0x2001, "HoLVWNanBBc");
    if (fallback == 0) return false;

    struct reg call = backup_;
    const uintptr_t entry_stack =
        (static_cast<uintptr_t>(backup_.r_rsp) & ~uintptr_t{0xf}) - 0x108;
    call.r_rip = static_cast<uint64_t>(fallback + 0xa);
    call.r_rsp = entry_stack;
    call.r_rax = static_cast<uint64_t>(syscall_number);
    call.r_rdi = arguments[0];
    call.r_rsi = arguments[1];
    call.r_rdx = arguments[2];
    call.r_r10 = arguments[3];
    call.r_r8 = arguments[4];
    call.r_r9 = arguments[5];
    if (ptrace(PT_SETREGS, pid_, reinterpret_cast<caddr_t>(&call), 0) != 0) {
      return false;
    }
    dirty_ = true;

    for (size_t step = 0;
         step < 100000 && static_cast<uintptr_t>(call.r_rsp) <= entry_stack;
         ++step) {
      if (ptrace(PT_STEP, pid_, reinterpret_cast<caddr_t>(1), 0) != 0 ||
          waitpid(pid_, nullptr, 0) < 0 ||
          ptrace(PT_GETREGS, pid_, reinterpret_cast<caddr_t>(&call), 0) != 0) {
        return false;
      }
    }
    if (static_cast<uintptr_t>(call.r_rsp) <= entry_stack) return false;
    result = static_cast<int64_t>(call.r_rax);
    return restore();
  }

 private:
  bool restore() noexcept {
    if (!dirty_) return valid_;
    if (ptrace(PT_SETREGS, pid_, reinterpret_cast<caddr_t>(&backup_), 0) != 0) {
      return false;
    }
    dirty_ = false;
    return true;
  }

  int pid_;
  struct reg backup_ {};
  bool valid_ = false;
  bool dirty_ = false;
};

bool page_range(uint64_t address, size_t size, uint64_t& page_start,
                size_t& page_size) noexcept {
  if (address == 0 || size == 0 || size > UINT64_MAX - address) return false;
  page_start = address & ~(kPageSize - 1);
  const uint64_t end = address + size;
  if (end > UINT64_MAX - (kPageSize - 1)) return false;
  const uint64_t page_end = (end + kPageSize - 1) & ~(kPageSize - 1);
  page_size = static_cast<size_t>(page_end - page_start);
  return page_size > 0;
}

bool map_code_cave_common(int pid, uint64_t address, size_t size) noexcept {
  uint64_t page_start = 0;
  size_t mapping_size = 0;
  if (pid < 0 || !page_range(address, size, page_start, mapping_size)) {
    return false;
  }

  PtraceAttachment attachment(pid);
  if (!attachment.ready()) return false;
  RemoteRegisters registers(pid);
  if (!registers.valid()) return false;
  const uint64_t arguments[6] = {
      page_start,
      mapping_size,
      PROT_READ | PROT_WRITE,
      MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED,
      static_cast<uint64_t>(-1),
      0,
  };
  int64_t mapped = -1;
  if (!registers.invoke_syscall(SYS_mmap, arguments, mapped) ||
      static_cast<uint64_t>(mapped) != page_start) {
    return false;
  }
  return kernel_mprotect(pid, page_start, mapping_size,
                         PROT_READ | PROT_WRITE | PROT_EXEC) == 0;
}

class MdbgMemoryBackend final : public domain::IMemoryBackend {
 public:
  const char* name() const override { return "mdbg"; }

  bool read(int pid, uint64_t address, void* output, size_t size) override {
    return pid >= 0 && address != 0 && output != nullptr && size > 0 &&
           mdbg_copyout(pid, static_cast<intptr_t>(address), output, size) == 0;
  }

  bool write(int pid, uint64_t address, const void* input,
             size_t size) override {
    return pid >= 0 && address != 0 && input != nullptr && size > 0 &&
           mdbg_copyin(pid, input, static_cast<intptr_t>(address), size) == 0;
  }

  bool map_code_cave(int pid, uint64_t address, size_t size) override {
    return map_code_cave_common(pid, address, size);
  }
};

class KdirectMemoryBackend final : public domain::IMemoryBackend {
 public:
  const char* name() const override { return "kdirect"; }

  bool read(int pid, uint64_t address, void* output, size_t size) override {
    return copy(pid, address, output, size, false);
  }

  bool write(int pid, uint64_t address, const void* input,
             size_t size) override {
    return copy(pid, address, const_cast<void*>(input), size, true);
  }

  bool map_code_cave(int pid, uint64_t address, size_t size) override {
    return map_code_cave_common(pid, address, size);
  }

 private:
  static uint32_t firmware_major() noexcept {
    return MemoryBackendFactory::detect_firmware_major();
  }

  static bool page_table(int pid, uint64_t& cr3,
                         uint64_t& direct_map) noexcept {
    constexpr uint64_t kProcessVmspaceOffset = 0x200;
    const intptr_t process = kernel_get_proc(pid);
    if (process == 0) return false;
    uint64_t vmspace = 0;
    if (kernel_copyout(process + kProcessVmspaceOffset, &vmspace,
                       sizeof(vmspace)) < 0 ||
        vmspace == 0) {
      return false;
    }
    const uint64_t pmap_offset = firmware_major() >= 0x600 ? 0x2e8 : 0x2e0;
    uint64_t pointers[2]{};
    if (kernel_copyout(vmspace + pmap_offset + 32, pointers,
                       sizeof(pointers)) < 0 ||
        pointers[0] < pointers[1] || pointers[1] == 0) {
      return false;
    }
    direct_map = pointers[0] - pointers[1];
    cr3 = pointers[1];
    return true;
  }

  static uint64_t virtual_to_physical(uint64_t address, uint64_t direct_map,
                                      uint64_t table,
                                      uint64_t& physical_end) noexcept {
    for (int shift = 39; shift >= 12; shift -= 9) {
      uint64_t entry = 0;
      const uint64_t pte =
          direct_map + table +
          ((address & (0x1ffULL << shift)) >> (shift - 3));
      if (kernel_copyout(pte, &entry, sizeof(entry)) < 0 ||
          (entry & 1) == 0) {
        return kInvalidPhysicalAddress;
      }
      if ((entry & 128) != 0 || shift == 12) {
        entry &= (1ULL << 52) - (1ULL << shift);
        entry |= address & ((1ULL << shift) - 1);
        physical_end = (entry | ((1ULL << shift) - 1)) + 1;
        return entry;
      }
      table = entry & ((1ULL << 52) - (1ULL << 12));
    }
    return kInvalidPhysicalAddress;
  }

  static bool copy(int pid, uint64_t address, void* buffer, size_t size,
                   bool to_process) noexcept {
    if (pid < 0 || address == 0 || buffer == nullptr || size == 0 ||
        size > UINT64_MAX - address) {
      return false;
    }
    uint64_t cr3 = 0, direct_map = 0;
    if (!page_table(pid, cr3, direct_map)) return false;

    auto* bytes = static_cast<uint8_t*>(buffer);
    size_t remaining = size;
    while (remaining > 0) {
      uint64_t physical_end = 0;
      const uint64_t physical = virtual_to_physical(
          address, direct_map, cr3, physical_end);
      if (physical == kInvalidPhysicalAddress || physical_end <= physical) {
        return false;
      }
      size_t chunk = static_cast<size_t>(physical_end - physical);
      if (chunk > remaining) chunk = remaining;
      const int result =
          to_process ? kernel_copyin(bytes, direct_map + physical, chunk)
                     : kernel_copyout(direct_map + physical, bytes, chunk);
      if (result < 0) return false;
      address += chunk;
      bytes += chunk;
      remaining -= chunk;
    }
    return true;
  }
};

}  // namespace

domain::IMemoryBackend* MemoryBackendFactory::create(
    MemoryBackendKind requested, uint32_t firmware_major) noexcept {
  static MdbgMemoryBackend mdbg;
  static KdirectMemoryBackend kdirect;
  return resolve_kind(requested, firmware_major) == MemoryBackendKind::kdirect
             ? static_cast<domain::IMemoryBackend*>(&kdirect)
             : static_cast<domain::IMemoryBackend*>(&mdbg);
}

uint32_t MemoryBackendFactory::detect_firmware_major() noexcept {
  KernelVersion version{};
  return sceKernelGetProsperoSystemSwVersion(&version) == 0
             ? version.version >> 16
             : 0;
}

}  // namespace ezcheats::memory
