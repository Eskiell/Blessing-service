#pragma once

#include <pthread.h>

#include "ezcheats/application/cheat_applier.hpp"
#include "ezcheats/domain/owned_cheat_file.hpp"
#include "ezcheats/repository/file_cheat_repository.hpp"

namespace ezcheats::application {

class CheatService final : public domain::ICheatService {
 public:
  CheatService(domain::IGamePlatform& platform,
               repository::FileCheatRepository& repository,
               domain::IMemoryBackend& memory) noexcept;
  ~CheatService();

  bool refresh() override;
  bool snapshot(domain::ServiceSnapshot& output) const override;
  bool set_enabled(uint32_t id, bool enabled,
                   domain::CheatEntry& updated) override;

 private:
  bool refresh_locked() noexcept;
  bool disable_enabled_locked() noexcept;
  void clear_locked() noexcept;
  void set_error_locked(const char* message) noexcept;

  domain::IGamePlatform& platform_;
  repository::FileCheatRepository& repository_;
  domain::IMemoryBackend& memory_;
  CheatApplier applier_;
  mutable pthread_mutex_t mutex_{};
  bool mutex_ready_ = false;
  bool has_game_ = false;
  bool loaded_ = false;
  domain::GameContext game_{};
  domain::OwnedCheatFile file_;
  char error_[160]{};
};

}  // namespace ezcheats::application
