#include <cassert>

#include "ezcheats/overlay/hold_shortcut.hpp"

int main() {
  using ezcheats::overlay::HoldShortcut;
  using ezcheats::overlay::ShortcutEvent;

  HoldShortcut shortcut{0x6, 1000};
  assert(shortcut.update(0, 0) == ShortcutEvent::none);
  assert(shortcut.update(0x2, 10) == ShortcutEvent::none);
  assert(shortcut.update(0x6, 100) == ShortcutEvent::none);
  assert(shortcut.update(0x6, 1099) == ShortcutEvent::none);
  assert(shortcut.update(0x6, 1100) == ShortcutEvent::activated);
  assert(shortcut.update(0x6, 2100) == ShortcutEvent::none);

  assert(shortcut.update(0, 2200) == ShortcutEvent::none);
  assert(shortcut.update(0x6, 2300) == ShortcutEvent::none);
  assert(shortcut.update(0x6, 3300) == ShortcutEvent::activated);

  shortcut.reset();
  assert(shortcut.update(0x6, 5000) == ShortcutEvent::none);
  assert(shortcut.update(0x6, 4000) == ShortcutEvent::none);
  assert(shortcut.update(0x6, 5000) == ShortcutEvent::activated);
}
