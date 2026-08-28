#include <signal.h>
#include <stdint.h>

#include "ezcheats/application/cheat_service.hpp"
#include "ezcheats/http/http_server.hpp"
#include "ezcheats/memory/memory_backend_factory.hpp"
#include "ezcheats/platform/ps5_game_platform.hpp"
#include "ezcheats/repository/file_cheat_repository.hpp"

#ifndef EZ_CHEATS_HTTP_PORT
#define EZ_CHEATS_HTTP_PORT 5911
#endif

int main() {
  signal(SIGPIPE, SIG_IGN);
  ezcheats::platform::Ps5GamePlatform platform;
  ezcheats::repository::FileCheatRepository repository;
  if (!repository.ensure_directory()) return 1;
  ezcheats::domain::IMemoryBackend* memory =
      ezcheats::memory::MemoryBackendFactory::create(
          ezcheats::memory::MemoryBackendKind::automatic, 0);
  if (memory == nullptr) return 1;
  ezcheats::application::CheatService cheat_service{platform, repository,
                                                     *memory};
  ezcheats::http::HttpServer server{static_cast<uint16_t>(EZ_CHEATS_HTTP_PORT),
                                    cheat_service};
  return server.run();
}
