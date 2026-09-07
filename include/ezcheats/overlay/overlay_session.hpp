#pragma once

#include <stddef.h>

#include "ezcheats/domain/models.hpp"

namespace ezcheats::overlay {

// Frontend- and transport-independent state for the future ShellUI overlay.
// Rendering and controller hooks consume this model but remain PS5 adapters.
class OverlaySession {
 public:
  void open(const domain::ServiceSnapshot& snapshot) noexcept;
  void close() noexcept;
  void update(const domain::ServiceSnapshot& snapshot) noexcept;
  void select_next() noexcept;
  void select_previous() noexcept;

  bool visible() const noexcept { return visible_; }
  const domain::ServiceSnapshot& snapshot() const noexcept { return snapshot_; }
  const domain::CheatEntry* selected_cheat() const noexcept;

 private:
  void replace_snapshot(const domain::ServiceSnapshot& snapshot) noexcept;
  size_t selected_index_ = 0;
  bool visible_ = false;
  domain::ServiceSnapshot snapshot_{};
};

}  // namespace ezcheats::overlay
