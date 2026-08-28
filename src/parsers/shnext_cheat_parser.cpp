#include "ezcheats/parsers/shnext_cheat_parser.hpp"

#include "ezcheats/domain/owned_cheat_file.hpp"
#include "ezcheats/parsers/shnext_core.hpp"

namespace ezcheats::parsers {

bool ShnExtCheatParser::parse(const uint8_t* data, size_t size,
                              domain::CheatFile& output) {
  domain::clear_cheat_file(output);
  if (data == nullptr || size == 0) return false;
  const bool parsed = detail::parse_shnext(
      reinterpret_cast<const char*>(data), size, &output);
  if (!parsed) domain::clear_cheat_file(output);
  return parsed;
}

}  // namespace ezcheats::parsers
