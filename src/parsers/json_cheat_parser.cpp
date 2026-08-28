#include "ezcheats/parsers/json_cheat_parser.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ezcheats/domain/owned_cheat_file.hpp"
#include "ezcheats/parsers/parser_utils.hpp"

namespace ezcheats::parsers {
namespace {

bool parse_patch(const char* start, const char* end,
                 domain::Patch& patch) noexcept {
  char value[domain::kMaxPatchBytes * 2 + 1]{};
  memset(&patch, 0, sizeof(patch));
  if (!detail::extract_scalar(start, end, "offset", value, sizeof(value))) {
    return false;
  }
  char* offset_end = nullptr;
  patch.offset = strtoull(value, &offset_end, 16);
  if (offset_end == value || *offset_end != '\0') return false;

  if (!detail::extract_string(start, end, "on", value, sizeof(value)) ||
      !detail::decode_hex(value, patch.enable, sizeof(patch.enable),
                          patch.enable_size)) {
    return false;
  }
  if (!detail::extract_string(start, end, "off", value, sizeof(value)) ||
      !detail::decode_hex(value, patch.disable, sizeof(patch.disable),
                          patch.disable_size)) {
    return false;
  }

  if (detail::extract_scalar(start, end, "section", value, sizeof(value))) {
    const int section = atoi(value);
    if (section >= 0 && section < 4) patch.section = section;
  }
  if (detail::extract_scalar(start, end, "absolute", value, sizeof(value))) {
    patch.absolute = strcmp(value, "1") == 0 || strcmp(value, "true") == 0;
  }
  return patch.enable_size > 0 && patch.disable_size > 0;
}

bool parse_mod(const char* start, const char* end, const char* process,
               domain::CheatEntry& cheat) noexcept {
  memset(&cheat, 0, sizeof(cheat));
  if (!detail::extract_string(start, end, "name", cheat.name,
                              sizeof(cheat.name))) {
    return false;
  }
  detail::extract_string(start, end, "description", cheat.description,
                         sizeof(cheat.description));
  snprintf(cheat.module, sizeof(cheat.module), "%s", process);

  const char* memory = detail::find_key(start, end, "memory");
  if (memory == nullptr) return false;
  memory = detail::skip_whitespace(memory, end);
  if (memory >= end || *memory != '[') return false;
  const char* array_end = detail::find_matching(memory, end, '[', ']');
  if (array_end == nullptr) return false;

  const char* cursor = memory + 1;
  while (cursor < array_end) {
    cursor = detail::skip_whitespace(cursor, array_end);
    if (cursor >= array_end) break;
    if (*cursor != '{') {
      ++cursor;
      continue;
    }
    const char* object_end = detail::find_matching(cursor, array_end + 1, '{', '}');
    if (object_end == nullptr || !domain::ensure_patch(cheat)) return false;
    if (!parse_patch(cursor, object_end + 1, cheat.patches[cheat.patch_count])) {
      return false;
    }
    ++cheat.patch_count;
    cursor = object_end + 1;
  }
  return cheat.patch_count > 0;
}

void parse_authors(const char* start, const char* end, const char* key,
                   domain::CheatFile& output) noexcept {
  const char* array = detail::find_key(start, end, key);
  if (array == nullptr) return;
  array = detail::skip_whitespace(array, end);
  if (array >= end || *array != '[') return;
  const char* array_end = detail::find_matching(array, end, '[', ']');
  if (array_end == nullptr) return;
  const char* cursor = array + 1;
  while (cursor < array_end) {
    cursor = detail::skip_whitespace(cursor, array_end);
    if (cursor >= array_end) break;
    if (*cursor++ != '"') continue;
    const char* value = cursor;
    bool escaped = false;
    while (cursor < array_end) {
      if (escaped) {
        escaped = false;
      } else if (*cursor == '\\') {
        escaped = true;
      } else if (*cursor == '"') {
        break;
      }
      ++cursor;
    }
    if (cursor >= array_end) return;
    char author[domain::kAuthorNameSize]{};
    size_t length = static_cast<size_t>(cursor - value);
    if (length >= sizeof(author)) length = sizeof(author) - 1;
    memcpy(author, value, length);
    domain::add_author(output, author);
    ++cursor;
  }
}

}  // namespace

bool JsonCheatParser::parse(const uint8_t* data, size_t size,
                            domain::CheatFile& output) {
  domain::clear_cheat_file(output);
  if (data == nullptr || size == 0) return false;
  const char* json = reinterpret_cast<const char*>(data);
  const char* end = json + size;
  if (!detail::extract_string(json, end, "process", output.process,
                              sizeof(output.process)) ||
      !detail::extract_string(json, end, "name", output.name,
                              sizeof(output.name))) {
    domain::clear_cheat_file(output);
    return false;
  }

  parse_authors(json, end, "credits", output);
  parse_authors(json, end, "authors", output);
  const char* mods = detail::find_key(json, end, "mods");
  if (mods == nullptr) {
    domain::clear_cheat_file(output);
    return false;
  }
  mods = detail::skip_whitespace(mods, end);
  const char* mods_end = mods < end && *mods == '['
                             ? detail::find_matching(mods, end, '[', ']')
                             : nullptr;
  if (mods_end == nullptr) {
    domain::clear_cheat_file(output);
    return false;
  }

  const char* cursor = mods + 1;
  while (cursor < mods_end) {
    cursor = detail::skip_whitespace(cursor, mods_end);
    if (cursor >= mods_end) break;
    if (*cursor != '{') {
      ++cursor;
      continue;
    }
    const char* object_end = detail::find_matching(cursor, mods_end + 1, '{', '}');
    if (object_end == nullptr || !domain::ensure_cheat(output)) {
      domain::clear_cheat_file(output);
      return false;
    }
    domain::CheatEntry& cheat = output.cheats[output.cheat_count];
    if (!parse_mod(cursor, object_end + 1, output.process, cheat)) {
      domain::clear_cheat_file(output);
      return false;
    }
    cheat.id = static_cast<uint32_t>(output.cheat_count);
    ++output.cheat_count;
    cursor = object_end + 1;
  }
  if (output.cheat_count == 0) {
    domain::clear_cheat_file(output);
    return false;
  }
  return true;
}

}  // namespace ezcheats::parsers
