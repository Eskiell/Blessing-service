#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

namespace ezcheats::http {

class JsonWriter final {
 public:
  JsonWriter(char* buffer, size_t capacity) noexcept
      : buffer_(buffer), capacity_(capacity) {
    if (capacity_ > 0) {
      buffer_[0] = '\0';
    } else {
      ok_ = false;
    }
  }

  bool append(const char* text) { return append(text, strlen(text)); }

  bool append(const char* text, size_t size) {
    if (!ok_ || length_ >= capacity_ || size >= capacity_ - length_) {
      ok_ = false;
      return false;
    }
    memcpy(buffer_ + length_, text, size);
    length_ += size;
    buffer_[length_] = '\0';
    return true;
  }

  bool quoted(const char* text) {
    if (!append("\"")) return false;
    for (const unsigned char* p =
             reinterpret_cast<const unsigned char*>(text);
         *p != '\0'; ++p) {
      switch (*p) {
        case '"':
          if (!append("\\\"")) return false;
          break;
        case '\\':
          if (!append("\\\\")) return false;
          break;
        case '\n':
          if (!append("\\n")) return false;
          break;
        case '\r':
          if (!append("\\r")) return false;
          break;
        case '\t':
          if (!append("\\t")) return false;
          break;
        default:
          if (*p < 0x20) {
            char escaped[7]{};
            snprintf(escaped, sizeof(escaped), "\\u%04x", *p);
            if (!append(escaped)) return false;
          } else if (!append(reinterpret_cast<const char*>(p), 1)) {
            return false;
          }
      }
    }
    return append("\"");
  }

  bool number(uint32_t value) {
    char text[16]{};
    snprintf(text, sizeof(text), "%u", value);
    return append(text);
  }

  bool boolean(bool value) { return append(value ? "true" : "false"); }

  [[nodiscard]] bool ok() const noexcept { return ok_; }
  [[nodiscard]] const char* data() const noexcept { return buffer_; }
  [[nodiscard]] size_t size() const noexcept { return length_; }

 private:
  char* buffer_;
  size_t capacity_;
  size_t length_{0};
  bool ok_{true};
};

}  // namespace ezcheats::http
