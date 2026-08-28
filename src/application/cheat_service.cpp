#include "ezcheats/application/cheat_service.hpp"

#include <stdio.h>
#include <string.h>

namespace ezcheats::application {
namespace {

class MutexLock final {
 public:
  MutexLock(pthread_mutex_t& mutex, bool ready) noexcept
      : mutex_(mutex), locked_(ready && pthread_mutex_lock(&mutex_) == 0) {}
  ~MutexLock() {
    if (locked_) pthread_mutex_unlock(&mutex_);
  }
  explicit operator bool() const noexcept { return locked_; }

 private:
  pthread_mutex_t& mutex_;
  bool locked_;
};

bool same_game(const domain::GameContext& left,
               const domain::GameContext& right) noexcept {
  return left.pid == right.pid && left.app_id == right.app_id &&
         strcmp(left.title_id, right.title_id) == 0;
}

void copy_public_cheat(const domain::CheatEntry& source,
                       domain::CheatEntry& target) noexcept {
  target = source;
  target.patches = nullptr;
  target.patch_count = 0;
  target.patch_capacity = 0;
}

}  // namespace

CheatService::CheatService(domain::IGamePlatform& platform,
                           repository::FileCheatRepository& repository,
                           domain::IMemoryBackend& memory) noexcept
    : platform_(platform), repository_(repository), memory_(memory),
      applier_(platform, memory) {
  game_.pid = -1;
  game_.app_id = -1;
  mutex_ready_ = pthread_mutex_init(&mutex_, nullptr) == 0;
  if (!mutex_ready_) set_error_locked("service synchronization unavailable");
}

CheatService::~CheatService() {
  if (!mutex_ready_) return;
  {
    MutexLock lock(mutex_, true);
    domain::GameContext live{};
    if (lock && has_game_ && platform_.current_game(live) &&
        same_game(game_, live)) {
      disable_enabled_locked();
    }
    clear_locked();
  }
  pthread_mutex_destroy(&mutex_);
}

void CheatService::set_error_locked(const char* message) noexcept {
  snprintf(error_, sizeof(error_), "%s", message == nullptr ? "" : message);
}

void CheatService::clear_locked() noexcept {
  file_.clear();
  loaded_ = false;
}

bool CheatService::disable_enabled_locked() noexcept {
  bool success = true;
  domain::CheatFile& file = file_.get();
  for (size_t i = 0; i < file.cheat_count; ++i) {
    if (!file.cheats[i].enabled) continue;
    char status[160]{};
    if (applier_.set_enabled(game_, file, file.cheats[i].id, false, status,
                             sizeof(status)) != ApplyResult::success) {
      success = false;
      set_error_locked(status);
    }
  }
  return success;
}

bool CheatService::refresh_locked() noexcept {
  domain::GameContext live{};
  if (!platform_.current_game(live)) {
    clear_locked();
    has_game_ = false;
    game_ = {};
    game_.pid = -1;
    game_.app_id = -1;
    set_error_locked("");
    return true;
  }

  const bool identity_changed = !has_game_ || !same_game(game_, live);
  if (identity_changed) {
    // The previous PID is no longer confirmed live. Never write to it: it may
    // have exited or already have been reused by another process.
    clear_locked();
    game_ = live;
    has_game_ = true;
  } else {
    game_ = live;
  }

  const repository::RepositoryProbe probe = repository_.probe(game_);
  if (probe == repository::RepositoryProbe::unchanged) {
    if (identity_changed) {
      if (!repository_.load(game_, file_.get())) {
        set_error_locked("could not reload cheat file for new process");
        return false;
      }
      loaded_ = true;
    }
    set_error_locked("");
    return true;
  }
  if (probe == repository::RepositoryProbe::error) {
    set_error_locked("could not inspect cheat file");
    return false;
  }

  // Revert with the old parsed file before the repository replaces its
  // patches. If this is not possible, keep the old state intact.
  if (loaded_ && !disable_enabled_locked()) return false;

  const repository::ReloadResult result =
      repository_.reload_if_changed(game_, file_.get());
  if (result == repository::ReloadResult::loaded ||
      result == repository::ReloadResult::unchanged) {
    loaded_ = file_.get().cheats != nullptr;
    set_error_locked("");
    return true;
  }
  if (result == repository::ReloadResult::missing) {
    loaded_ = false;
    set_error_locked("");
    return true;
  }
  set_error_locked("could not parse cheat file");
  return false;
}

bool CheatService::refresh() {
  MutexLock lock(mutex_, mutex_ready_);
  return lock && refresh_locked();
}

bool CheatService::snapshot(domain::ServiceSnapshot& output) const {
  memset(&output, 0, sizeof(output));
  MutexLock lock(mutex_, mutex_ready_);
  if (!lock) return false;
  output.connected = true;
  snprintf(output.backend, sizeof(output.backend), "%s", memory_.name());
  output.has_game = has_game_;
  if (has_game_) output.game = game_;
  snprintf(output.error, sizeof(output.error), "%s", error_);
  if (!loaded_) return true;
  output.cheat_count = file_.get().cheat_count < domain::kMaxCheats
                           ? file_.get().cheat_count
                           : domain::kMaxCheats;
  for (size_t i = 0; i < output.cheat_count; ++i) {
    copy_public_cheat(file_.get().cheats[i], output.cheats[i]);
  }
  return true;
}

bool CheatService::set_enabled(uint32_t id, bool enabled,
                               domain::CheatEntry& updated) {
  memset(&updated, 0, sizeof(updated));
  MutexLock lock(mutex_, mutex_ready_);
  if (!lock || !refresh_locked() || !has_game_ || !loaded_) return false;
  char status[160]{};
  if (applier_.set_enabled(game_, file_.get(), id, enabled, status,
                           sizeof(status)) != ApplyResult::success) {
    set_error_locked(status);
    return false;
  }
  for (size_t i = 0; i < file_.get().cheat_count; ++i) {
    if (file_.get().cheats[i].id == id) {
      copy_public_cheat(file_.get().cheats[i], updated);
      set_error_locked("");
      return true;
    }
  }
  return false;
}

}  // namespace ezcheats::application
