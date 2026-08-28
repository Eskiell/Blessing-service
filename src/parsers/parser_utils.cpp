#include "ezcheats/parsers/parser_utils.hpp"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace ezcheats::parsers::detail {

const char* skip_whitespace(const char* cursor, const char* end) noexcept {
  while (cursor < end && isspace(static_cast<unsigned char>(*cursor))) ++cursor;
  return cursor;
}

const char* find_key(const char* start, const char* end,
                     const char* key) noexcept {
  char pattern[64]{};
  const int size = snprintf(pattern, sizeof(pattern), "\"%s\"", key);
  if (size <= 0 || static_cast<size_t>(size) >= sizeof(pattern)) return nullptr;
  const size_t pattern_size = static_cast<size_t>(size);
  for (const char* candidate = start; candidate + pattern_size <= end;
       ++candidate) {
    if (memcmp(candidate, pattern, pattern_size) != 0) continue;
    const char* cursor = skip_whitespace(candidate + pattern_size, end);
    if (cursor < end && *cursor == ':') return cursor + 1;
  }
  return nullptr;
}

const char* find_matching(const char* start, const char* end, char opening,
                          char closing) noexcept {
  int depth = 0;
  bool in_string = false;
  bool escaped = false;
  for (const char* cursor = start; cursor < end; ++cursor) {
    if (in_string) {
      if (escaped) {
        escaped = false;
      } else if (*cursor == '\\') {
        escaped = true;
      } else if (*cursor == '"') {
        in_string = false;
      }
      continue;
    }
    if (*cursor == '"') {
      in_string = true;
    } else if (*cursor == opening) {
      ++depth;
    } else if (*cursor == closing && --depth == 0) {
      return cursor;
    }
  }
  return nullptr;
}

bool extract_string(const char* start, const char* end, const char* key,
                    char* output, size_t capacity) noexcept {
  if (output == nullptr || capacity == 0) return false;
  output[0] = '\0';
  const char* cursor = find_key(start, end, key);
  if (cursor == nullptr) return false;
  cursor = skip_whitespace(cursor, end);
  if (cursor >= end || *cursor != '"') return false;
  ++cursor;
  const char* value_start = cursor;
  bool escaped = false;
  while (cursor < end) {
    if (escaped) {
      escaped = false;
    } else if (*cursor == '\\') {
      escaped = true;
    } else if (*cursor == '"') {
      size_t length = static_cast<size_t>(cursor - value_start);
      if (length >= capacity) length = capacity - 1;
      memcpy(output, value_start, length);
      output[length] = '\0';
      return true;
    }
    ++cursor;
  }
  return false;
}

bool extract_scalar(const char* start, const char* end, const char* key,
                    char* output, size_t capacity) noexcept {
  if (output == nullptr || capacity == 0) return false;
  output[0] = '\0';
  const char* cursor = find_key(start, end, key);
  if (cursor == nullptr) return false;
  cursor = skip_whitespace(cursor, end);
  if (cursor >= end) return false;
  if (*cursor == '"') return extract_string(start, end, key, output, capacity);
  const char* value_start = cursor;
  while (cursor < end && *cursor != ',' && *cursor != '}' && *cursor != ']' &&
         !isspace(static_cast<unsigned char>(*cursor))) {
    ++cursor;
  }
  size_t length = static_cast<size_t>(cursor - value_start);
  if (length == 0) return false;
  if (length >= capacity) length = capacity - 1;
  memcpy(output, value_start, length);
  output[length] = '\0';
  return true;
}

bool decode_hex(const char* input, uint8_t* output, size_t capacity,
                size_t& output_size) noexcept {
  output_size = 0;
  if (input == nullptr || output == nullptr) return false;
  const size_t length = strlen(input);
  if ((length + 1) / 2 > capacity) return false;
  size_t input_index = 0;
  char pair[3] = {'0', '0', '\0'};
  if ((length % 2) != 0) {
    if (!isxdigit(static_cast<unsigned char>(input[0]))) return false;
    pair[1] = input[input_index++];
    output[output_size++] = static_cast<uint8_t>(strtoul(pair, nullptr, 16));
  }
  while (input_index < length) {
    pair[0] = input[input_index++];
    pair[1] = input[input_index++];
    if (!isxdigit(static_cast<unsigned char>(pair[0])) ||
        !isxdigit(static_cast<unsigned char>(pair[1]))) return false;
    output[output_size++] = static_cast<uint8_t>(strtoul(pair, nullptr, 16));
  }
  return true;
}

}  // namespace ezcheats::parsers::detail
