#include "ezcheats/domain/owned_cheat_file.hpp"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

namespace ezcheats::domain {

void secure_zero(void* data, size_t size) noexcept {
  auto* bytes = static_cast<volatile unsigned char*>(data);
  if (bytes == nullptr) return;
  while (size-- > 0) *bytes++ = 0;
}

void clear_cheat_file(CheatFile& file) noexcept {
  for (size_t i = 0; file.cheats != nullptr && i < file.cheat_capacity; ++i) {
    if (file.cheats[i].patches != nullptr) {
      secure_zero(file.cheats[i].patches,
                  file.cheats[i].patch_capacity * sizeof(Patch));
      free(file.cheats[i].patches);
    }
  }
  if (file.cheats != nullptr) {
    secure_zero(file.cheats, file.cheat_capacity * sizeof(CheatEntry));
    free(file.cheats);
  }
  memset(&file, 0, sizeof(file));
  file.master_code_id = -1;
}

bool ensure_cheat(CheatFile& file) noexcept {
  if (file.cheat_count < file.cheat_capacity) return true;
  if (file.cheat_capacity >= kMaxCheats) return false;
  size_t capacity = file.cheat_capacity == 0 ? 4 : file.cheat_capacity * 2;
  if (capacity > kMaxCheats) capacity = kMaxCheats;
  void* resized = realloc(file.cheats, capacity * sizeof(CheatEntry));
  if (resized == nullptr) return false;
  file.cheats = static_cast<CheatEntry*>(resized);
  memset(file.cheats + file.cheat_capacity, 0,
         (capacity - file.cheat_capacity) * sizeof(CheatEntry));
  file.cheat_capacity = capacity;
  return true;
}

bool ensure_patch(CheatEntry& cheat) noexcept {
  if (cheat.patch_count < cheat.patch_capacity) return true;
  if (cheat.patch_capacity >= kMaxPatchesPerCheat) return false;
  size_t capacity = cheat.patch_capacity == 0 ? 4 : cheat.patch_capacity * 2;
  if (capacity > kMaxPatchesPerCheat) capacity = kMaxPatchesPerCheat;
  void* resized = realloc(cheat.patches, capacity * sizeof(Patch));
  if (resized == nullptr) return false;
  cheat.patches = static_cast<Patch*>(resized);
  memset(cheat.patches + cheat.patch_capacity, 0,
         (capacity - cheat.patch_capacity) * sizeof(Patch));
  cheat.patch_capacity = capacity;
  return true;
}

bool add_author(CheatFile& file, const char* author) noexcept {
  if (author == nullptr || author[0] == '\0') return false;
  for (size_t i = 0; i < file.author_count; ++i) {
    if (strcmp(file.authors[i], author) == 0) return true;
  }
  if (file.author_count >= kMaxAuthors) return false;
  snprintf(file.authors[file.author_count], kAuthorNameSize, "%s", author);
  ++file.author_count;
  return true;
}

}  // namespace ezcheats::domain
