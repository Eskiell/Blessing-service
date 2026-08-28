#include "ezcheats/platform/ps5_game_platform.hpp"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include <unistd.h>

#include <ps5/kernel.h>

namespace ezcheats::platform {
namespace {

#ifndef SYS_dl_get_list
#define SYS_dl_get_list 0x217
#endif
#ifndef SYS_dl_get_info_2
#define SYS_dl_get_info_2 0x2cd
#endif

extern "C" {
int sceKernelGetProcessName(int pid, char* name);
int sceSystemServiceGetAppIdOfRunningBigApp(void);
int sceSystemServiceGetAppTitleId(int app_id, char* title_id);
}

struct AppInfo {
  uint32_t app_id;
  uint64_t unknown1;
  char title_id[14];
  char unknown2[0x3c];
};

extern "C" int sceKernelGetAppInfo(int pid, AppInfo* info);

struct NativeModuleSection {
  uint64_t address;
  uint64_t size;
  uint32_t protection;
};

struct NativeModuleInfo {
  char filename[128];
  uint64_t handle;
  uint8_t unknown0[32];
  uint64_t init;
  uint64_t fini;
  uint64_t eh_frame_header;
  uint64_t eh_frame_header_size;
  uint64_t eh_frame;
  uint64_t eh_frame_size;
  NativeModuleSection sections[4];
  uint8_t unknown7[1176];
  uint8_t fingerprint[20];
  uint32_t unknown8;
  char library_name[128];
  uint32_t unknown9;
  char sandboxed_path[1024];
  uint64_t sdk_version;
};

constexpr size_t kMaxMetadataSize = 256 * 1024;
constexpr intptr_t kProcessSharedObjectOffset = 0x3e8;
constexpr intptr_t kSharedLibraryImagebaseOffset = 0x30;

void copy_text(char* output, size_t output_size, const char* value) {
  if (output == nullptr || output_size == 0) return;
  snprintf(output, output_size, "%s", value == nullptr ? "" : value);
}

const char* basename_of(const char* path) {
  if (path == nullptr) return "";
  const char* slash = strrchr(path, '/');
  return slash == nullptr ? path : slash + 1;
}

void classify_platform(const char* title_id, char* output,
                       size_t output_size) {
  if (title_id != nullptr &&
      (strncmp(title_id, "PPSA", 4) == 0 ||
       strncmp(title_id, "PCSA", 4) == 0)) {
    copy_text(output, output_size, "ps5");
  } else if (title_id != nullptr &&
             (strncmp(title_id, "CUSA", 4) == 0 ||
              strncmp(title_id, "CUSB", 4) == 0 ||
              strncmp(title_id, "CUSC", 4) == 0 ||
              strncmp(title_id, "PCAS", 4) == 0)) {
    copy_text(output, output_size, "ps4");
  } else {
    copy_text(output, output_size, "unknown");
  }
}

bool read_file(const char* path, uint8_t** output, size_t* output_size) {
  *output = nullptr;
  *output_size = 0;
  FILE* file = fopen(path, "rb");
  if (file == nullptr || fseek(file, 0, SEEK_END) != 0) {
    if (file != nullptr) fclose(file);
    return false;
  }
  const long length = ftell(file);
  if (length <= 0 || static_cast<size_t>(length) > kMaxMetadataSize ||
      fseek(file, 0, SEEK_SET) != 0) {
    fclose(file);
    return false;
  }
  auto* data = static_cast<uint8_t*>(calloc(static_cast<size_t>(length) + 1, 1));
  if (data == nullptr) {
    fclose(file);
    return false;
  }
  const bool read = fread(data, 1, static_cast<size_t>(length), file) ==
                    static_cast<size_t>(length);
  fclose(file);
  if (!read) {
    free(data);
    return false;
  }
  *output = data;
  *output_size = static_cast<size_t>(length);
  return true;
}

bool extract_json_string(const char* json, const char* key, char* output,
                         size_t output_size) {
  if (json == nullptr || key == nullptr || output_size == 0) return false;
  char search[64];
  if (snprintf(search, sizeof(search), "\"%s\"", key) < 0) return false;
  const char* cursor = strstr(json, search);
  if (cursor == nullptr) return false;
  cursor = strchr(cursor + strlen(search), ':');
  if (cursor == nullptr) return false;
  do {
    ++cursor;
  } while (*cursor != '\0' && isspace(static_cast<unsigned char>(*cursor)));
  if (*cursor++ != '"') return false;
  size_t written = 0;
  while (*cursor != '\0' && *cursor != '"' && written + 1 < output_size) {
    output[written++] = *cursor++;
  }
  output[written] = '\0';
  return written > 0 && *cursor == '"';
}

bool extract_sfo_string(const uint8_t* data, size_t size, const char* key,
                        char* output, size_t output_size) {
  if (data == nullptr || size < 20 || output_size == 0 ||
      memcmp(data, "\0PSF", 4) != 0) {
    return false;
  }
  uint32_t key_table = 0, data_table = 0, entries = 0;
  memcpy(&key_table, data + 8, sizeof(key_table));
  memcpy(&data_table, data + 12, sizeof(data_table));
  memcpy(&entries, data + 16, sizeof(entries));
  if (key_table >= size || data_table >= size || entries > 4096) return false;

  for (uint32_t i = 0; i < entries; ++i) {
    const size_t index = 20 + static_cast<size_t>(i) * 16;
    if (index + 16 > size || index + 16 > key_table) break;
    uint16_t key_offset = 0;
    uint32_t value_length = 0, value_offset = 0;
    memcpy(&key_offset, data + index, sizeof(key_offset));
    memcpy(&value_length, data + index + 4, sizeof(value_length));
    memcpy(&value_offset, data + index + 12, sizeof(value_offset));
    const size_t key_position = key_table + key_offset;
    const size_t value_position = data_table + value_offset;
    if (key_position >= size || value_position >= size ||
        value_length > size - value_position ||
        memchr(data + key_position, '\0', size - key_position) == nullptr ||
        strcmp(reinterpret_cast<const char*>(data + key_position), key) != 0) {
      continue;
    }
    size_t copied = value_length;
    if (copied >= output_size) copied = output_size - 1;
    memcpy(output, data + value_position, copied);
    output[copied] = '\0';
    return output[0] != '\0';
  }
  return false;
}

void resolve_metadata(domain::GameContext& game) {
  static constexpr const char* kPs5Paths[] = {
      "/system_data/priv/appmeta/%s/param.json",
      "/system_data/priv/appmeta/external/%s/param.json",
      "/system_ex/app/%s/sce_sys/param.json", "/user/appmeta/%s/param.json"};
  static constexpr const char* kPs4Paths[] = {
      "/system_data/priv/appmeta/%s/param.sfo",
      "/system_data/priv/appmeta/external/%s/param.sfo",
      "/user/appmeta/%s/param.sfo"};

  const bool ps5 = strcmp(game.platform, "ps5") == 0;
  const char* const* paths = ps5 ? kPs5Paths : kPs4Paths;
  const size_t path_count =
      ps5 ? sizeof(kPs5Paths) / sizeof(kPs5Paths[0])
          : sizeof(kPs4Paths) / sizeof(kPs4Paths[0]);
  for (size_t i = 0; i < path_count; ++i) {
    char path[256];
    snprintf(path, sizeof(path), paths[i], game.title_id);
    uint8_t* data = nullptr;
    size_t size = 0;
    if (!read_file(path, &data, &size)) continue;
    if (ps5) {
      static constexpr const char* kVersionKeys[] = {
          "contentVersion", "content_version", "titleVersion", "version",
          "APP_VER"};
      for (const char* key : kVersionKeys) {
        if (extract_json_string(reinterpret_cast<const char*>(data), key,
                                game.version, sizeof(game.version))) {
          break;
        }
      }
      extract_json_string(reinterpret_cast<const char*>(data), "titleName",
                          game.name, sizeof(game.name));
    } else {
      char version[32]{};
      char app_version[32]{};
      extract_sfo_string(data, size, "VERSION", version, sizeof(version));
      extract_sfo_string(data, size, "APP_VER", app_version,
                         sizeof(app_version));
      copy_text(game.version, sizeof(game.version),
                app_version[0] != '\0' ? app_version : version);
      extract_sfo_string(data, size, "TITLE", game.name, sizeof(game.name));
    }
    free(data);
    if (game.version[0] != '\0' || game.name[0] != '\0') break;
  }
  if (game.version[0] == '\0') copy_text(game.version, sizeof(game.version), "unknown");
  if (game.name[0] == '\0') copy_text(game.name, sizeof(game.name), game.title_id);
}

bool read_process_table(void** output, size_t* output_size) {
  int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PROC, 0};
  *output = nullptr;
  *output_size = 0;
  if (sysctl(mib, 4, nullptr, output_size, nullptr, 0) != 0 ||
      *output_size == 0) {
    return false;
  }
  void* data = malloc(*output_size);
  if (data == nullptr) return false;
  if (sysctl(mib, 4, data, output_size, nullptr, 0) != 0) {
    free(data);
    return false;
  }
  *output = data;
  return true;
}

bool process_matches_app(int pid, int app_id, AppInfo* output) {
  AppInfo info{};
  if (sceKernelGetAppInfo(pid, &info) < 0 ||
      static_cast<int>(info.app_id) != app_id) {
    return false;
  }
  if (output != nullptr) *output = info;
  return true;
}

bool find_app_process(int app_id, int& pid, char* process_name,
                      size_t process_name_size, char* title_id,
                      size_t title_id_size) {
  void* table = nullptr;
  size_t table_size = 0;
  if (read_process_table(&table, &table_size)) {
    char* cursor = static_cast<char*>(table);
    const char* end = cursor + table_size;
    while (cursor < end) {
      auto* process = reinterpret_cast<struct kinfo_proc*>(cursor);
      if (process->ki_structsize <= 0 ||
          static_cast<size_t>(end - cursor) <
              static_cast<size_t>(process->ki_structsize)) {
        break;
      }
      cursor += process->ki_structsize;
      AppInfo info{};
      char name[64]{};
      if (!process_matches_app(process->ki_pid, app_id, &info) ||
          sceKernelGetProcessName(process->ki_pid, name) < 0 ||
          strcmp(process->ki_comm, name) != 0) {
        continue;
      }
      pid = process->ki_pid;
      copy_text(process_name, process_name_size, name);
      if (title_id[0] == '\0') copy_text(title_id, title_id_size, info.title_id);
      free(table);
      return true;
    }
    free(table);
  }

  for (int candidate = 1; candidate <= 9999; ++candidate) {
    AppInfo info{};
    if (!process_matches_app(candidate, app_id, &info)) continue;
    pid = candidate;
    if (sceKernelGetProcessName(candidate, process_name) < 0) {
      process_name[0] = '\0';
    }
    if (title_id[0] == '\0') copy_text(title_id, title_id_size, info.title_id);
    return true;
  }
  return false;
}

bool module_matches(const NativeModuleInfo& module, const char* name) {
  return strcmp(module.filename, name) == 0 ||
         strcmp(module.library_name, name) == 0 ||
         strcmp(basename_of(module.sandboxed_path), name) == 0;
}

void convert_module(const NativeModuleInfo& native,
                    domain::ModuleInfo& output) {
  memset(&output, 0, sizeof(output));
  const char* name = native.filename[0] != '\0'
                         ? native.filename
                         : (native.library_name[0] != '\0'
                                ? native.library_name
                                : basename_of(native.sandboxed_path));
  copy_text(output.name, sizeof(output.name), name);
  copy_text(output.path, sizeof(output.path), native.sandboxed_path);
  output.handle = native.handle;
  for (size_t i = 0; i < domain::kMaxModuleSections; ++i) {
    if (native.sections[i].address == 0) continue;
    output.sections[output.section_count++] = {
        native.sections[i].address, native.sections[i].size,
        native.sections[i].protection};
  }
}

bool find_native_module(int pid, const char* name,
                        domain::ModuleInfo& output) {
  if (pid < 0 || name == nullptr || name[0] == '\0') return false;
  size_t handle_count = 0;
  if (syscall(SYS_dl_get_list, pid, nullptr, 0, &handle_count) < 0 ||
      handle_count == 0 || handle_count > 4096) {
    return false;
  }
  auto* handles = static_cast<uintptr_t*>(
      calloc(handle_count, sizeof(uintptr_t)));
  if (handles == nullptr) return false;
  if (syscall(SYS_dl_get_list, pid, handles, handle_count, &handle_count) < 0) {
    free(handles);
    return false;
  }
  for (size_t i = 0; i < handle_count; ++i) {
    NativeModuleInfo module{};
    if (syscall(SYS_dl_get_info_2, pid, 1, handles[i], &module) >= 0 &&
        module_matches(module, name)) {
      convert_module(module, output);
      free(handles);
      return output.section_count > 0;
    }
  }
  free(handles);
  return false;
}

bool find_eboot_imagebase(int pid, const char* name,
                          domain::ModuleInfo& output) {
  if (pid < 0 || name == nullptr || name[0] == '\0') return false;
  char process_name[64]{};
  if (sceKernelGetProcessName(pid, process_name) < 0 ||
      (strcmp(name, "eboot") != 0 && strcmp(name, "eboot.bin") != 0 &&
       strcmp(name, process_name) != 0)) {
    return false;
  }

  const intptr_t process = kernel_get_proc(pid);
  intptr_t shared_object = 0;
  intptr_t eboot = 0;
  uint64_t imagebase = 0;
  if (process == 0 ||
      kernel_copyout(process + kProcessSharedObjectOffset, &shared_object,
                     sizeof(shared_object)) < 0 ||
      shared_object == 0 ||
      kernel_copyout(shared_object, &eboot, sizeof(eboot)) < 0 || eboot == 0) {
    return false;
  }
  if (kernel_copyout(eboot + kSharedLibraryImagebaseOffset, &imagebase,
                     sizeof(imagebase)) < 0 ||
      imagebase == 0) {
    if (kernel_copyout(eboot + 0x38, &imagebase, sizeof(imagebase)) < 0 ||
        imagebase == 0) {
      return false;
    }
  }

  memset(&output, 0, sizeof(output));
  copy_text(output.name, sizeof(output.name), name);
  copy_text(output.path, sizeof(output.path), name);
  output.handle = static_cast<uint64_t>(eboot);
  output.sections[0] = {imagebase, 0, PROT_READ | PROT_EXEC};
  output.section_count = 1;
  return true;
}

bool find_module_with_fallback(int pid, const char* name,
                               domain::ModuleInfo& output) {
  return find_native_module(pid, name, output) ||
         find_eboot_imagebase(pid, name, output);
}

}  // namespace

