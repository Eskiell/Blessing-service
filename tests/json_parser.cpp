#include <assert.h>
#include <string.h>

#include "ezcheats/domain/owned_cheat_file.hpp"
#include "ezcheats/parsers/cheat_parser_factory.hpp"

namespace {

void parse_buffer_success() {
  constexpr const char* json =
      "{\"process\":\"eboot.bin\",\"name\":\"Demo\","
      "\"mods\":[{\"name\":\"Infinite HP\",\"description\":\"Lock HP\","
      "\"memory\":[{\"offset\":\"1000\",\"on\":\"AABBCCDD\","
      "\"off\":\"11223344\",\"section\":1,\"absolute\":true}]}],"
      "\"credits\":[\"Alice\",\"Bob\"],\"authors\":[\"Alice\",\"Carol\"]}";
  ezcheats::domain::OwnedCheatFile owned;
  auto& file = owned.get();
  assert(ezcheats::parsers::CheatParserFactory::load_buffer(
      ".JSON", reinterpret_cast<const uint8_t*>(json), strlen(json), file));
  assert(strcmp(file.process, "eboot.bin") == 0);
  assert(strcmp(file.name, "Demo") == 0);
  assert(file.author_count == 3);
  assert(strcmp(file.authors[0], "Alice") == 0);
  assert(strcmp(file.authors[2], "Carol") == 0);
  assert(file.cheat_count == 1);
  assert(strcmp(file.cheats[0].name, "Infinite HP") == 0);
  assert(strcmp(file.cheats[0].module, "eboot.bin") == 0);
  assert(file.cheats[0].patch_count == 1);
  const auto& patch = file.cheats[0].patches[0];
  assert(patch.offset == 0x1000);
  assert(patch.section == 1);
  assert(patch.absolute);
  assert(patch.enable_size == 4 && patch.enable[0] == 0xaa &&
         patch.enable[3] == 0xdd);
  assert(patch.disable_size == 4 && patch.disable[0] == 0x11 &&
         patch.disable[3] == 0x44);
}

void rejects_invalid_input() {
  constexpr const char* invalid_hex =
      "{\"process\":\"eboot.bin\",\"name\":\"Demo\",\"mods\":[{"
      "\"name\":\"Broken\",\"memory\":[{\"offset\":\"10\","
      "\"on\":\"ABG\",\"off\":\"00\"}]}]}";
  ezcheats::domain::OwnedCheatFile owned;
  auto& file = owned.get();
  assert(!ezcheats::parsers::CheatParserFactory::load_buffer(
      "json", reinterpret_cast<const uint8_t*>(invalid_hex),
      strlen(invalid_hex), file));
  assert(file.cheats == nullptr && file.cheat_count == 0);
  assert(!ezcheats::parsers::CheatParserFactory::load_buffer(
      "unknown", reinterpret_cast<const uint8_t*>(invalid_hex),
      strlen(invalid_hex), file));
}

void accepts_bounded_non_terminated_buffer() {
  constexpr char json[] =
      "{\"process\":\"eboot.bin\",\"name\":\"Bounded\",\"mods\":[{"
      "\"name\":\"Braces\",\"description\":\"text with } and \\\"quote\\\"\","
      "\"memory\":[{\"offset\":\"1\",\"on\":\"A\",\"off\":\"0B\"}]}]}";
  char bounded[sizeof(json) - 1]{};
  memcpy(bounded, json, sizeof(bounded));
  ezcheats::domain::OwnedCheatFile owned;
  assert(ezcheats::parsers::CheatParserFactory::load_buffer(
      "json", reinterpret_cast<const uint8_t*>(bounded), sizeof(bounded),
      owned.get()));
  assert(owned.get().cheat_count == 1);
  assert(owned.get().cheats[0].patches[0].enable[0] == 0x0a);
}

void loads_real_fixture() {
  ezcheats::domain::OwnedCheatFile owned;
  auto& file = owned.get();
  assert(ezcheats::parsers::CheatParserFactory::load_file(
      "tests/fixtures/PPSA26344_01.008.000.json", file));
  assert(strcmp(file.name, "Ghost of Y\\u014dtei") == 0);
  assert(strcmp(file.process, "eboot.bin") == 0);
  assert(file.author_count == 1);
  assert(strcmp(file.authors[0], "Yharnam") == 0);
  assert(file.cheat_count == 3);
  assert(strcmp(file.cheats[0].name, "Infi Health") == 0);
  assert(file.cheats[0].patch_count == 2);
}

}  // namespace

int main() {
  parse_buffer_success();
  rejects_invalid_input();
  accepts_bounded_non_terminated_buffer();
  loads_real_fixture();
}
