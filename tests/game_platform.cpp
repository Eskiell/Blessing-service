#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "ezcheats/platform/fake_game_platform.hpp"

namespace {

ezcheats::domain::GameContext sample_game() {
  ezcheats::domain::GameContext game{};
  game.pid = 123;
  game.app_id = 456;
  snprintf(game.title_id, sizeof(game.title_id), "PPSA00001");
  snprintf(game.name, sizeof(game.name), "Test Game");
  snprintf(game.version, sizeof(game.version), "01.002.003");
  snprintf(game.platform, sizeof(game.platform), "ps5");
  snprintf(game.process_name, sizeof(game.process_name), "eboot.bin");
  return game;
}

ezcheats::domain::ModuleInfo sample_module() {
  ezcheats::domain::ModuleInfo module{};
  snprintf(module.name, sizeof(module.name), "eboot.bin");
  snprintf(module.path, sizeof(module.path), "/app0/eboot.bin");
  module.handle = 0x100;
  module.sections[0] = {0x400000, 0x1000, 5};
  module.sections[1] = {0x500000, 0x800, 3};
  module.section_count = 2;
  return module;
}

void represents_no_game_as_normal_state() {
  ezcheats::platform::FakeGamePlatform platform;
  ezcheats::domain::GameContext output{};
  output.pid = 99;
  assert(!platform.current_game(output));
  assert(output.pid == -1);
  assert(output.app_id == -1);
  assert(output.title_id[0] == '\0');
}

void returns_game_and_modules() {
  ezcheats::platform::FakeGamePlatform platform;
  const auto game = sample_game();
  const auto module = sample_module();
  platform.set_game(game);
  assert(platform.add_module(game.pid, game.app_id, module));

  ezcheats::domain::GameContext output{};
  assert(platform.current_game(output));
  assert(output.pid == 123 && output.app_id == 456);
  assert(strcmp(output.title_id, "PPSA00001") == 0);
  assert(strcmp(output.name, "Test Game") == 0);
  assert(strcmp(output.version, "01.002.003") == 0);
  assert(strcmp(output.platform, "ps5") == 0);
  assert(strcmp(output.process_name, "eboot.bin") == 0);

  ezcheats::domain::ModuleInfo found{};
  assert(platform.find_module(game.pid, "eboot.bin", found));
  assert(found.handle == 0x100 && found.section_count == 2);
  assert(found.sections[0].address == 0x400000);

  int module_pid = -1;
  assert(platform.find_module_in_app(game.app_id, "eboot.bin", module_pid,
                                     found));
  assert(module_pid == game.pid);
  assert(!platform.find_module(game.pid, "missing.sprx", found));
  assert(found.section_count == 0);

  platform.clear_game();
  assert(!platform.current_game(output));
  assert(output.pid == -1);
  assert(output.app_id == -1);
}

}  // namespace

int main() {
  represents_no_game_as_normal_state();
  returns_game_and_modules();
}
