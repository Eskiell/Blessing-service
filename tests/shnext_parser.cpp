#include <assert.h>
#include <string.h>

#include "ezcheats/domain/owned_cheat_file.hpp"
#include "ezcheats/parsers/cheat_parser_factory.hpp"

namespace {

void registers_extension_case_insensitively() {
  auto* parser =
      ezcheats::parsers::CheatParserFactory::parser_for_path("demo.ShnExt");
  assert(parser != nullptr);
  assert(strcmp(parser->name(), "ShnExt") == 0);
}

void rejects_invalid_deflate_data() {
  constexpr uint8_t invalid[] = {0x00, 0x01, 0x02, 0x03};
  ezcheats::domain::OwnedCheatFile owned;
  assert(!ezcheats::parsers::CheatParserFactory::load_buffer(
      "shnext", invalid, sizeof(invalid), owned.get()));
  assert(owned.get().cheats == nullptr);
}

void loads_real_fixture() {
  ezcheats::domain::OwnedCheatFile owned;
  auto& file = owned.get();
  assert(ezcheats::parsers::CheatParserFactory::load_file(
      "tests/fixtures/Assassins-Creed-Mirage_PPSA07230_01.012.000_Aigars_Uze.ShnExt",
      file));

  assert(strcmp(file.name, "Assassin's Creed Mirage") == 0);
  assert(strcmp(file.process, "eboot.bin") == 0);
  assert(file.author_count == 1);
  assert(strcmp(file.authors[0], "Anonyme") == 0);
  assert(file.cheat_count == 4);

  const auto& visibility = file.cheats[0];
  assert(visibility.id == 0);
  assert(strcmp(visibility.name, "Enemys Dont See You") == 0);
  assert(strcmp(visibility.author, "Anonyme") == 0);
  assert(strcmp(visibility.description, "by Anonyme") == 0);
  assert(strcmp(visibility.module, "eboot.bin") == 0);
  assert(visibility.patch_count == 1);
  assert(visibility.patches[0].offset == 109230218);
  assert(visibility.patches[0].enable_size == 3);
  assert(visibility.patches[0].enable[0] == 0x90);
  assert(visibility.patches[0].assembly);
  assert(visibility.patches[0].disable_size == 0);

  assert(strcmp(file.cheats[1].name, "Infinite Air Under Water") == 0);
  assert(file.cheats[1].patches[0].offset == 88497722);
  assert(file.cheats[1].patches[0].enable_size == 8);
  assert(strcmp(file.cheats[2].name, "Infinite Health") == 0);
  assert(file.cheats[2].patches[0].offset == 31563972);
  assert(file.cheats[2].patches[0].enable_size == 6);
  assert(strcmp(file.cheats[3].name, "Infinite Stamina") == 0);
  assert(file.cheats[3].patches[0].offset == 39201365);
  assert(file.cheats[3].patches[0].assembly);
  assert(file.cheats[3].patches[0].enable_size == 0);
}

}  // namespace

int main() {
  registers_extension_case_insensitively();
  rejects_invalid_deflate_data();
  loads_real_fixture();
}
