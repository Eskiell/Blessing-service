#include "ezcheats/platform/fake_game_platform.hpp"

#include <string.h>

namespace ezcheats::platform {

void FakeGamePlatform::set_game(const domain::GameContext& game) noexcept {
  game_ = game;
  has_game_ = true;
}

void FakeGamePlatform::clear_game() noexcept {
  memset(&game_, 0, sizeof(game_));
  game_.pid = -1;
  game_.app_id = -1;
  memset(modules_, 0, sizeof(modules_));
  module_count_ = 0;
  has_game_ = false;
}

bool FakeGamePlatform::add_module(
    int pid, int app_id, const domain::ModuleInfo& module) noexcept {
  if (pid < 0 || app_id < 0 || module.name[0] == '\0' ||
      module.section_count > domain::kMaxModuleSections ||
      module_count_ >= kMaxFakeModules) {
    return false;
  }
  modules_[module_count_++] = {pid, app_id, module};
  return true;
}

bool FakeGamePlatform::current_game(domain::GameContext& output) {
  memset(&output, 0, sizeof(output));
  output.pid = -1;
  output.app_id = -1;
  if (!has_game_) return false;
  output = game_;
  return true;
}

bool FakeGamePlatform::find_module(int pid, const char* module_name,
                                   domain::ModuleInfo& output) {
  memset(&output, 0, sizeof(output));
  if (module_name == nullptr || module_name[0] == '\0') return false;
  for (size_t i = 0; i < module_count_; ++i) {
    if (modules_[i].pid == pid &&
        strcmp(modules_[i].module.name, module_name) == 0) {
      output = modules_[i].module;
      return true;
    }
  }
  return false;
}

bool FakeGamePlatform::find_module_in_app(int app_id, const char* module_name,
                                          int& pid,
                                          domain::ModuleInfo& output) {
  pid = -1;
  memset(&output, 0, sizeof(output));
  if (module_name == nullptr || module_name[0] == '\0') return false;
  for (size_t i = 0; i < module_count_; ++i) {
    if (modules_[i].app_id == app_id &&
        strcmp(modules_[i].module.name, module_name) == 0) {
      pid = modules_[i].pid;
      output = modules_[i].module;
      return true;
    }
  }
  return false;
}

}  // namespace ezcheats::platform
