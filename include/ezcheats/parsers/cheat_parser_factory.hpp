#pragma once

#include <stddef.h>
#include <stdint.h>

#include "ezcheats/domain/interfaces.hpp"

namespace ezcheats::parsers {

class CheatParserFactory final {
 public:
  static domain::ICheatParser* parser_for_format(const char* format) noexcept;
  static domain::ICheatParser* parser_for_path(const char* path) noexcept;
  static bool load_buffer(const char* format, const uint8_t* data, size_t size,
                          domain::CheatFile& output) noexcept;
  static bool load_file(const char* path,
                        domain::CheatFile& output) noexcept;
};

}  // namespace ezcheats::parsers
