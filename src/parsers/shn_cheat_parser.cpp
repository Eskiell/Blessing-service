#include "ezcheats/parsers/shn_cheat_parser.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ezcheats/domain/owned_cheat_file.hpp"
#include "ezcheats/parsers/parser_utils.hpp"

namespace ezcheats::parsers {
namespace {

const char* find_bounded(const char* start, const char* end,
                         const char* needle) noexcept {
  const size_t size = strlen(needle);
  if (size == 0) return start;
  for (const char* cursor = start; cursor + size <= end; ++cursor) {
    if (memcmp(cursor, needle, size) == 0) return cursor;
  }
  return nullptr;
}

void unescape_xml(char* text) noexcept {
  if (text == nullptr) return;
  struct Entity {
    const char* encoded;
    size_t size;
    char decoded;
  };
  constexpr Entity entities[] = {{"&quot;", 6, '"'}, {"&apos;", 6, '\''},
                                 {"&amp;", 5, '&'},  {"&lt;", 4, '<'},
                                 {"&gt;", 4, '>'}};
  char* read = text;
  char* write = text;
  while (*read != '\0') {
    bool decoded = false;
    for (const auto& entity : entities) {
      if (strncmp(read, entity.encoded, entity.size) == 0) {
        *write++ = entity.decoded;
        read += entity.size;
        decoded = true;
        break;
      }
    }
    if (!decoded) *write++ = *read++;
  }
  *write = '\0';
}

bool extract_attribute(const char* element, const char* element_end,
                       const char* attribute, char* output,
                       size_t capacity) noexcept {
  if (output == nullptr || capacity == 0) return false;
  output[0] = '\0';
  char marker[64]{};
  const int marker_size = snprintf(marker, sizeof(marker), "%s=\"", attribute);
  if (marker_size <= 0 || static_cast<size_t>(marker_size) >= sizeof(marker)) {
    return false;
  }
  const char* value = element;
  do {
    value = find_bounded(value, element_end, marker);
    if (value == nullptr) return false;
    if (value == element || value[-1] == ' ' || value[-1] == '\t' ||
        value[-1] == '\r' || value[-1] == '\n') {
      break;
    }
    ++value;
  } while (value < element_end);
  value += static_cast<size_t>(marker_size);
  const char* quote = find_bounded(value, element_end, "\"");
  if (quote == nullptr) return false;
  size_t length = static_cast<size_t>(quote - value);
  if (length >= capacity) length = capacity - 1;
  memcpy(output, value, length);
  output[length] = '\0';
  unescape_xml(output);
  return true;
}

bool extract_tag(const char* start, const char* end, const char* tag,
                 char* output, size_t capacity) noexcept {
  if (output == nullptr || capacity == 0) return false;
  output[0] = '\0';
  char opening[64]{};
  char closing[64]{};
  const int opening_size = snprintf(opening, sizeof(opening), "<%s>", tag);
  const int closing_size = snprintf(closing, sizeof(closing), "</%s>", tag);
  if (opening_size <= 0 || closing_size <= 0 ||
      static_cast<size_t>(opening_size) >= sizeof(opening) ||
      static_cast<size_t>(closing_size) >= sizeof(closing)) return false;
  const char* value = find_bounded(start, end, opening);
  if (value == nullptr) return false;
  value += static_cast<size_t>(opening_size);
  const char* close = find_bounded(value, end, closing);
  if (close == nullptr) return false;
  size_t length = static_cast<size_t>(close - value);
  if (length >= capacity) length = capacity - 1;
  memcpy(output, value, length);
  output[length] = '\0';
  unescape_xml(output);
  return true;
}

void replace_all(char* text, size_t& length, const char* from,
                 const char* to) noexcept {
  const size_t from_size = strlen(from);
  const size_t to_size = strlen(to);
  if (from_size == 0 || to_size > from_size) return;
  char* cursor = text;
  char* end = text + length;
  while (cursor + from_size <= end) {
    char* match = const_cast<char*>(find_bounded(cursor, end, from));
    if (match == nullptr) break;
    memcpy(match, to, to_size);
    const size_t tail = static_cast<size_t>(end - (match + from_size));
    memmove(match + to_size, match + from_size, tail);
    length -= from_size - to_size;
    end = text + length;
    text[length] = '\0';
    cursor = match + to_size;
  }
}

void remove_dashes(char* text) noexcept {
  char* output = text;
  for (char* cursor = text; *cursor != '\0'; ++cursor) {
    if (*cursor != '-') *output++ = *cursor;
  }
  *output = '\0';
}

bool parse_patch(const char* start, const char* end,
                 domain::Patch& patch) noexcept {
  char offset[64]{};
  char section[32]{};
  char enable[domain::kMaxPatchBytes * 2 + 1]{};
  char disable[domain::kMaxPatchBytes * 2 + 1]{};
  char absolute[32]{};
  if (!extract_tag(start, end, "Offset", offset, sizeof(offset)) ||
      !extract_tag(start, end, "ValueOn", enable, sizeof(enable)) ||
      !extract_tag(start, end, "ValueOff", disable, sizeof(disable))) {
    return false;
  }
  memset(&patch, 0, sizeof(patch));
  char* offset_end = nullptr;
  patch.offset = strtoull(offset, &offset_end, 16);
  if (offset_end == offset || *offset_end != '\0') return false;
  if (extract_tag(start, end, "Section", section, sizeof(section))) {
    const int parsed = atoi(section);
    if (parsed >= 0 && parsed < 4) patch.section = parsed;
  }
  patch.absolute = extract_tag(start, end, "Absolute", absolute,
                               sizeof(absolute)) && absolute[0] != '\0';
  remove_dashes(enable);
  remove_dashes(disable);
  return detail::decode_hex(enable, patch.enable, sizeof(patch.enable),
                            patch.enable_size) &&
         detail::decode_hex(disable, patch.disable, sizeof(patch.disable),
                            patch.disable_size) &&
         patch.enable_size > 0 && patch.disable_size > 0;
}

bool parse_xml(char* xml, size_t size, domain::CheatFile& output) noexcept {
  domain::clear_cheat_file(output);
  if (find_bounded(xml, xml + size, "<Trainer") == nullptr) {
    replace_all(xml, size, "&lt;", "<");
    replace_all(xml, size, "&gt;", ">");
    replace_all(xml, size, "\\&quot;", "\"");
    replace_all(xml, size, "&quot;", "\"");
  }
  const char* end = xml + size;

  const char* trainer = find_bounded(xml, end, "<Trainer");
  const char* trainer_end = trainer == nullptr ? nullptr : find_bounded(trainer, end, ">");
  if (trainer == nullptr || trainer_end == nullptr ||
      !extract_attribute(trainer, trainer_end, "Process", output.process,
                         sizeof(output.process)) ||
      !extract_attribute(trainer, trainer_end, "Game", output.name,
                         sizeof(output.name))) {
    domain::clear_cheat_file(output);
    return false;
  }
  char author[domain::kAuthorNameSize]{};
  if (extract_attribute(trainer, trainer_end, "Moder", author, sizeof(author))) {
    domain::add_author(output, author);
  }

  const char* cursor = trainer_end + 1;
  while ((cursor = find_bounded(cursor, end, "<Cheat ")) != nullptr) {
    const char* opening_end = find_bounded(cursor, end, ">");
    const char* cheat_end = opening_end == nullptr
                                ? nullptr
                                : find_bounded(opening_end + 1, end, "</Cheat>");
    if (opening_end == nullptr || cheat_end == nullptr ||
        !domain::ensure_cheat(output)) {
      domain::clear_cheat_file(output);
      return false;
    }
    domain::CheatEntry& cheat = output.cheats[output.cheat_count];
    memset(&cheat, 0, sizeof(cheat));
    if (!extract_attribute(cursor, opening_end, "Text", cheat.name,
                           sizeof(cheat.name))) {
      domain::clear_cheat_file(output);
      return false;
    }
    extract_attribute(cursor, opening_end, "Description", cheat.description,
                      sizeof(cheat.description));
    snprintf(cheat.module, sizeof(cheat.module), "%s", output.process);

    const char* line = opening_end + 1;
    while ((line = find_bounded(line, cheat_end, "<Cheatline>")) != nullptr) {
      const char* line_end = find_bounded(line, cheat_end, "</Cheatline>");
      if (line_end == nullptr || !domain::ensure_patch(cheat) ||
          !parse_patch(line, line_end, cheat.patches[cheat.patch_count])) {
        domain::clear_cheat_file(output);
        return false;
      }
      ++cheat.patch_count;
      line = line_end + strlen("</Cheatline>");
    }
    if (cheat.patch_count == 0) {
      domain::clear_cheat_file(output);
      return false;
    }
    cheat.id = static_cast<uint32_t>(output.cheat_count);
    ++output.cheat_count;
    cursor = cheat_end + strlen("</Cheat>");
  }
  if (output.cheat_count == 0) {
    domain::clear_cheat_file(output);
    return false;
  }
  return true;
}

}  // namespace

bool ShnCheatParser::parse(const uint8_t* data, size_t size,
                           domain::CheatFile& output) {
  domain::clear_cheat_file(output);
  if (data == nullptr || size == 0) return false;
  auto* xml = static_cast<char*>(calloc(size + 1, 1));
  if (xml == nullptr) return false;
  memcpy(xml, data, size);
  const bool parsed = parse_xml(xml, size, output);
  domain::secure_zero(xml, size + 1);
  free(xml);
  return parsed;
}

}  // namespace ezcheats::parsers
