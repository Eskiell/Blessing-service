#pragma once

#include <stdint.h>

namespace ezcheats::overlay {

enum class ShortcutEvent { none, activated };

// Edge-triggered long-press detector. It fires once per hold and rearms only
// after every shortcut button is released.
class HoldShortcut {
 public:
  HoldShortcut(uint32_t button_mask, uint64_t hold_duration_ms) noexcept
      : button_mask_(button_mask), hold_duration_ms_(hold_duration_ms) {}

  ShortcutEvent update(uint32_t buttons, uint64_t now_ms) noexcept;
  void reset() noexcept;

 private:
  uint32_t button_mask_ = 0;
  uint64_t hold_duration_ms_ = 0;
  uint64_t pressed_at_ms_ = 0;
  bool tracking_ = false;
  bool fired_ = false;
};

}  // namespace ezcheats::overlay
