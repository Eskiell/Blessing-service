#pragma once

#include <unistd.h>

namespace ezcheats::platform {

class UniqueFd final {
 public:
  explicit UniqueFd(int value = -1) noexcept : value_(value) {}
  ~UniqueFd() { reset(); }

  UniqueFd(const UniqueFd&) = delete;
  UniqueFd& operator=(const UniqueFd&) = delete;

  UniqueFd(UniqueFd&& other) noexcept : value_(other.release()) {}

  UniqueFd& operator=(UniqueFd&& other) noexcept {
    if (this != &other) {
      reset(other.release());
    }
    return *this;
  }

  [[nodiscard]] int get() const noexcept { return value_; }
  [[nodiscard]] explicit operator bool() const noexcept { return value_ >= 0; }

  [[nodiscard]] int release() noexcept {
    const int value = value_;
    value_ = -1;
    return value;
  }

  void reset(int value = -1) noexcept {
    if (value_ >= 0) {
      ::close(value_);
    }
    value_ = value;
  }

 private:
  int value_;
};

}  // namespace ezcheats::platform
