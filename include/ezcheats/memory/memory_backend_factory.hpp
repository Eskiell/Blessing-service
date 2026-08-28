#pragma once

#include <stdint.h>

#include "ezcheats/domain/interfaces.hpp"

namespace ezcheats::memory {

enum class MemoryBackendKind {
  automatic,
  mdbg,
  kdirect,
};

class MemoryBackendFactory final {
 public:
  static constexpr MemoryBackendKind resolve_kind(
      MemoryBackendKind requested, uint32_t firmware_major) noexcept {
    if (requested != MemoryBackendKind::automatic) return requested;
    return firmware_major >= 0x840 ? MemoryBackendKind::kdirect
                                   : MemoryBackendKind::mdbg;
  }

  static domain::IMemoryBackend* create(MemoryBackendKind requested,
                                        uint32_t firmware_major) noexcept;
  static uint32_t detect_firmware_major() noexcept;
};

}  // namespace ezcheats::memory
