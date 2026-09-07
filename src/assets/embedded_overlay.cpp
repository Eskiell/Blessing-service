#include "ezcheats/assets/embedded_overlay.hpp"

#ifndef EZ_CHEATS_OVERLAY_PATH
#error EZ_CHEATS_OVERLAY_PATH must point to blessing_overlay.elf
#endif

extern "C" {
extern const uint8_t blessing_overlay_elf_start[];
extern const uint8_t blessing_overlay_elf_end[];
}

__asm__(
    ".section .rodata\n"
    ".global blessing_overlay_elf_start\n"
    ".type blessing_overlay_elf_start, @object\n"
    ".balign 16\n"
    "blessing_overlay_elf_start:\n"
    ".incbin \"" EZ_CHEATS_OVERLAY_PATH "\"\n"
    ".global blessing_overlay_elf_end\n"
    ".type blessing_overlay_elf_end, @object\n"
    "blessing_overlay_elf_end:\n"
    ".previous\n");

namespace ezcheats::assets {

const uint8_t* embedded_overlay_data() noexcept {
  return blessing_overlay_elf_start;
}

size_t embedded_overlay_size() noexcept {
  return static_cast<size_t>(blessing_overlay_elf_end -
                             blessing_overlay_elf_start);
}

}  // namespace ezcheats::assets
