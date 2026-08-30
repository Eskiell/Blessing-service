#include <signal.h>
#include <stdio.h>
#include <stdint.h>

#include "ezcheats/application/cheat_service.hpp"
#include "ezcheats/http/http_server.hpp"
#include "ezcheats/memory/memory_backend_factory.hpp"
#include "ezcheats/platform/ps5_game_platform.hpp"
#include "ezcheats/platform/ps5_media_tile.hpp"
#include "ezcheats/repository/file_cheat_repository.hpp"

#ifndef EZ_CHEATS_HTTP_PORT
#define EZ_CHEATS_HTTP_PORT 5911
#endif
#ifndef EZ_CHEATS_MEMORY_BACKEND
#define EZ_CHEATS_MEMORY_BACKEND 0
#endif

int main() {
  signal(SIGPIPE, SIG_IGN);
  ezcheats::platform::Ps5GamePlatform platform;
  ezcheats::repository::FileCheatRepository repository;
  if (!repository.ensure_directory()) {
    printf("Blessing: could not create cheat directory %s\n",
           repository.directory());
    return 1;
  }
  const auto tile_result = ezcheats::platform::install_media_tile_if_needed();
  const char* tile_status = "failed";
  if (tile_result == ezcheats::platform::MediaTileResult::installed) {
    tile_status = "installed";
  } else if (tile_result == ezcheats::platform::MediaTileResult::current) {
    tile_status = "current";
  }
  printf("Blessing: media tile=%s\n", tile_status);
  constexpr auto requested_backend =
      static_cast<ezcheats::memory::MemoryBackendKind>(
          EZ_CHEATS_MEMORY_BACKEND);
  const uint32_t firmware =
      ezcheats::memory::MemoryBackendFactory::detect_firmware_major();
  ezcheats::domain::IMemoryBackend* memory =
      ezcheats::memory::MemoryBackendFactory::create(
          requested_backend, firmware);
  if (memory == nullptr) return 1;
  printf("Blessing: directory=%s backend=%s firmware=0x%x\n",
         repository.directory(), memory->name(), firmware);
  ezcheats::application::CheatService cheat_service{platform, repository,
                                                     *memory};
  ezcheats::http::HttpServer server{static_cast<uint16_t>(EZ_CHEATS_HTTP_PORT),
                                    cheat_service};
  return server.run();
}
