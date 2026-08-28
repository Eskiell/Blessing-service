#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ezcheats/domain/owned_cheat_file.hpp"
#include "ezcheats/repository/file_cheat_repository.hpp"

namespace {

using ezcheats::repository::FileCheatRepository;
using ezcheats::repository::ReloadResult;

constexpr const char* kJsonA =
    "{\"process\":\"eboot.bin\",\"name\":\"First\",\"mods\":[{"
    "\"name\":\"Infinite HP\",\"memory\":[{\"offset\":\"10\","
    "\"on\":\"AA\",\"off\":\"00\"}]}]}";
constexpr const char* kJsonB =
    "{\"process\":\"eboot.bin\",\"name\":\"Second\",\"mods\":[{"
    "\"name\":\"Infinite MP\",\"memory\":[{\"offset\":\"20\","
    "\"on\":\"BB\",\"off\":\"00\"}]}]}";

void write_file(const char* path, const char* contents) {
  FILE* file = fopen(path, "wb");
  assert(file != nullptr);
  const size_t size = strlen(contents);
  assert(fwrite(contents, 1, size, file) == size);
  assert(fclose(file) == 0);
}

void join(char* output, size_t output_size, const char* left,
          const char* right) {
  const int written = snprintf(output, output_size, "%s/%s", left, right);
  assert(written > 0 && static_cast<size_t>(written) < output_size);
}

ezcheats::domain::GameContext game(const char* title_id = "PPSA00001",
                                   const char* version = "01.002.003") {
  ezcheats::domain::GameContext result{};
  snprintf(result.title_id, sizeof(result.title_id), "%s", title_id);
  snprintf(result.version, sizeof(result.version), "%s", version);
  return result;
}

void tests_format_order_and_safe_paths(const char* directory) {
  FileCheatRepository repository(directory);
  FileCheatRepository relative("../escape");
  assert(relative.directory()[0] == '\0');
  char json[512], shn[512], mc4[512], shnext[512], resolved[512];
  join(json, sizeof(json), directory, "PPSA00001_01.002.003.json");
  join(shn, sizeof(shn), directory, "PPSA00001_01.002.003.shn");
  join(mc4, sizeof(mc4), directory, "PPSA00001_01.002.003.mc4");
  join(shnext, sizeof(shnext), directory, "PPSA00001_01.002.003.ShnExt");
  write_file(shnext, "x");
  write_file(mc4, "x");
  write_file(shn, "x");
  write_file(json, "x");

  assert(repository.resolve_path(game(), resolved, sizeof(resolved)));
  assert(strcmp(resolved, json) == 0);
  assert(unlink(json) == 0);
  assert(repository.resolve_path(game(), resolved, sizeof(resolved)));
  assert(strcmp(resolved, shn) == 0);
  assert(unlink(shn) == 0);
  assert(repository.resolve_path(game(), resolved, sizeof(resolved)));
  assert(strcmp(resolved, mc4) == 0);
  assert(unlink(mc4) == 0);
  assert(repository.resolve_path(game(), resolved, sizeof(resolved)));
  assert(strcmp(resolved, shnext) == 0);

  assert(!repository.resolve_path(game("../escape"), resolved,
                                  sizeof(resolved)));
  char normalized[32];
  assert(FileCheatRepository::normalize_version("1.0 (beta)", normalized,
                                                sizeof(normalized)));
  assert(strcmp(normalized, "1.0__beta_") == 0);
  assert(!FileCheatRepository::normalize_version("unknown", normalized,
                                                 sizeof(normalized)));
  assert(unlink(shnext) == 0);

  assert(symlink("/etc/hosts", json) == 0);
  assert(!repository.resolve_path(game(), resolved, sizeof(resolved)));
  assert(unlink(json) == 0);
}

void tests_loading_and_hot_reload(const char* directory) {
  FileCheatRepository repository(directory);
  char path[512], replacement[512];
  join(path, sizeof(path), directory, "PPSA00001_01.002.003.json");
  join(replacement, sizeof(replacement), directory, "replacement.json");
  write_file(path, kJsonA);

  ezcheats::domain::OwnedCheatFile owned;
  assert(repository.reload_if_changed(game(), owned.get()) ==
         ReloadResult::loaded);
  assert(strcmp(owned.get().name, "First") == 0);
  assert(repository.reload_if_changed(game(), owned.get()) ==
         ReloadResult::unchanged);

  write_file(replacement, kJsonB);
  assert(rename(replacement, path) == 0);
  assert(repository.reload_if_changed(game(), owned.get()) ==
         ReloadResult::loaded);
  assert(strcmp(owned.get().name, "Second") == 0);
  assert(strcmp(owned.get().cheats[0].name, "Infinite MP") == 0);

  write_file(replacement, "invalid");
  assert(rename(replacement, path) == 0);
  assert(repository.reload_if_changed(game(), owned.get()) ==
         ReloadResult::error);
  assert(strcmp(owned.get().name, "Second") == 0);

  assert(unlink(path) == 0);
  assert(repository.reload_if_changed(game(), owned.get()) ==
         ReloadResult::missing);
  assert(owned.get().cheats == nullptr);
}

}  // namespace

int main() {
  char temporary[] = "/tmp/ez-cheats-repository-XXXXXX";
  char* root = mkdtemp(temporary);
  assert(root != nullptr);
  char directory[512];
  join(directory, sizeof(directory), root, "cheats");

  FileCheatRepository repository(directory);
  assert(repository.ensure_directory());
  tests_format_order_and_safe_paths(directory);
  tests_loading_and_hot_reload(directory);

  assert(rmdir(directory) == 0);
  assert(rmdir(root) == 0);
}
