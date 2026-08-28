#include "ezcheats/parsers/cheat_parser_factory.hpp"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ezcheats/domain/owned_cheat_file.hpp"
#include "ezcheats/parsers/json_cheat_parser.hpp"
#include "ezcheats/parsers/mc4_cheat_parser.hpp"
#include "ezcheats/parsers/shn_cheat_parser.hpp"

namespace ezcheats::parsers {
namespace {

bool format_is(const char* format, const char* expected) noexcept {
  while (*format == '.') ++format;
  while (*format != '\0' && *expected != '\0') {
    if (tolower(static_cast<unsigned char>(*format)) !=
        tolower(static_cast<unsigned char>(*expected))) return false;
    ++format;
    ++expected;
  }
  return *format == '\0' && *expected == '\0';
}

}  // namespace

domain::ICheatParser* CheatParserFactory::parser_for_format(
    const char* format) noexcept {
  static JsonCheatParser json;
  static ShnCheatParser shn;
  static Mc4CheatParser mc4;
  if (format != nullptr && format_is(format, "json")) return &json;
  if (format != nullptr && format_is(format, "shn")) return &shn;
  if (format != nullptr && format_is(format, "mc4")) return &mc4;
  return nullptr;
}

domain::ICheatParser* CheatParserFactory::parser_for_path(
    const char* path) noexcept {
  if (path == nullptr) return nullptr;
  const char* extension = strrchr(path, '.');
  return extension == nullptr ? nullptr : parser_for_format(extension + 1);
}

bool CheatParserFactory::load_buffer(const char* format, const uint8_t* data,
                                     size_t size,
                                     domain::CheatFile& output) noexcept {
  domain::ICheatParser* parser = parser_for_format(format);
  if (parser == nullptr || data == nullptr || size == 0) {
    domain::clear_cheat_file(output);
    return false;
  }
  return parser->parse(data, size, output);
}

bool CheatParserFactory::load_file(const char* path,
                                   domain::CheatFile& output) noexcept {
  domain::clear_cheat_file(output);
  domain::ICheatParser* parser = parser_for_path(path);
  if (parser == nullptr) return false;
  FILE* file = fopen(path, "rb");
  if (file == nullptr || fseek(file, 0, SEEK_END) != 0) {
    if (file != nullptr) fclose(file);
    return false;
  }
  const long length = ftell(file);
  if (length <= 0 || fseek(file, 0, SEEK_SET) != 0) {
    fclose(file);
    return false;
  }
  auto* buffer = static_cast<uint8_t*>(calloc(static_cast<size_t>(length) + 1, 1));
  if (buffer == nullptr) {
    fclose(file);
    return false;
  }
  const bool read = fread(buffer, 1, static_cast<size_t>(length), file) ==
                    static_cast<size_t>(length);
  fclose(file);
  const bool parsed = read && parser->parse(buffer, static_cast<size_t>(length), output);
  domain::secure_zero(buffer, static_cast<size_t>(length));
  free(buffer);
  return parsed;
}

}  // namespace ezcheats::parsers
