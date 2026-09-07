#include "ezcheats/overlay/overlay_session.hpp"

namespace ezcheats::overlay {

void OverlaySession::open(const domain::ServiceSnapshot& snapshot) noexcept {
  visible_ = true;
  replace_snapshot(snapshot);
}

void OverlaySession::close() noexcept { visible_ = false; }

void OverlaySession::update(const domain::ServiceSnapshot& snapshot) noexcept {
  replace_snapshot(snapshot);
}

void OverlaySession::select_next() noexcept {
  if (snapshot_.cheat_count == 0) return;
  selected_index_ = (selected_index_ + 1) % snapshot_.cheat_count;
}

void OverlaySession::select_previous() noexcept {
  if (snapshot_.cheat_count == 0) return;
  selected_index_ = selected_index_ == 0 ? snapshot_.cheat_count - 1
                                         : selected_index_ - 1;
}

const domain::CheatEntry* OverlaySession::selected_cheat() const noexcept {
  if (snapshot_.cheat_count == 0 ||
      selected_index_ >= snapshot_.cheat_count) {
    return nullptr;
  }
  return &snapshot_.cheats[selected_index_];
}

void OverlaySession::replace_snapshot(
    const domain::ServiceSnapshot& snapshot) noexcept {
  const domain::CheatEntry* selected = selected_cheat();
  const uint32_t selected_id = selected == nullptr ? 0 : selected->id;
  const bool preserve_selection = selected != nullptr;

  snapshot_ = snapshot;
  if (snapshot_.cheat_count > domain::kMaxCheats) {
    snapshot_.cheat_count = domain::kMaxCheats;
  }

  selected_index_ = 0;
  if (!preserve_selection) return;
  for (size_t index = 0; index < snapshot_.cheat_count; ++index) {
    if (snapshot_.cheats[index].id == selected_id) {
      selected_index_ = index;
      return;
    }
  }
}

}  // namespace ezcheats::overlay
