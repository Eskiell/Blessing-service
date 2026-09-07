#pragma once

#include <stddef.h>
#include <stdint.h>

#include "ezcheats/domain/models.hpp"

namespace ezcheats::overlay {

// Transport boundary used by the ShellUI module. Implementations may use the
// existing loopback HTTP API or a native IPC channel without coupling the UI
// state to either transport.
class IOverlayClient {
 public:
  virtual bool fetch(domain::ServiceSnapshot& output, char* error,
                     size_t error_capacity) = 0;
  virtual bool set_enabled(uint32_t id, bool enabled,
                           domain::CheatEntry& updated, char* error,
                           size_t error_capacity) = 0;

 protected:
  ~IOverlayClient() = default;
};

}  // namespace ezcheats::overlay
