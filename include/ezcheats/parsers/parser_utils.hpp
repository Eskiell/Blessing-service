#pragma once

#include <stddef.h>
#include <stdint.h>

namespace ezcheats::parsers::detail {

const char* skip_whitespace(const char* cursor, const char* end) noexcept;
const char* find_key(const char* start, const char* end,
                     const char* key) noexcept;
const char* find_matching(const char* start, const char* end, char opening,
                          char closing) noexcept;
bool extract_string(const char* start, const char* end, const char* key,
                    char* output, size_t capacity) noexcept;
bool extract_scalar(const char* start, const char* end, const char* key,
                    char* output, size_t capacity) noexcept;
bool decode_hex(const char* input, uint8_t* output, size_t capacity,
                size_t& output_size) noexcept;

}  // namespace ezcheats::parsers::detail
