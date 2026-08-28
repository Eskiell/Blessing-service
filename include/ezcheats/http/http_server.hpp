#pragma once

#include <stdint.h>

namespace ezcheats::http {

class HttpServer final {
 public:
  explicit HttpServer(uint16_t port) noexcept;

  HttpServer(const HttpServer&) = delete;
  HttpServer& operator=(const HttpServer&) = delete;

  int run();

 private:
  uint16_t port_;
};

}  // namespace ezcheats::http
