#include <signal.h>
#include <stdint.h>

#include "ezcheats/http/http_server.hpp"

#ifndef EZ_CHEATS_HTTP_PORT
#define EZ_CHEATS_HTTP_PORT 5911
#endif

int main() {
  signal(SIGPIPE, SIG_IGN);
  ezcheats::http::HttpServer server{
      static_cast<uint16_t>(EZ_CHEATS_HTTP_PORT)};
  return server.run();
}
