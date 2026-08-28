#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "ezcheats/application/cheat_applier.hpp"
#include "ezcheats/domain/owned_cheat_file.hpp"
#include "ezcheats/memory/fake_memory_backend.hpp"
#include "ezcheats/platform/fake_game_platform.hpp"

namespace {

using ezcheats::application::ApplyResult;
using ezcheats::application::CheatApplier;
using ezcheats::domain::CheatEntry;
using ezcheats::domain::CheatFile;
using ezcheats::domain::Patch;
using ezcheats::memory::FakeMemoryBackend;

ezcheats::domain::GameContext game(int pid = 42) {
  ezcheats::domain::GameContext value{};
  value.pid = pid;
  value.app_id = 7;
  snprintf(value.process_name, sizeof(value.process_name), "eboot.bin");
  return value;
}

ezcheats::domain::ModuleInfo module() {
  ezcheats::domain::ModuleInfo value{};
  snprintf(value.name, sizeof(value.name), "eboot.bin");
  value.sections[0] = {FakeMemoryBackend::kBaseAddress,
                       FakeMemoryBackend::kMemorySize, 5};
  value.section_count = 1;
  return value;
}

CheatEntry& add_cheat(CheatFile& file, uint32_t id, const char* name,
                      size_t patch_count) {
  assert(ezcheats::domain::ensure_cheat(file));
  CheatEntry& cheat = file.cheats[file.cheat_count++];
  cheat.id = id;
  snprintf(cheat.name, sizeof(cheat.name), "%s", name);
  snprintf(cheat.module, sizeof(cheat.module), "eboot.bin");
  for (size_t i = 0; i < patch_count; ++i) {
    assert(ezcheats::domain::ensure_patch(cheat));
    ++cheat.patch_count;
  }
  return cheat;
}

void set_patch(Patch& patch, uint64_t offset, uint8_t enabled,
               uint8_t disabled = 0) {
  patch.offset = offset;
  patch.enable[0] = enabled;
  patch.disable[0] = disabled;
  patch.enable_size = 1;
  patch.disable_size = 1;
}

void applies_and_restores_patch() {
  ezcheats::platform::FakeGamePlatform platform;
  FakeMemoryBackend memory;
  platform.set_game(game());
  assert(platform.add_module(42, 7, module()));
  CheatApplier applier(platform, memory);
  ezcheats::domain::OwnedCheatFile owned;
  CheatEntry& cheat = add_cheat(owned.get(), 10, "Infinite HP", 1);
  set_patch(cheat.patches[0], 0x20, 0xaa);
  char status[128];

  assert(applier.set_enabled(game(), owned.get(), 10, true, status,
                             sizeof(status)) == ApplyResult::success);
  assert(cheat.enabled && strstr(status, "enabled") != nullptr);
  uint8_t value = 0;
  assert(memory.read(42, FakeMemoryBackend::kBaseAddress + 0x20, &value, 1));
  assert(value == 0xaa);

  assert(applier.set_enabled(game(), owned.get(), 10, false, status,
                             sizeof(status)) == ApplyResult::success);
  assert(!cheat.enabled);
  assert(memory.read(42, FakeMemoryBackend::kBaseAddress + 0x20, &value, 1));
  assert(value == 0);
}

void rolls_back_partial_failure() {
  ezcheats::platform::FakeGamePlatform platform;
  FakeMemoryBackend memory;
  assert(platform.add_module(42, 7, module()));
  CheatApplier applier(platform, memory);
  ezcheats::domain::OwnedCheatFile owned;
  CheatEntry& cheat = add_cheat(owned.get(), 1, "Two patches", 2);
  set_patch(cheat.patches[0], 0x30, 0xaa);
  set_patch(cheat.patches[1], 0x31, 0xbb);
  memory.fail_on_write_call(2);
  char status[128];

  assert(applier.set_enabled(game(), owned.get(), 1, true, status,
                             sizeof(status)) == ApplyResult::memory_error);
  assert(!cheat.enabled);
  uint8_t values[2] = {1, 1};
  assert(memory.read(42, FakeMemoryBackend::kBaseAddress + 0x30, values, 2));
  assert(values[0] == 0 && values[1] == 0);
}

void maps_cave_when_original_address_is_unreadable() {
  ezcheats::platform::FakeGamePlatform platform;
  FakeMemoryBackend memory;
  assert(platform.add_module(42, 7, module()));
  CheatApplier applier(platform, memory);
  ezcheats::domain::OwnedCheatFile owned;
  CheatEntry& cheat = add_cheat(owned.get(), 2, "Cave", 1);
  set_patch(cheat.patches[0], 0x40, 0xcc);
  memory.fail_on_read_call(1);
  char status[128];

  assert(applier.set_enabled(game(), owned.get(), 2, true, status,
                             sizeof(status)) == ApplyResult::success);
  assert(memory.mapped_cave_count() == 1);
  assert(cheat.enabled);
}

void resolves_master_code_dependency() {
  ezcheats::platform::FakeGamePlatform platform;
  FakeMemoryBackend memory;
  assert(platform.add_module(42, 7, module()));
  CheatApplier applier(platform, memory);
  ezcheats::domain::OwnedCheatFile owned;
  CheatEntry& master = add_cheat(owned.get(), 0, "Master Code", 1);
  master.patches[0].offset = 0x100;
  master.patches[0].enable_size = 8;
  master.patches[0].disable_size = 8;
  CheatEntry& dependent = add_cheat(owned.get(), 1, "MC Ammo", 1);
  set_patch(dependent.patches[0], 0x1ff, 0xcc, 0xaa);
  dependent.patches[0].section = 1;
  owned.get().master_code_id = 0;
  const uint8_t master_bytes[8] = {0, 0, 0, 0xaa, 0, 0, 0, 0};
  assert(memory.write(42, FakeMemoryBackend::kBaseAddress + 0x100,
                      master_bytes, sizeof(master_bytes)));
  char status[128];

  assert(applier.set_enabled(game(), owned.get(), 1, true, status,
                             sizeof(status)) == ApplyResult::success);
  assert(dependent.patches[0].offset == 0x103);
}

void rejects_invalid_inputs_and_resets_pid_state() {
  ezcheats::platform::FakeGamePlatform platform;
  FakeMemoryBackend memory;
  assert(platform.add_module(77, 7, module()));
  CheatApplier applier(platform, memory);
  ezcheats::domain::OwnedCheatFile owned;
  CheatEntry& stale = add_cheat(owned.get(), 1, "Stale", 1);
  set_patch(stale.patches[0], 0x50, 0xaa);
  stale.enabled = true;
  CheatEntry& current = add_cheat(owned.get(), 2, "Current", 1);
  set_patch(current.patches[0], 0x60, 0xbb);
  current.enabled = true;
  owned.get().last_applied_pid = 42;
  char status[128];

  assert(applier.set_enabled(game(42), owned.get(), 2, true, status,
                             sizeof(status)) == ApplyResult::success);
  assert(!stale.enabled && current.enabled);
  assert(owned.get().last_applied_pid == 77);
  assert(applier.set_enabled(game(), owned.get(), 999, true, status,
                             sizeof(status)) == ApplyResult::invalid_cheat);
}

void reports_missing_module_name() {
  ezcheats::platform::FakeGamePlatform platform;
  FakeMemoryBackend memory;
  CheatApplier applier(platform, memory);
  ezcheats::domain::OwnedCheatFile owned;
  CheatEntry& cheat = add_cheat(owned.get(), 4, "Walk On Water", 1);
  set_patch(cheat.patches[0], 0x20, 0xaa);
  char status[256]{};

  assert(applier.set_enabled(game(), owned.get(), 4, true, status,
                             sizeof(status)) ==
         ApplyResult::module_not_found);
  assert(strstr(status, "module not found: eboot.bin") != nullptr);
}

}  // namespace

int main() {
  applies_and_restores_patch();
  rolls_back_partial_failure();
  maps_cave_when_original_address_is_unreadable();
  resolves_master_code_dependency();
  rejects_invalid_inputs_and_resets_pid_state();
  reports_missing_module_name();
}
