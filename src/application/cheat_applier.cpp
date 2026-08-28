#include "ezcheats/application/cheat_applier.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ezcheats/domain/owned_cheat_file.hpp"

namespace ezcheats::application {
namespace {

struct PatchSnapshot {
  uint64_t address;
  size_t size;
  uint8_t bytes[domain::kMaxPatchBytes];
};

void set_status(char* output, size_t output_size, const char* cheat,
                const char* message) noexcept {
  if (output == nullptr || output_size == 0) return;
  snprintf(output, output_size, "%s%s%s", cheat == nullptr ? "" : cheat,
           cheat != nullptr && cheat[0] != '\0' ? " -> " : "", message);
}

bool contains(const char* value, const char* token) noexcept {
  return value != nullptr && token != nullptr && strstr(value, token) != nullptr;
}

bool is_master_code(const domain::CheatEntry& cheat) noexcept {
  return contains(cheat.name, "Master Code") ||
         contains(cheat.name, "Mastercode");
}

domain::CheatEntry* find_cheat(domain::CheatFile& file, uint32_t id,
                               size_t& index) noexcept {
  for (size_t i = 0; i < file.cheat_count; ++i) {
    if (file.cheats[i].id == id) {
      index = i;
      return &file.cheats[i];
    }
  }
  return nullptr;
}

bool patch_address(const domain::Patch& patch, bool ps2, uint64_t base,
                   uint64_t& output) noexcept {
  if (patch.absolute || ps2) {
    output = patch.offset;
    return output != 0;
  }
  if (patch.offset > UINT64_MAX - base) return false;
  output = base + patch.offset;
  return output != 0;
}

bool rollback(domain::IMemoryBackend& memory, int pid, PatchSnapshot* snapshots,
              size_t count) noexcept {
  bool restored = true;
  while (count > 0) {
    --count;
    if (!memory.write_verified(pid, snapshots[count].address,
                               snapshots[count].bytes,
                               snapshots[count].size)) {
      restored = false;
    }
  }
  return restored;
}

void reset_states_for_pid(domain::CheatFile& file, int pid) noexcept {
  if (file.last_applied_pid == 0 || file.last_applied_pid == pid) return;
  for (size_t i = 0; i < file.cheat_count; ++i) {
    file.cheats[i].enabled = false;
  }
  file.master_code_id = -1;
  file.last_applied_pid = 0;
}

void fix_master_code(const domain::GameContext& game, domain::CheatFile& file,
                     domain::CheatEntry& cheat, uint64_t base,
                     domain::IMemoryBackend& memory) noexcept {
  if (file.master_code_id < 0 ||
      static_cast<size_t>(file.master_code_id) >= file.cheat_count ||
      !contains(cheat.name, "MC") || cheat.patch_count != 1 ||
      cheat.patches[0].section == 0) {
    return;
  }
  domain::CheatEntry& master = file.cheats[file.master_code_id];
  if (master.patch_count == 0) return;
  domain::Patch& master_patch = master.patches[0];
  domain::Patch& dependent = cheat.patches[0];
  if (master_patch.enable_size == 0 ||
      master_patch.enable_size > domain::kMaxPatchBytes ||
      dependent.disable_size == 0 ||
      dependent.disable_size > master_patch.enable_size) {
    return;
  }
  uint64_t master_address = 0;
  if (!patch_address(master_patch, false, base, master_address)) return;
  uint8_t bytes[domain::kMaxPatchBytes]{};
  if (!memory.read(game.pid, master_address, bytes,
                   master_patch.enable_size)) {
    return;
  }
  for (size_t i = 0;
       i + dependent.disable_size <= master_patch.enable_size; ++i) {
    if (memcmp(bytes + i, dependent.disable, dependent.disable_size) == 0) {
      dependent.offset = master_patch.offset + i;
      domain::secure_zero(bytes, sizeof(bytes));
      return;
    }
  }
  dependent.offset =
      ((master_patch.offset >> 8) << 8) | (dependent.offset & 0xff);
  domain::secure_zero(bytes, sizeof(bytes));
}

}  // namespace

ApplyResult CheatApplier::set_enabled(const domain::GameContext& game,
                                      domain::CheatFile& file,
                                      uint32_t cheat_id, bool enabled,
                                      char* status,
                                      size_t status_size) noexcept {
  if (status != nullptr && status_size > 0) status[0] = '\0';
  size_t cheat_index = 0;
  domain::CheatEntry* cheat = find_cheat(file, cheat_id, cheat_index);
  if (cheat == nullptr) {
    set_status(status, status_size, nullptr, "invalid cheat id");
    return ApplyResult::invalid_cheat;
  }
  if (game.pid < 0 || game.app_id < 0 || cheat->patch_count == 0 ||
      cheat->patch_count > domain::kMaxPatchesPerCheat ||
      cheat->patches == nullptr) {
    set_status(status, status_size, cheat->name, "invalid patch set");
    return ApplyResult::invalid_patch;
  }
  const char* module_name =
      cheat->module[0] != '\0' ? cheat->module : game.process_name;
  if (module_name == nullptr || module_name[0] == '\0') {
    set_status(status, status_size, cheat->name, "module not found");
    return ApplyResult::module_not_found;
  }
  int target_pid = game.pid;
  domain::ModuleInfo module{};
  if (!platform_.find_module(target_pid, module_name, module) &&
      !platform_.find_module_in_app(game.app_id, module_name, target_pid,
                                    module)) {
    set_status(status, status_size, cheat->name, "module not found");
    return ApplyResult::module_not_found;
  }
  if (target_pid < 0 || module.section_count == 0 ||
      module.sections[0].address == 0) {
    set_status(status, status_size, cheat->name, "invalid module sections");
    return ApplyResult::module_not_found;
  }

  reset_states_for_pid(file, target_pid);
  cheat = &file.cheats[cheat_index];
  if (cheat->enabled == enabled) {
    set_status(status, status_size, cheat->name,
               enabled ? "already enabled" : "already disabled");
    return ApplyResult::success;
  }
  if (file.master_code_id < 0 && is_master_code(*cheat)) {
    file.master_code_id = static_cast<int>(cheat_index);
  } else {
    domain::GameContext target = game;
    target.pid = target_pid;
    fix_master_code(target, file, *cheat, module.sections[0].address, memory_);
  }

  domain::ModuleInfo ps2_module{};
  const bool ps2 = platform_.find_module(
      target_pid, "libScePs2EmuMenuDialog.sprx", ps2_module);
  auto* snapshots = static_cast<PatchSnapshot*>(
      calloc(cheat->patch_count, sizeof(PatchSnapshot)));
  if (snapshots == nullptr) {
    set_status(status, status_size, cheat->name, "snapshot allocation failed");
    return ApplyResult::memory_error;
  }

  size_t applied = 0;
  ApplyResult result = ApplyResult::success;
  for (size_t i = 0; i < cheat->patch_count; ++i) {
    domain::Patch& patch = cheat->patches[i];
    const uint8_t* bytes = enabled ? patch.enable : patch.disable;
    const size_t size = enabled ? patch.enable_size : patch.disable_size;
    uint64_t address = 0;
    if (patch.assembly || size == 0 || size > domain::kMaxPatchBytes ||
        !patch_address(patch, ps2, module.sections[0].address, address)) {
      result = ApplyResult::invalid_patch;
      break;
    }
    snapshots[applied].address = address;
    snapshots[applied].size = size;
    if (!memory_.read(target_pid, address, snapshots[applied].bytes, size)) {
      if (!enabled || patch.disable_size != size ||
          !memory_.map_code_cave(target_pid, address, size)) {
        result = ApplyResult::memory_error;
        break;
      }
      memcpy(snapshots[applied].bytes, patch.disable, size);
    }
    ++applied;
    if (!memory_.write_verified(target_pid, address, bytes, size)) {
      result = ApplyResult::memory_error;
      break;
    }
  }

  if (result != ApplyResult::success) {
    const bool restored = rollback(memory_, target_pid, snapshots, applied);
    domain::secure_zero(snapshots,
                        cheat->patch_count * sizeof(PatchSnapshot));
    free(snapshots);
    if (!restored) {
      set_status(status, status_size, cheat->name, "rollback failed");
      return ApplyResult::rollback_error;
    }
    set_status(status, status_size, cheat->name,
               result == ApplyResult::invalid_patch ? "invalid patch"
                                                     : "memory write failed");
    return result;
  }

  domain::secure_zero(snapshots, cheat->patch_count * sizeof(PatchSnapshot));
  free(snapshots);
  cheat->enabled = enabled;
  file.last_applied_pid = target_pid;
  set_status(status, status_size, cheat->name,
             enabled ? "enabled" : "disabled");
  return ApplyResult::success;
}

}  // namespace ezcheats::application
