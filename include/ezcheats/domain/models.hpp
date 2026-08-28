#pragma once

#include <stddef.h>
#include <stdint.h>

namespace ezcheats::domain {

constexpr size_t kMaxPatchBytes = 1024;
constexpr size_t kMaxCheats = 64;

struct Patch {
  uint64_t offset;
  int section;
  bool absolute;
  bool assembly;
  size_t enable_size;
  size_t disable_size;
  uint8_t enable[kMaxPatchBytes];
  uint8_t disable[kMaxPatchBytes];
};

struct CheatEntry {
  uint32_t id;
  char name[128];
  char description[256];
  char author[64];
  char module[128];
  bool enabled;
  Patch* patches;
  size_t patch_count;
};

struct CheatFile {
  char name[128];
  char process[128];
  CheatEntry* cheats;
  size_t cheat_count;
};

struct GameContext {
  int pid;
  int app_id;
  char title_id[16];
  char name[128];
  char version[32];
  char platform[16];
  char process_name[64];
};

struct ServiceState {
  bool connected;
  const char* backend;
  const GameContext* game;
  const CheatEntry* cheats;
  size_t cheat_count;
};

}  // namespace ezcheats::domain
