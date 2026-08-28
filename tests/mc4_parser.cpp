#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "ezcheats/domain/owned_cheat_file.hpp"
#include "ezcheats/parsers/cheat_parser_factory.hpp"

extern "C" {
#define CBC 1
#include "mc4/aes.h"
#include "mc4/base64.h"
}

namespace {

constexpr uint8_t kMc4Key[] = "304c6528f659c766110239a51cl5dd9c";
constexpr uint8_t kMc4Iv[] = "u@}kzW2u[u(8DWar";

unsigned char* encode_mc4(const char* xml, size_t& encoded_size) {
  const size_t xml_size = strlen(xml) + 1;
  const size_t padded_size = (xml_size + AES_BLOCKLEN - 1) &
                             ~(static_cast<size_t>(AES_BLOCKLEN) - 1);
  auto* encrypted = static_cast<uint8_t*>(calloc(padded_size, 1));
  assert(encrypted != nullptr);
  memcpy(encrypted, xml, xml_size);
  AES_ctx context{};
  AES_init_ctx_iv(&context, kMc4Key, kMc4Iv);
  AES_CBC_encrypt_buffer(&context, encrypted, padded_size);
  unsigned char* encoded = base64_encode(encrypted, padded_size, &encoded_size);
  memset(&context, 0, sizeof(context));
  memset(encrypted, 0, padded_size);
  free(encrypted);
  return encoded;
}

void parses_synthetic_mc4() {
  constexpr const char* xml =
      "<Trainer Process=\"eboot.bin\" Game=\"MC4 Demo\" Moder=\"Tester\">"
      "<Cheat Text=\"Master Code\" Description=\"base\">"
      "<Cheatline><Offset>2000</Offset><ValueOn>AA</ValueOn>"
      "<ValueOff>BB</ValueOff></Cheatline></Cheat></Trainer>";
  size_t encoded_size = 0;
  unsigned char* encoded = encode_mc4(xml, encoded_size);
  assert(encoded != nullptr);

  ezcheats::domain::OwnedCheatFile owned;
  auto* parser = ezcheats::parsers::CheatParserFactory::parser_for_format("MC4");
  assert(parser != nullptr && strcmp(parser->name(), "mc4") == 0);
  assert(ezcheats::parsers::CheatParserFactory::load_buffer(
      ".mc4", encoded, encoded_size, owned.get()));
  memset(encoded, 0, encoded_size);
  free(encoded);

  const auto& file = owned.get();
  assert(strcmp(file.name, "MC4 Demo") == 0);
  assert(strcmp(file.process, "eboot.bin") == 0);
  assert(file.author_count == 1 && strcmp(file.authors[0], "Tester") == 0);
  assert(file.cheat_count == 1);
  assert(strcmp(file.cheats[0].name, "Master Code") == 0);
  assert(file.cheats[0].patches[0].offset == 0x2000);
}

void rejects_invalid_payloads() {
  ezcheats::domain::OwnedCheatFile owned;
  constexpr const char* invalid_base64 = "%%%%";
  assert(!ezcheats::parsers::CheatParserFactory::load_buffer(
      "mc4", reinterpret_cast<const uint8_t*>(invalid_base64),
      strlen(invalid_base64), owned.get()));

  constexpr const char* wrong_block_size = "YWJj";
  assert(!ezcheats::parsers::CheatParserFactory::load_buffer(
      "mc4", reinterpret_cast<const uint8_t*>(wrong_block_size),
      strlen(wrong_block_size), owned.get()));
  assert(owned.get().cheats == nullptr);
}

void loads_real_fixture() {
  ezcheats::domain::OwnedCheatFile owned;
  auto& file = owned.get();
  assert(ezcheats::parsers::CheatParserFactory::load_file(
      "tests/fixtures/PPSA08710_01.005.000.mc4", file));
  assert(strcmp(file.process, "eboot.bin") == 0);
  assert(file.name[0] != '\0');
  assert(file.cheat_count > 0);
  assert(file.cheats[0].name[0] != '\0');
}

}  // namespace

int main() {
  parses_synthetic_mc4();
  rejects_invalid_payloads();
  loads_real_fixture();
}
