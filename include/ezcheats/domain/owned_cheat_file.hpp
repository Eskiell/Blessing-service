#pragma once

#include "ezcheats/domain/models.hpp"

namespace ezcheats::domain {

void clear_cheat_file(CheatFile& file) noexcept;
bool ensure_cheat(CheatFile& file) noexcept;
bool ensure_patch(CheatEntry& cheat) noexcept;
bool add_author(CheatFile& file, const char* author) noexcept;
void secure_zero(void* data, size_t size) noexcept;

class OwnedCheatFile final {
 public:
  OwnedCheatFile() noexcept { file_.master_code_id = -1; }
  ~OwnedCheatFile() { clear_cheat_file(file_); }

  OwnedCheatFile(const OwnedCheatFile&) = delete;
  OwnedCheatFile& operator=(const OwnedCheatFile&) = delete;

  CheatFile& get() noexcept { return file_; }
  const CheatFile& get() const noexcept { return file_; }
  void clear() noexcept { clear_cheat_file(file_); }

 private:
  CheatFile file_{};
};

}  // namespace ezcheats::domain