bool Ps5GamePlatform::current_game(domain::GameContext& output) {
  memset(&output, 0, sizeof(output));
  output.pid = -1;
  output.app_id = -1;
  const int app_id = sceSystemServiceGetAppIdOfRunningBigApp();
  if (app_id < 0) return false;
  output.app_id = app_id;
  if (sceSystemServiceGetAppTitleId(app_id, output.title_id) != 0) {
    output.title_id[0] = '\0';
  }
  if (!find_app_process(app_id, output.pid, output.process_name,
                        sizeof(output.process_name), output.title_id,
                        sizeof(output.title_id))) {
    memset(&output, 0, sizeof(output));
    output.pid = -1;
    output.app_id = -1;
    return false;
  }
  classify_platform(output.title_id, output.platform, sizeof(output.platform));
  resolve_metadata(output);
  return true;
}

bool Ps5GamePlatform::find_module(int pid, const char* module_name,
                                  domain::ModuleInfo& output) {
  memset(&output, 0, sizeof(output));
  return find_module_with_fallback(pid, module_name, output);
}

bool Ps5GamePlatform::find_module_in_app(int app_id, const char* module_name,
                                         int& pid,
                                         domain::ModuleInfo& output) {
  pid = -1;
  memset(&output, 0, sizeof(output));
  void* table = nullptr;
  size_t table_size = 0;
  if (read_process_table(&table, &table_size)) {
    char* cursor = static_cast<char*>(table);
    const char* end = cursor + table_size;
    while (cursor < end) {
      auto* process = reinterpret_cast<struct kinfo_proc*>(cursor);
      if (process->ki_structsize <= 0 ||
          static_cast<size_t>(end - cursor) <
              static_cast<size_t>(process->ki_structsize)) {
        break;
      }
      cursor += process->ki_structsize;
      if (process_matches_app(process->ki_pid, app_id, nullptr) &&
          find_module_with_fallback(process->ki_pid, module_name, output)) {
        pid = process->ki_pid;
        free(table);
        return true;
      }
    }
    free(table);
  }
  for (int candidate = 1; candidate <= 9999; ++candidate) {
    if (process_matches_app(candidate, app_id, nullptr) &&
        find_module_with_fallback(candidate, module_name, output)) {
      pid = candidate;
      return true;
    }
  }
  return false;
}

}  // namespace ezcheats::platform
