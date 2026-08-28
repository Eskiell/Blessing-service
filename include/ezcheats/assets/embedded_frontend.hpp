#pragma once

#include <stddef.h>

namespace ezcheats::assets {

struct Asset {
  const unsigned char* data;
  size_t size;
};

Asset frontend();

}  // namespace ezcheats::assets
