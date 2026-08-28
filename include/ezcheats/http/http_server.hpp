#pragma once

#include <stdint.h>

#include "ezcheats/domain/interfaces.hpp"

namespace ezcheats::http {

class HttpServer final {
 public:
  HttpServer(uint16_t port, domain::ICheatService& cheat_service) noexcept;

  HttpServer(const HttpServer&) = delete;
  HttpServer& operator=(const HttpServer&) = delete;

  int run();

 private:
  uint16_t port_;
  domain::ICheatService& cheat_service_;
};

}  // namespace ezcheats::http
