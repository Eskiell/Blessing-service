#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ezcheats/application/cheat_service.hpp"
#include "ezcheats/memory/fake_memory_backend.hpp"
#include "ezcheats/platform/fake_game_platform.hpp"

namespace {

constexpr const char* kFirst =
    "{\"process\":\"eboot.bin\",\"name\":\"First\",\"mods\":[{"
    "\"name\":\"Infinite HP\",\"memory\":[{\"offset\":\"10\","
    "\"on\":\"AA\",\"off\":\"00\"}]}]}";
constexpr const char* kSecond =
    "{\"process\":\"eboot.bin\",\"name\":\"Second\",\"mods\":[{"
    "\"name\":\"Infinite MP\",\"memory\":[{\"offset\":\"20\","
    "\"on\":\"BB\",\"off\":\"00\"}]}]}";

void write_file(const char* path, const char* contents) {
  FILE* file = fopen(path, "wb");
  assert(file != nullptr);
  assert(fwrite(contents, 1, strlen(contents), file) == strlen(contents));
  assert(fclose(file) == 0);
}

ezcheats::domain::GameContext game(int pid = 42) {
  ezcheats::domain::GameContext result{};
  result.pid = pid;
  result.app_id = 7;
  snprintf(result.title_id, sizeof(result.title_id), "PPSA00001");
  snprintf(result.name, sizeof(result.name), "Test Game");
  snprintf(result.version, sizeof(result.version), "1.00");
  snprintf(result.platform, sizeof(result.platform), "ps5");
  snprintf(result.process_name, sizeof(result.process_name), "eboot.bin");
  return result;
}

ezcheats::domain::ModuleInfo module() {
  ezcheats::domain::ModuleInfo result{};
  snprintf(result.name, sizeof(result.name), "eboot.bin");
  result.section_count = 1;
  result.sections[0].address =
      ezcheats::memory::FakeMemoryBackend::kBaseAddress;
  result.sections[0].size =
      ezcheats::memory::FakeMemoryBackend::kMemorySize;
  return result;
}

struct ToggleJob {
  ezcheats::application::CheatService* service;
  bool success;
};

void* enable_repeatedly(void* raw) {
  auto* job = static_cast<ToggleJob*>(raw);
  job->success = true;
  for (size_t i = 0; i < 100; ++i) {
    ezcheats::domain::CheatEntry updated{};
    if (!job->service->set_enabled(0, true, updated) || !updated.enabled) {
      job->success = false;
      break;
    }
  }
  return nullptr;
}

}  // namespace

int main() {
  char temporary[] = "/tmp/ez-cheats-service-XXXXXX";
  char* directory = mkdtemp(temporary);
  assert(directory != nullptr);
  char path[512], replacement[512];
  snprintf(path, sizeof(path), "%s/PPSA00001_1.00.json", directory);
  snprintf(replacement, sizeof(replacement), "%s/replacement.json", directory);
  write_file(path, kFirst);

  ezcheats::platform::FakeGamePlatform platform;
  ezcheats::memory::FakeMemoryBackend memory;
  ezcheats::repository::FileCheatRepository repository(directory);
  const auto first_game = game();
  platform.set_game(first_game);
  assert(platform.add_module(first_game.pid, first_game.app_id, module()));

  ezcheats::application::CheatService service(platform, repository, memory);
  assert(service.refresh());
  ezcheats::domain::ServiceSnapshot snapshot{};
  assert(service.snapshot(snapshot));
  assert(snapshot.connected && snapshot.has_game);
  assert(snapshot.cheat_count == 1);
  assert(snapshot.cheats[0].patches == nullptr);
  assert(strcmp(snapshot.cheats[0].name, "Infinite HP") == 0);

  pthread_t threads[4]{};
  ToggleJob jobs[4]{};
  for (size_t i = 0; i < 4; ++i) {
    jobs[i] = {&service, false};
    assert(pthread_create(&threads[i], nullptr, enable_repeatedly, &jobs[i]) ==
           0);
  }
  for (size_t i = 0; i < 4; ++i) {
    assert(pthread_join(threads[i], nullptr) == 0);
    assert(jobs[i].success);
  }
  uint8_t byte = 0;
  assert(memory.read(first_game.pid,
                     ezcheats::memory::FakeMemoryBackend::kBaseAddress + 0x10,
                     &byte, 1));
  assert(byte == 0xaa);

  write_file(replacement, kSecond);
  assert(rename(replacement, path) == 0);
  assert(service.refresh());
  assert(memory.read(first_game.pid,
                     ezcheats::memory::FakeMemoryBackend::kBaseAddress + 0x10,
                     &byte, 1));
  assert(byte == 0x00);
  assert(service.snapshot(snapshot));
  assert(snapshot.cheat_count == 1 && !snapshot.cheats[0].enabled);
  assert(strcmp(snapshot.cheats[0].name, "Infinite MP") == 0);

  const auto second_process = game(84);
  platform.set_game(second_process);
  assert(platform.add_module(second_process.pid, second_process.app_id,
                             module()));
  assert(service.refresh());
  assert(service.snapshot(snapshot));
  assert(snapshot.game.pid == second_process.pid);
  assert(snapshot.cheat_count == 1 && !snapshot.cheats[0].enabled);

  ezcheats::domain::CheatEntry updated{};
  assert(service.set_enabled(0, true, updated));
  write_file(replacement, "invalid");
  assert(rename(replacement, path) == 0);
  assert(!service.refresh());
  assert(service.snapshot(snapshot));
  assert(snapshot.cheat_count == 1);
  assert(!snapshot.cheats[0].enabled);
  assert(snapshot.error[0] != '\0');

  // An absent process is normal. Its old PID is not touched because it may
  // already be gone or reused; only the in-memory service state is discarded.
  platform.clear_game();
  assert(service.refresh());
  assert(service.snapshot(snapshot));
  assert(snapshot.connected && !snapshot.has_game && snapshot.cheat_count == 0);

  assert(unlink(path) == 0);
  assert(rmdir(directory) == 0);
}
