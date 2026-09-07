#include <cassert>

#include "ezcheats/overlay/overlay_session.hpp"

namespace {

ezcheats::domain::ServiceSnapshot snapshot_with(size_t cheat_count) {
  ezcheats::domain::ServiceSnapshot snapshot{};
  snapshot.connected = true;
  snapshot.has_game = true;
  snapshot.cheat_count = cheat_count;
  for (size_t index = 0; index < cheat_count; ++index) {
    snapshot.cheats[index].id = static_cast<uint32_t>(10 + index);
  }
  return snapshot;
}

}  // namespace

int main() {
  ezcheats::overlay::OverlaySession session;
  assert(!session.visible());
  assert(session.selected_cheat() == nullptr);

  auto snapshot = snapshot_with(3);
  session.open(snapshot);
  assert(session.visible());
  assert(session.selected_cheat()->id == 10);

  session.select_next();
  assert(session.selected_cheat()->id == 11);
  session.select_next();
  assert(session.selected_cheat()->id == 12);
  session.select_next();
  assert(session.selected_cheat()->id == 10);
  session.select_previous();
  assert(session.selected_cheat()->id == 12);

  auto reordered = snapshot_with(3);
  reordered.cheats[0].id = 12;
  reordered.cheats[1].id = 10;
  reordered.cheats[2].id = 11;
  session.update(reordered);
  assert(session.selected_cheat()->id == 12);

  auto missing = snapshot_with(1);
  missing.cheats[0].id = 99;
  session.update(missing);
  assert(session.selected_cheat()->id == 99);

  session.update(snapshot_with(0));
  assert(session.selected_cheat() == nullptr);
  session.select_next();
  session.select_previous();

  session.close();
  assert(!session.visible());
}
