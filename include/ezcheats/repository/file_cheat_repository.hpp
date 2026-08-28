#pragma once

#include <stddef.h>
#include <stdint.h>

#include "ezcheats/domain/interfaces.hpp"

#ifndef EZ_CHEATS_DIRECTORY
#define EZ_CHEATS_DIRECTORY "/data/ez-cheats/cheats"
#endif

namespace ezcheats::repository {

struct FileSignature {
  char path[512];
  uint64_t inode;
  uint64_t size;
  int64_t mtime_seconds;
  int64_t mtime_nanoseconds;
  int64_t ctime_seconds;
  int64_t ctime_nanoseconds;

  bool operator==(const FileSignature& other) const noexcept;
  bool operator!=(const FileSignature& other) const noexcept {
    return !(*this == other);
  }
};

enum class ReloadResult {
  loaded,
  unchanged,
  missing,
  error,
};

enum class RepositoryProbe {
  changed,
  unchanged,
  missing,
  error,
};

class FileCheatRepository final : public domain::ICheatRepository {
 public:
  explicit FileCheatRepository(
      const char* directory = EZ_CHEATS_DIRECTORY) noexcept;

  bool load(const domain::GameContext& game,
            domain::CheatFile& output) override;
  ReloadResult reload_if_changed(const domain::GameContext& game,
                                 domain::CheatFile& output);
  RepositoryProbe probe(const domain::GameContext& game) const noexcept;

  bool resolve_path(const domain::GameContext& game, char* output,
                    size_t output_size) const noexcept;
  bool ensure_directory() const noexcept;
  const char* directory() const noexcept { return directory_; }

  static bool normalize_version(const char* version, char* output,
                                size_t output_size) noexcept;
  static bool stat_signature(const char* path,
                             FileSignature& output) noexcept;

 private:
  bool load_path(const char* path, domain::CheatFile& output,
                 FileSignature& signature) const noexcept;
  void reset_signature() noexcept;

  char directory_[384];
  FileSignature signature_{};
  bool has_signature_ = false;
};

}  // namespace ezcheats::repository
