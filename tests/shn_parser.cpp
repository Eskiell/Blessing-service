#include <assert.h>
#include <string.h>

#include "ezcheats/domain/owned_cheat_file.hpp"
#include "ezcheats/parsers/cheat_parser_factory.hpp"

namespace {

void parses_buffer() {
  constexpr const char* xml =
      "<Trainer Process=\"eboot.bin\" Game=\"Demo\" Moder=\"Yharnam\">"
      "<Cheat Text=\"Infinite Ammo\" Description=\"No reload\">"
      "<Cheatline><Offset>1234</Offset><Section>2</Section>"
      "<ValueOn>AA-BB</ValueOn><ValueOff>00-11</ValueOff>"
      "<Absolute>true</Absolute></Cheatline></Cheat></Trainer>";
  ezcheats::domain::OwnedCheatFile owned;
  auto& file = owned.get();
  auto* parser = ezcheats::parsers::CheatParserFactory::parser_for_format("SHN");
  assert(parser != nullptr && strcmp(parser->name(), "shn") == 0);
  assert(ezcheats::parsers::CheatParserFactory::load_buffer(
      ".SHN", reinterpret_cast<const uint8_t*>(xml), strlen(xml), file));
  assert(strcmp(file.process, "eboot.bin") == 0);
  assert(strcmp(file.name, "Demo") == 0);
  assert(file.author_count == 1);
  assert(strcmp(file.authors[0], "Yharnam") == 0);
  assert(file.cheat_count == 1);
  assert(strcmp(file.cheats[0].name, "Infinite Ammo") == 0);
  assert(strcmp(file.cheats[0].description, "No reload") == 0);
  assert(strcmp(file.cheats[0].module, "eboot.bin") == 0);
  assert(file.cheats[0].patch_count == 1);
  const auto& patch = file.cheats[0].patches[0];
  assert(patch.offset == 0x1234);
  assert(patch.section == 2);
  assert(patch.absolute);
  assert(patch.enable_size == 2 && patch.enable[0] == 0xaa &&
         patch.enable[1] == 0xbb);
  assert(patch.disable_size == 2 && patch.disable[0] == 0x00 &&
         patch.disable[1] == 0x11);
}

void handles_entities_and_section_bounds() {
  constexpr const char* xml =
      "&lt;Trainer Process=\"eboot.bin\" Game=\"Entity Demo\"&gt;"
      "&lt;Cheat Text=\"Patch\"&gt;&lt;Cheatline&gt;"
      "&lt;Offset&gt;10&lt;/Offset&gt;&lt;Section&gt;99&lt;/Section&gt;"
      "&lt;ValueOn&gt;A&lt;/ValueOn&gt;&lt;ValueOff&gt;0B&lt;/ValueOff&gt;"
      "&lt;/Cheatline&gt;&lt;/Cheat&gt;&lt;/Trainer&gt;";
  ezcheats::domain::OwnedCheatFile owned;
  assert(ezcheats::parsers::CheatParserFactory::load_buffer(
      "shn", reinterpret_cast<const uint8_t*>(xml), strlen(xml), owned.get()));
  const auto& patch = owned.get().cheats[0].patches[0];
  assert(patch.section == 0);
  assert(patch.enable_size == 1 && patch.enable[0] == 0x0a);
}

void decodes_named_entities_in_metadata() {
  constexpr const char* xml =
      "<Trainer Process=\"eboot.bin\" Game=\"Ratchet &amp; Clank\" "
      "Moder=\"Alice &amp; Bob\">"
      "<Cheat Text=\"Health &amp; Ammo\" "
      "Description=\"A &lt; B &gt; &quot;q&quot; &apos;s &amp; Z &nbsp;\">"
      "<Cheatline><Offset>10</Offset><ValueOn>AA</ValueOn>"
      "<ValueOff>BB</ValueOff></Cheatline></Cheat></Trainer>";
  ezcheats::domain::OwnedCheatFile owned;
  assert(ezcheats::parsers::CheatParserFactory::load_buffer(
      "shn", reinterpret_cast<const uint8_t*>(xml), strlen(xml), owned.get()));
  const auto& file = owned.get();
  assert(strcmp(file.name, "Ratchet & Clank") == 0);
  assert(file.author_count == 1);
  assert(strcmp(file.authors[0], "Alice & Bob") == 0);
  assert(strcmp(file.cheats[0].name, "Health & Ammo") == 0);
  assert(strcmp(file.cheats[0].description, "A < B > \"q\" 's & Z &nbsp;") == 0);
}

void rejects_malformed_input() {
  constexpr const char* malformed =
      "<Trainer Process=\"eboot.bin\" Game=\"Demo\"><Cheat Text=\"Bad\">"
      "<Cheatline><Offset>10</Offset><ValueOn>GG</ValueOn>"
      "<ValueOff>00</ValueOff></Cheatline></Cheat></Trainer>";
  ezcheats::domain::OwnedCheatFile owned;
  assert(!ezcheats::parsers::CheatParserFactory::load_buffer(
      "shn", reinterpret_cast<const uint8_t*>(malformed), strlen(malformed),
      owned.get()));
  assert(owned.get().cheats == nullptr);
}

void loads_real_fixture() {
  ezcheats::domain::OwnedCheatFile owned;
  auto& file = owned.get();
  assert(ezcheats::parsers::CheatParserFactory::load_file(
      "tests/fixtures/PPSA21159_01.001.000.shn", file));
  assert(strcmp(file.name, "Silent Hill f") == 0);
  assert(strcmp(file.process, "eboot.bin") == 0);
  assert(file.author_count == 1);
  assert(strcmp(file.authors[0], "Yharnam") == 0);
  assert(file.cheat_count == 9);
  assert(strcmp(file.cheats[0].name, "Infi Health") == 0);
  assert(file.cheats[0].patch_count == 3);
}

}  // namespace

int main() {
  parses_buffer();
  handles_entities_and_section_bounds();
  decodes_named_entities_in_metadata();
  rejects_malformed_input();
  loads_real_fixture();
}
