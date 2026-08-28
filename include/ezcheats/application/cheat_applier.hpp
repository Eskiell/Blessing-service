#pragma once

#include <stddef.h>
#include <stdint.h>

#include "ezcheats/domain/interfaces.hpp"

namespace ezcheats::application {

enum class ApplyResult {
  success,
  invalid_cheat,
  invalid_patch,
  module_not_found,
  memory_error,
  rollback_error,
};

class CheatApplier final {
 public:
  CheatApplier(domain::IGamePlatform& platform,
               domain::IMemoryBackend& memory) noexcept
      : platform_(platform), memory_(memory) {}

  ApplyResult set_enabled(const domain::GameContext& game,
                          domain::CheatFile& file, uint32_t cheat_id,
                          bool enabled, char* status,
                          size_t status_size) noexcept;

 private:
  domain::IGamePlatform& platform_;
  domain::IMemoryBackend& memory_;
};

}  // namespace ezcheats::application
