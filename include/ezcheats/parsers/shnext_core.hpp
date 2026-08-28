#pragma once

#include <stddef.h>

#include "ezcheats/domain/models.hpp"

#ifndef EZ_CHEATS_HAS_KEYSTONE
#define EZ_CHEATS_HAS_KEYSTONE 0
#endif

namespace ezcheats::parsers::detail {

bool parse_shnext(const char* data, size_t size,
                   domain::CheatFile* output);

}  // namespace ezcheats::parsers::detail
