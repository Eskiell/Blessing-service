#include "ezcheats/assets/embedded_frontend.hpp"

#include <stddef.h>

#ifndef EZ_CHEATS_FRONTEND_PATH
#error "EZ_CHEATS_FRONTEND_PATH must point to the generated index.html"
#endif

#define EMBED_ASSET(name, file)                                               \
  __asm__(".section .rodata\n"                                                \
          ".global " #name "\n"                                              \
          ".global " #name "_end\n"                                          \
          ".global " #name "_size\n"                                         \
          ".align 16\n" #name ":\n"                                          \
          ".incbin \"" file "\"\n" #name "_end:\n"                          \
          #name "_size:\n"                                                    \
          ".quad " #name "_end - " #name "\n"                               \
          ".previous\n");                                                     \
  extern const unsigned char name[];                                          \
  extern const size_t name##_size

EMBED_ASSET(ez_cheats_frontend, EZ_CHEATS_FRONTEND_PATH);

namespace ezcheats::assets {

Asset frontend() {
  return {ez_cheats_frontend, ez_cheats_frontend_size};
}

}  // namespace ezcheats::assets
