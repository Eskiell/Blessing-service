#include "ezcheats/overlay/hold_shortcut.hpp"

namespace ezcheats::overlay {

ShortcutEvent HoldShortcut::update(uint32_t buttons, uint64_t now_ms) noexcept {
  const bool held = button_mask_ != 0 &&
                    (buttons & button_mask_) == button_mask_;
  if (!held) {
    reset();
    return ShortcutEvent::none;
  }
  if (!tracking_ || now_ms < pressed_at_ms_) {
    tracking_ = true;
    fired_ = false;
    pressed_at_ms_ = now_ms;
    return ShortcutEvent::none;
  }
  if (!fired_ && now_ms - pressed_at_ms_ >= hold_duration_ms_) {
    fired_ = true;
    return ShortcutEvent::activated;
  }
  return ShortcutEvent::none;
}

void HoldShortcut::reset() noexcept {
  pressed_at_ms_ = 0;
  tracking_ = false;
  fired_ = false;
}

}  // namespace ezcheats::overlay
