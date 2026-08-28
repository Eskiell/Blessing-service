#include "ezcheats/repository/file_cheat_repository.hpp"

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ezcheats/domain/owned_cheat_file.hpp"
#include "ezcheats/parsers/cheat_parser_factory.hpp"

#ifndef O_NOFOLLOW
#define O_NOFOLLOW 0
#endif

namespace ezcheats::repository {
namespace {

constexpr size_t kMaxCheatFileSize = 16 * 1024 * 1024;
constexpr const char* kExtensions[] = {"json", "shn", "mc4", "ShnExt"};

bool valid_title_id(const char* title_id) noexcept {
  if (title_id == nullptr || title_id[0] == '\0') return false;
  for (size_t i = 0; title_id[i] != '\0'; ++i) {
    const unsigned char value = static_cast<unsigned char>(title_id[i]);
    if (!isalnum(value) && value != '-' && value != '_') return false;
  }
  return true;
}

bool regular_file_signature(const char* path, const struct stat& status,
                            FileSignature& output) noexcept {
  if (path == nullptr || !S_ISREG(status.st_mode) || status.st_size <= 0) {
    return false;
  }
  const int written = snprintf(output.path, sizeof(output.path), "%s", path);
  if (written < 0 || static_cast<size_t>(written) >= sizeof(output.path)) {
    return false;
  }
  output.inode = static_cast<uint64_t>(status.st_ino);
  output.size = static_cast<uint64_t>(status.st_size);
#if defined(__APPLE__)
  output.mtime_seconds = status.st_mtimespec.tv_sec;
  output.mtime_nanoseconds = status.st_mtimespec.tv_nsec;
  output.ctime_seconds = status.st_ctimespec.tv_sec;
  output.ctime_nanoseconds = status.st_ctimespec.tv_nsec;
#else
  output.mtime_seconds = status.st_mtim.tv_sec;
  output.mtime_nanoseconds = status.st_mtim.tv_nsec;
  output.ctime_seconds = status.st_ctim.tv_sec;
  output.ctime_nanoseconds = status.st_ctim.tv_nsec;
#endif
  return true;
}

}  // namespace

bool FileSignature::operator==(const FileSignature& other) const noexcept {
  return strcmp(path, other.path) == 0 && inode == other.inode &&
         size == other.size && mtime_seconds == other.mtime_seconds &&
         mtime_nanoseconds == other.mtime_nanoseconds &&
         ctime_seconds == other.ctime_seconds &&
         ctime_nanoseconds == other.ctime_nanoseconds;
}

FileCheatRepository::FileCheatRepository(const char* directory) noexcept {
  if (directory == nullptr || directory[0] != '/') {
    directory_[0] = '\0';
    return;
  }
  const int copied = snprintf(directory_, sizeof(directory_), "%s", directory);
  if (copied < 0 || static_cast<size_t>(copied) >= sizeof(directory_)) {
    directory_[0] = '\0';
    return;
  }
  const size_t length = strlen(directory_);
  if (length == 0) {
    directory_[0] = '\0';
  } else if (length > 1 && directory_[length - 1] == '/') {
    directory_[length - 1] = '\0';
  }
}

bool FileCheatRepository::normalize_version(const char* version, char* output,
                                            size_t output_size) noexcept {
  if (output == nullptr || output_size == 0) return false;
  output[0] = '\0';
  if (version == nullptr || version[0] == '\0' ||
      strcmp(version, "unknown") == 0) {
    return false;
  }
  size_t written = 0;
  for (size_t i = 0; version[i] != '\0'; ++i) {
    if (written + 1 >= output_size) {
      output[0] = '\0';
      return false;
    }
    const unsigned char value = static_cast<unsigned char>(version[i]);
    output[written++] =
        (isalnum(value) || value == '.' || value == '_' || value == '-')
            ? static_cast<char>(value)
            : '_';
  }
  output[written] = '\0';
  return written > 0;
}

bool FileCheatRepository::stat_signature(const char* path,
                                         FileSignature& output) noexcept {
  struct stat status {};
  memset(&output, 0, sizeof(output));
  return path != nullptr && lstat(path, &status) == 0 &&
         regular_file_signature(path, status, output);
}

bool FileCheatRepository::resolve_path(const domain::GameContext& game,
                                       char* output,
                                       size_t output_size) const noexcept {
  if (output == nullptr || output_size == 0) return false;
  output[0] = '\0';
  if (directory_[0] == '\0' || !valid_title_id(game.title_id)) return false;

  char version[sizeof(game.version)]{};
  if (!normalize_version(game.version, version, sizeof(version))) return false;

  for (const char* extension : kExtensions) {
    char candidate[512];
    const int written = snprintf(candidate, sizeof(candidate), "%s/%s_%s.%s",
                                 directory_, game.title_id, version, extension);
    if (written < 0 || static_cast<size_t>(written) >= sizeof(candidate)) {
      return false;
    }
    FileSignature ignored{};
    if (!stat_signature(candidate, ignored)) continue;
    const int copied = snprintf(output, output_size, "%s", candidate);
    return copied >= 0 && static_cast<size_t>(copied) < output_size;
  }
  return false;
}

bool FileCheatRepository::load_path(const char* path,
                                    domain::CheatFile& output,
                                    FileSignature& signature) const noexcept {
  domain::ICheatParser* parser =
      parsers::CheatParserFactory::parser_for_path(path);
  if (parser == nullptr) return false;

  const int descriptor = open(path, O_RDONLY | O_NOFOLLOW);
  if (descriptor < 0) return false;
  struct stat status {};
  if (fstat(descriptor, &status) != 0 ||
      !regular_file_signature(path, status, signature) ||
      signature.size > kMaxCheatFileSize) {
    close(descriptor);
    return false;
  }

  auto* data = static_cast<uint8_t*>(malloc(signature.size));
  if (data == nullptr) {
    close(descriptor);
    return false;
  }
  size_t total = 0;
  while (total < signature.size) {
    const ssize_t count =
        read(descriptor, data + total, signature.size - total);
    if (count <= 0) break;
    total += static_cast<size_t>(count);
  }
  close(descriptor);

  domain::CheatFile parsed{};
  parsed.master_code_id = -1;
  const bool loaded = total == signature.size &&
                      parser->parse(data, total, parsed);
  domain::secure_zero(data, signature.size);
  free(data);
  if (!loaded) {
    domain::clear_cheat_file(parsed);
    return false;
  }

  domain::clear_cheat_file(output);
  output = parsed;
  return true;
}

bool FileCheatRepository::load(const domain::GameContext& game,
                               domain::CheatFile& output) {
  char path[512];
  FileSignature signature{};
  if (!resolve_path(game, path, sizeof(path)) ||
      !load_path(path, output, signature)) {
    domain::clear_cheat_file(output);
    reset_signature();
    return false;
  }
  signature_ = signature;
  has_signature_ = true;
  return true;
}

ReloadResult FileCheatRepository::reload_if_changed(
    const domain::GameContext& game, domain::CheatFile& output) {
  char path[512];
  if (!resolve_path(game, path, sizeof(path))) {
    domain::clear_cheat_file(output);
    reset_signature();
    return ReloadResult::missing;
  }

  FileSignature current{};
  if (!stat_signature(path, current)) return ReloadResult::error;
  if (has_signature_ && current == signature_) return ReloadResult::unchanged;

  FileSignature loaded{};
  if (!load_path(path, output, loaded)) return ReloadResult::error;
  signature_ = loaded;
  has_signature_ = true;
  return ReloadResult::loaded;
}

bool FileCheatRepository::ensure_directory() const noexcept {
  if (directory_[0] == '\0') return false;
  if (strcmp(directory_, EZ_CHEATS_DIRECTORY) == 0) {
    if (mkdir("/data/ez-cheats", 0777) != 0) {
      struct stat status {};
      if (stat("/data/ez-cheats", &status) != 0 ||
          !S_ISDIR(status.st_mode)) {
        return false;
      }
    }
  }
  if (mkdir(directory_, 0777) == 0) return true;
  struct stat status {};
  return stat(directory_, &status) == 0 && S_ISDIR(status.st_mode);
}

void FileCheatRepository::reset_signature() noexcept {
  memset(&signature_, 0, sizeof(signature_));
  has_signature_ = false;
}

}  // namespace ezcheats::repository
