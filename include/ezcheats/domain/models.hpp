#pragma once

#include <stddef.h>
#include <stdint.h>

namespace ezcheats::domain {

constexpr size_t kMaxPatchBytes = 1024;
constexpr size_t kMaxCheats = 64;
constexpr size_t kMaxPatchesPerCheat = 256;
constexpr size_t kMaxAuthors = 16;
constexpr size_t kAuthorNameSize = 64;

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
  size_t patch_capacity;
};

struct CheatFile {
  char name[128];
  char process[128];
  char authors[kMaxAuthors][kAuthorNameSize];
  size_t author_count;
  CheatEntry* cheats;
  size_t cheat_count;
  size_t cheat_capacity;
  int master_code_id;
  int last_applied_pid;
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

constexpr size_t kMaxModuleSections = 4;

struct ModuleSection {
  uint64_t address;
  uint64_t size;
  uint32_t protection;
};

struct ModuleInfo {
  char name[128];
  char path[1024];
  uint64_t handle;
  ModuleSection sections[kMaxModuleSections];
  size_t section_count;
};

struct ServiceSnapshot {
  bool connected;
  char backend[32];
  bool has_game;
  GameContext game;
  CheatEntry cheats[kMaxCheats];
  size_t cheat_count;
  char error[160];
};

}  // namespace ezcheats::domain
