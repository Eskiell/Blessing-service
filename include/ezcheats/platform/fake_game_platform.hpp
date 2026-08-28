#pragma once

#include "ezcheats/domain/interfaces.hpp"

namespace ezcheats::platform {

class FakeGamePlatform final : public domain::IGamePlatform {
 public:
  void set_game(const domain::GameContext& game) noexcept;
  void clear_game() noexcept;
  bool add_module(int pid, int app_id,
                  const domain::ModuleInfo& module) noexcept;

  bool current_game(domain::GameContext& output) override;
  bool find_module(int pid, const char* module_name,
                   domain::ModuleInfo& output) override;
  bool find_module_in_app(int app_id, const char* module_name, int& pid,
                          domain::ModuleInfo& output) override;

 private:
  struct FakeModule {
    int pid;
    int app_id;
    domain::ModuleInfo module;
  };

  static constexpr size_t kMaxFakeModules = 16;
  domain::GameContext game_{};
  FakeModule modules_[kMaxFakeModules]{};
  size_t module_count_ = 0;
  bool has_game_ = false;
};

}  // namespace ezcheats::platform
