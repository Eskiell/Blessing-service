#pragma once

#include "ezcheats/domain/interfaces.hpp"

namespace ezcheats::platform {

class Ps5GamePlatform final : public domain::IGamePlatform {
 public:
  bool current_game(domain::GameContext& output) override;
  bool find_module(int pid, const char* module_name,
                   domain::ModuleInfo& output) override;
  bool find_module_in_app(int app_id, const char* module_name, int& pid,
                          domain::ModuleInfo& output) override;
};

}  // namespace ezcheats::platform
