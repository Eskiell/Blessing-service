#include "ezcheats/parsers/shnext_core.hpp"
#include "ezcheats/domain/owned_cheat_file.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
#define CBC 1
#include "mc4/aes.h"
#include "mc4/base64.h"
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include "miniz.h"
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#include "sha256.h"
}

#include "cJSON.hpp"
#if EZ_CHEATS_HAS_KEYSTONE
#include "keystone/keystone.h"
#endif

#define LOG_INFO(...) ((void)fprintf(stderr, __VA_ARGS__))
#define LOG_ERROR(...) ((void)fprintf(stderr, __VA_ARGS__))

namespace ezcheats::parsers::detail {

static const uint8_t SHNEXT_FIELD[64] = {
    0x04, 0x14, 0x02, 0x80, 0x01, 0x20, 0xF3, 0xEB,
    0x03, 0x01, 0x02, 0x80, 0x00, 0x20, 0xF3, 0xEB,
    0x05, 0x18, 0x02, 0x80, 0x00, 0x20, 0xF3, 0xEB,
    0x00, 0x1C, 0x02, 0x80, 0x71, 0x0F, 0x8C, 0xBF,
    0xF2, 0x0E, 0x06, 0x08, 0x03, 0x06, 0x10, 0x10,
    0x00, 0x07, 0x24, 0xC8, 0x08, 0x05, 0x0E, 0x3E,
    0x00, 0x04, 0x20, 0xC8, 0x01, 0x07, 0x25, 0xC8,
    0x00, 0x05, 0x28, 0xC8, 0x00, 0x06, 0x2C, 0xC8,
};

static const char SHNEXT_PROCESS_KEY[] = "#my*S3cr3t";

static bool shnext_deflate_decompress(const uint8_t *data, size_t data_len,
                                      uint8_t **out, size_t *out_len) {
  size_t buf_len = data_len * 10 + 1024;
  uint8_t *buf = NULL;

  *out = NULL;
  *out_len = 0;

  buf = (uint8_t *)malloc(buf_len);
  if (buf == NULL) {
    return false;
  }

  for (int attempt = 0; attempt < 4; ++attempt) {
    size_t result = tinfl_decompress_mem_to_mem(
        buf, buf_len, data, data_len,
        TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
    if (result > 0 && result <= buf_len) {
      *out = buf;
      *out_len = result;
      return true;
    }
    domain::secure_zero(buf, buf_len);
    free(buf);
    buf_len *= 2;
    buf = (uint8_t *)malloc(buf_len);
    if (buf == NULL) {
      return false;
    }
  }

  domain::secure_zero(buf, buf_len);
  free(buf);
  return false;
}

static void shnext_clear_json_strings(cJSON *item) {
  if (item == NULL) {
    return;
  }
  if (item->valuestring != NULL) {
    domain::secure_zero(item->valuestring, strlen(item->valuestring));
  }
  shnext_clear_json_strings(item->child);
  shnext_clear_json_strings(item->next);
}

static void shnext_clear_pending_cheat(domain::CheatEntry *cheat) {
  if (cheat == NULL) {
    return;
  }
  if (cheat->patches != NULL) {
    domain::secure_zero(cheat->patches,
                        cheat->patch_capacity * sizeof(domain::Patch));
    free(cheat->patches);
  }
  memset(cheat, 0, sizeof(*cheat));
}

static void shnext_sha256(const uint8_t *data, size_t data_len,
                          uint8_t hash[32]) {
  SHA256_CTX ctx;

  sha256_init(&ctx);
  sha256_update(&ctx, data, data_len);
  sha256_final(&ctx, hash);
}

static size_t shnext_pkcs7_unpad(uint8_t *buf, size_t len) {
  uint8_t pad;
  size_t i;

  if (len == 0) {
    return 0;
  }
  pad = buf[len - 1];
  if (pad == 0 || pad > AES_BLOCKLEN) {
    return len;
  }
  for (i = len - pad; i < len; ++i) {
    if (buf[i] != pad) {
      return len;
    }
  }
  return len - pad;
}

static bool shnext_aes256_cbc_decrypt(const uint8_t *ciphertext, size_t ct_len,
                                      const uint8_t key[32],
                                      const uint8_t iv[16], uint8_t **out,
                                      size_t *out_len) {
  struct AES_ctx ctx;
  uint8_t *buf = NULL;

  *out = NULL;
  *out_len = 0;

  if (ct_len == 0 || (ct_len % AES_BLOCKLEN) != 0) {
    return false;
  }

  buf = (uint8_t *)malloc(ct_len + 1);
  if (buf == NULL) {
    return false;
  }
  memcpy(buf, ciphertext, ct_len);

  AES_init_ctx_iv(&ctx, key, iv);
  AES_CBC_decrypt_buffer(&ctx, buf, ct_len);
  domain::secure_zero(&ctx, sizeof(ctx));

  *out_len = shnext_pkcs7_unpad(buf, ct_len);
  *out = buf;
  return true;
}

static char *shnext_decrypt_process_name(const char *b64_blob) {
  unsigned char *raw = NULL;
  size_t raw_len = 0;
  uint8_t proc_key[32];
  uint8_t *plain = NULL;
  size_t plain_len = 0;
  char *result = NULL;

  raw = base64_decode((const unsigned char *)b64_blob, strlen(b64_blob),
                      &raw_len);
  if (raw == NULL || raw_len < 17) {
    free(raw);
    return NULL;
  }

  shnext_sha256((const uint8_t *)SHNEXT_PROCESS_KEY,
                strlen(SHNEXT_PROCESS_KEY), proc_key);

  if (!shnext_aes256_cbc_decrypt(raw + 16, raw_len - 16, proc_key, raw,
                                 &plain, &plain_len)) {
    domain::secure_zero(raw, raw_len);
    domain::secure_zero(proc_key, sizeof(proc_key));
    free(raw);
    return NULL;
  }
  domain::secure_zero(raw, raw_len);
  free(raw);

  if (plain_len >= 23 && plain[17] == 0x06) {
    uint32_t str_len = ((uint32_t)plain[19] << 24) |
                       ((uint32_t)plain[20] << 16) |
                       ((uint32_t)plain[21] << 8) | ((uint32_t)plain[22]);
    if (23 + str_len <= plain_len) {
      result = (char *)malloc(str_len + 1);
      if (result != NULL) {
        memcpy(result, plain + 23, str_len);
        result[str_len] = '\0';
      }
    }
  }

  if (result == NULL) {
    result = (char *)malloc(plain_len + 1);
    if (result != NULL) {
      memcpy(result, plain, plain_len);
      result[plain_len] = '\0';
    }
  }

  domain::secure_zero(plain, plain_len);
  domain::secure_zero(proc_key, sizeof(proc_key));
  free(plain);
  return result;
}

static void shnext_get_entry_key_iv(int entry_index, uint8_t key[32],
                                    uint8_t iv[16]) {
  uint8_t key_byte = SHNEXT_FIELD[(entry_index * 5) % 64];
  uint8_t iv_byte = SHNEXT_FIELD[(entry_index * 5) % 32];

  memset(key, key_byte, 32);
  memset(iv, iv_byte, 16);
}

static bool shnext_parse_nop(const char *value, uint8_t *out_bytes,
                             size_t *out_len) {
  int nop_count;

  if (value == NULL) {
    return false;
  }

  if (sscanf(value, "nop:%d", &nop_count) == 1 && nop_count > 0 &&
      nop_count <= (int)(domain::kMaxPatchBytes)) {
    memset(out_bytes, 0x90, (size_t)nop_count);
    *out_len = (size_t)nop_count;
    LOG_INFO("[engine] shnext nop:%d -> %zu byte(s) of 0x90",
                     nop_count, *out_len);
    return true;
  }

  return false;
}

static bool shnext_assemble(const char *asm_text, uint8_t *out_bytes,
                            size_t *out_len) {
#if EZ_CHEATS_HAS_KEYSTONE
  ks_engine *ks = NULL;
  ks_err err;
  unsigned char *enc = NULL;
  size_t enc_size = 0;
  size_t count = 0;
  int rc;

  if (asm_text == NULL || asm_text[0] == '\0') {
    return false;
  }

  err = ks_open(KS_ARCH_X86, KS_MODE_64, &ks);
  if (err != KS_ERR_OK) {
    LOG_ERROR("[engine] shnext keystone open failed: %s",
                     ks_strerror(err));
    return false;
  }

  rc = ks_asm(ks, asm_text, 0, &enc, &enc_size, &count);
  if (rc != 0) {
    LOG_ERROR("[engine] shnext keystone asm failed: %s",
                     ks_strerror(ks_errno(ks)));
    ks_close(ks);
    return false;
  }

  ks_close(ks);

  if (enc_size == 0 || enc_size > domain::kMaxPatchBytes) {
    LOG_ERROR("[engine] shnext keystone asm invalid size %zu",
                     enc_size);
    if(enc != NULL) {
      domain::secure_zero(enc, enc_size);
    }
    ks_free(enc);
    return false;
  }

  memcpy(out_bytes, enc, enc_size);
  *out_len = enc_size;

  LOG_INFO("[engine] shnext keystone asm produced %zu byte(s)",
                   enc_size);

  domain::secure_zero(enc, enc_size);
  ks_free(enc);
  return true;
#else
  (void)asm_text;
  (void)out_bytes;
  (void)out_len;
  return false;
#endif
}

static bool shnext_asm_value_to_bytes(const char *value, uint8_t *out_bytes,
                                      size_t *out_len) {
  if (shnext_parse_nop(value, out_bytes, out_len)) {
    return true;
  }
  if (shnext_assemble(value, out_bytes, out_len)) {
    return true;
  }
  return false;
}

static bool shnext_parse_variable(const cJSON *var_obj, domain::Patch *patch) {
  cJSON *field = NULL;
  const char *label = NULL;
  const char *on_str = NULL;
  const char *off_str = NULL;

  if (var_obj == NULL || !cJSON_IsObject(var_obj)) {
    return false;
  }

  memset(patch, 0, sizeof(*patch));
  patch->section = 0;
  patch->absolute = false;
  patch->assembly = false;

  field = cJSON_GetObjectItem(var_obj, "offset");
  if (field == NULL || !cJSON_IsNumber(field)) {
    return false;
  }
  patch->offset = (uint64_t)field->valuedouble;

  field = cJSON_GetObjectItem(var_obj, "label");
  label = cJSON_IsString(field) ? field->valuestring : NULL;

  field = cJSON_GetObjectItem(var_obj, "newValue");
  on_str = cJSON_IsString(field) ? field->valuestring : NULL;
  if (on_str != NULL &&
      shnext_asm_value_to_bytes(on_str, patch->enable,
                                &patch->enable_size)) {
  } else {
    patch->assembly = true;
    patch->enable_size = 0;
    if (on_str != NULL) {
      LOG_ERROR("[engine] shnext failed to assemble on value");
    }
  }

  field = cJSON_GetObjectItem(var_obj, "defaultValue");
  off_str = cJSON_IsString(field) ? field->valuestring : NULL;
  if (off_str != NULL &&
      shnext_asm_value_to_bytes(off_str, patch->disable,
                                &patch->disable_size)) {
  } else {
    patch->assembly = true;
    patch->disable_size = 0;
    if (off_str != NULL) {
      LOG_ERROR("[engine] shnext failed to assemble off value");
    }
  }

  LOG_INFO("[engine] shnext var label='%s' offset=0x%llx "
                   "on_len=%zu off_len=%zu is_asm=%d",
                   label ? label : "(none)",
                   (unsigned long long)patch->offset,
                   patch->enable_size, patch->disable_size, patch->assembly);

  return true;
}

static bool shnext_decrypt_patch_entry(const char *b64_blob, int entry_index,
                                       cJSON **out) {
  unsigned char *raw = NULL;
  size_t raw_len = 0;
  uint8_t key[32];
  uint8_t iv[16];
  uint8_t *plain = NULL;
  size_t plain_len = 0;
  uint8_t *json_buf = NULL;
  uint8_t *resized_json = NULL;
  size_t json_len = 0;

  *out = NULL;

  raw = base64_decode((const unsigned char *)b64_blob, strlen(b64_blob),
                      &raw_len);
  if (raw == NULL || raw_len == 0 || (raw_len % AES_BLOCKLEN) != 0) {
    if(raw != NULL) {
      domain::secure_zero(raw, raw_len);
    }
    free(raw);
    return false;
  }

  shnext_get_entry_key_iv(entry_index, key, iv);

  if (!shnext_aes256_cbc_decrypt(raw, raw_len, key, iv, &plain, &plain_len)) {
    domain::secure_zero(raw, raw_len);
    domain::secure_zero(key, sizeof(key));
    domain::secure_zero(iv, sizeof(iv));
    free(raw);
    return false;
  }
  domain::secure_zero(raw, raw_len);
  free(raw);

  if (!shnext_deflate_decompress(plain, plain_len, &json_buf, &json_len)) {
    domain::secure_zero(plain, plain_len);
    domain::secure_zero(key, sizeof(key));
    domain::secure_zero(iv, sizeof(iv));
    free(plain);
    return false;
  }
  domain::secure_zero(plain, plain_len);
  free(plain);

  resized_json = (uint8_t *)realloc(json_buf, json_len + 1);
  if (resized_json == NULL) {
    domain::secure_zero(json_buf, json_len);
    free(json_buf);
    domain::secure_zero(key, sizeof(key));
    domain::secure_zero(iv, sizeof(iv));
    return false;
  }
  json_buf = resized_json;
  json_buf[json_len] = '\0';

  *out = cJSON_Parse((const char *)json_buf);
  domain::secure_zero(json_buf, json_len + 1);
  free(json_buf);
  domain::secure_zero(key, sizeof(key));
  domain::secure_zero(iv, sizeof(iv));

  if (*out == NULL) {
    return false;
  }

  return true;
}

bool parse_shnext(const char *data, size_t size,
                                    domain::CheatFile *out) {
  uint8_t *json_buf = NULL;
  uint8_t *resized_json = NULL;
  size_t json_len = 0;
  cJSON *root = NULL;
  cJSON *field = NULL;
  cJSON *entries_arr = NULL;
  int entry_count = 0;
  char *proc_name = NULL;

  domain::clear_cheat_file(*out);

  if (data == NULL || size == 0) {
    return false;
  }

  if (!shnext_deflate_decompress((const uint8_t *)data, size, &json_buf,
                                 &json_len)) {
    LOG_ERROR("[engine] shnext deflate decompress failed");
    return false;
  }

  resized_json = (uint8_t *)realloc(json_buf, json_len + 1);
  if (resized_json == NULL) {
    domain::secure_zero(json_buf, json_len);
    free(json_buf);
    return false;
  }
  json_buf = resized_json;
  json_buf[json_len] = '\0';

  root = cJSON_Parse((const char *)json_buf);
  domain::secure_zero(json_buf, json_len + 1);
  free(json_buf);

  if (root == NULL) {
    LOG_ERROR("[engine] shnext JSON parse failed");
    return false;
  }

  field = cJSON_GetObjectItem(root, "ProcessName");
  if (cJSON_IsString(field) && field->valuestring != NULL) {
    proc_name = shnext_decrypt_process_name(field->valuestring);
    if (proc_name != NULL) {
      snprintf(out->process, sizeof(out->process), "%s", proc_name);
      domain::secure_zero(proc_name, strlen(proc_name));
      free(proc_name);
    }
  }

  field = cJSON_GetObjectItem(root, "Game");
  if (cJSON_IsString(field) && field->valuestring != NULL) {
    snprintf(out->name, sizeof(out->name), "%s", field->valuestring);
  }

  entries_arr = cJSON_GetObjectItem(root, "patchEntries");
  if (entries_arr == NULL || !cJSON_IsArray(entries_arr)) {
    shnext_clear_json_strings(root);
    cJSON_Delete(root);
    return out->cheat_count > 0;
  }

  entry_count = cJSON_GetArraySize(entries_arr);
  for (int i = 0; i < entry_count; ++i) {
    cJSON *entry_json = NULL;
    cJSON *entry_item = NULL;
    cJSON *vars_arr = NULL;
    cJSON *entry_field = NULL;
    domain::CheatEntry *cheat = NULL;
    int var_count = 0;

    entry_item = cJSON_GetArrayItem(entries_arr, i);
    if (entry_item == NULL || !cJSON_IsString(entry_item)) {
      continue;
    }
    if (!shnext_decrypt_patch_entry(entry_item->valuestring, i, &entry_json)) {
      continue;
    }

    if (!domain::ensure_cheat(*out)) {
      cJSON_Delete(entry_json);
      continue;
    }
    cheat = &out->cheats[out->cheat_count];
    shnext_clear_pending_cheat(cheat);

    entry_field = cJSON_GetObjectItem(entry_json, "name");
    if (cJSON_IsString(entry_field) && entry_field->valuestring != NULL) {
      snprintf(cheat->name, sizeof(cheat->name), "%s",
               entry_field->valuestring);
    }

    entry_field = cJSON_GetObjectItem(entry_json, "author");
    if (cJSON_IsString(entry_field) && entry_field->valuestring != NULL) {
      snprintf(cheat->author, sizeof(cheat->author), "%s",
               entry_field->valuestring);
      snprintf(cheat->description, sizeof(cheat->description), "by %s",
               entry_field->valuestring);
    }

    if (out->process[0] != '\0') {
      snprintf(cheat->module, sizeof(cheat->module), "%s",
               out->process);
    }

    vars_arr = cJSON_GetObjectItem(entry_json, "Variables");
    if (vars_arr == NULL || !cJSON_IsArray(vars_arr)) {
      shnext_clear_json_strings(entry_json);
      cJSON_Delete(entry_json);
      if (cheat->name[0] != '\0') {
        cheat->id = static_cast<uint32_t>(out->cheat_count);
        domain::add_author(*out, cheat->author);
        ++out->cheat_count;
      }
      continue;
    }

    var_count = cJSON_GetArraySize(vars_arr);
    for (int j = 0; j < var_count; ++j) {
      cJSON *var_obj = cJSON_GetArrayItem(vars_arr, j);

      if (!domain::ensure_patch(*cheat)) {
        break;
      }
      if (shnext_parse_variable(var_obj,
                                &cheat->patches[cheat->patch_count])) {
        ++cheat->patch_count;
      }
    }

    shnext_clear_json_strings(entry_json);
    cJSON_Delete(entry_json);

    if (cheat->patch_count > 0) {
      cheat->id = static_cast<uint32_t>(out->cheat_count);
      domain::add_author(*out, cheat->author);
      ++out->cheat_count;
    } else {
      shnext_clear_pending_cheat(cheat);
    }
  }

  shnext_clear_json_strings(root);
  cJSON_Delete(root);

  if (out->cheat_count == 0) {
    LOG_INFO("[engine] shnext no cheats parsed");
    return false;
  }

  LOG_INFO("[engine] shnext parsed %zu cheats",
                   out->cheat_count);
  return true;
}

}  // namespace ezcheats::parsers::detail
