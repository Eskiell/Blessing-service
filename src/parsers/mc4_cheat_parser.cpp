#include "ezcheats/parsers/mc4_cheat_parser.hpp"

#include <stdlib.h>
#include <string.h>

#include "ezcheats/domain/owned_cheat_file.hpp"
#include "ezcheats/parsers/shn_cheat_parser.hpp"

extern "C" {
#define CBC 1
#include "mc4/aes.h"
#include "mc4/base64.h"
}

namespace ezcheats::parsers {
namespace {

constexpr uint8_t kMc4Key[] = "304c6528f659c766110239a51cl5dd9c";
constexpr uint8_t kMc4Iv[] = "u@}kzW2u[u(8DWar";

struct DecryptedBuffer {
  uint8_t* data;
  size_t size;
};

DecryptedBuffer decrypt(const uint8_t* encoded, size_t encoded_size) noexcept {
  size_t ciphertext_size = 0;
  unsigned char* ciphertext = base64_decode(encoded, encoded_size,
                                             &ciphertext_size);
  if (ciphertext == nullptr || ciphertext_size == 0 ||
      (ciphertext_size % AES_BLOCKLEN) != 0) {
    if (ciphertext != nullptr) {
      domain::secure_zero(ciphertext, ciphertext_size);
      free(ciphertext);
    }
    return {nullptr, 0};
  }

  auto* plaintext = static_cast<uint8_t*>(calloc(ciphertext_size + 1, 1));
  if (plaintext == nullptr) {
    domain::secure_zero(ciphertext, ciphertext_size);
    free(ciphertext);
    return {nullptr, 0};
  }
  memcpy(plaintext, ciphertext, ciphertext_size);
  domain::secure_zero(ciphertext, ciphertext_size);
  free(ciphertext);

  AES_ctx context{};
  AES_init_ctx_iv(&context, kMc4Key, kMc4Iv);
  AES_CBC_decrypt_buffer(&context, plaintext, ciphertext_size);
  domain::secure_zero(&context, sizeof(context));
  return {plaintext, ciphertext_size};
}

}  // namespace

bool Mc4CheatParser::parse(const uint8_t* data, size_t size,
                           domain::CheatFile& output) {
  domain::clear_cheat_file(output);
  if (data == nullptr || size == 0) return false;
  DecryptedBuffer plaintext = decrypt(data, size);
  if (plaintext.data == nullptr) return false;
  ShnCheatParser shn;
  const bool parsed = shn.parse(plaintext.data, plaintext.size, output);
  domain::secure_zero(plaintext.data, plaintext.size + 1);
  free(plaintext.data);
  return parsed;
}

}  // namespace ezcheats::parsers
