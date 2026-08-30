#include "ezcheats/application/in_memory_cheat_service.hpp"

#include <stdio.h>
#include <string.h>

namespace ezcheats::application {
namespace {

void copy_text(char* output, size_t capacity, const char* input) {
  if (capacity == 0) {
    return;
  }
  snprintf(output, capacity, "%s", input);
}

void initialize_cheat(domain::CheatEntry& cheat, uint32_t id, const char* name,
                      const char* description, const char* author) {
  cheat.id = id;
  copy_text(cheat.name, sizeof(cheat.name), name);
  copy_text(cheat.description, sizeof(cheat.description), description);
  copy_text(cheat.author, sizeof(cheat.author), author);
  copy_text(cheat.module, sizeof(cheat.module), "eboot.bin");
}

}  // namespace

InMemoryCheatService::InMemoryCheatService() noexcept {
  game_.pid = 100;
  game_.app_id = 1;
  copy_text(game_.title_id, sizeof(game_.title_id), "CUSA00001");
  copy_text(game_.name, sizeof(game_.name), "Jogo de demonstração");
  copy_text(game_.version, sizeof(game_.version), "1.00");
  copy_text(game_.platform, sizeof(game_.platform), "ps4");
  copy_text(game_.process_name, sizeof(game_.process_name), "eboot.bin");

  initialize_cheat(cheats_[0], 0, "Vida infinita",
                   "Mantém a vida do personagem no valor máximo", "Blessing");
  initialize_cheat(cheats_[1], 1, "Munição infinita",
                   "Impede que a munição seja consumida", "Blessing");
  initialize_cheat(cheats_[2], 2, "Multiplicador de experiência",
                   "Aumenta a experiência recebida", "Blessing");
}

bool InMemoryCheatService::snapshot(domain::ServiceSnapshot& output) const {
  memset(&output, 0, sizeof(output));
  output.connected = true;
  snprintf(output.backend, sizeof(output.backend), "mock");
  output.has_game = true;
  output.game = game_;
  output.cheat_count = 3;
  for (size_t i = 0; i < output.cheat_count; ++i) {
    output.cheats[i] = cheats_[i];
    output.cheats[i].patches = nullptr;
    output.cheats[i].patch_count = 0;
    output.cheats[i].patch_capacity = 0;
  }
  return true;
}

bool InMemoryCheatService::set_enabled(uint32_t id, bool enabled,
                                       domain::CheatEntry& updated) {
  for (auto& cheat : cheats_) {
    if (cheat.id == id) {
      cheat.enabled = enabled;
      updated = cheat;
      return true;
    }
  }
  return false;
}

}  // namespace ezcheats::application
