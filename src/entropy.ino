extern "C" {
#include "sha256.h"
#include "tools.h"
}

#include <stdint.h>
#include <string.h>

static SHA256_CTX entropy_ctx;
static uint8_t entropy_seed[SHA256_BLOCK_SIZE];

#define MIN(a, b) (a < b ? a : b)

void entropy_setup() { sha256_init(&entropy_ctx); }

void entropy_add(const uint8_t *data, uint32_t length) {
  sha256_update(&entropy_ctx, data, length);
}

void entropy_touch() {
  unsigned long time = millis();
  entropy_add((uint8_t *)&time, sizeof(time));
}

uint32_t entropy_get_32() {
  uint32_t v;
  entropy_get((uint8_t *)&v, sizeof(v));
  return v;
}

void entropy_get(uint8_t *data, uint32_t length) {
  SHA256_CTX ctx;
  uint32_t copied = 0;
  uint8_t hash[SHA256_BLOCK_SIZE];
  while (copied < length) {
    memcpy(&ctx, &entropy_ctx, sizeof(entropy_ctx));
    sha256_final(&ctx, hash);
    uint32_t to_copy = MIN(length - copied, sizeof(hash));
    memcpy(&data[copied], hash, to_copy);
    copied += to_copy;
    entropy_touch();
  }
}

void entropy_get_hex(char *string, uint32_t length) {
  uint8_t buffer[SHA256_BLOCK_SIZE];
  entropy_get(buffer, sizeof(buffer));
  to_hex(buffer, sizeof(buffer), string, length);
}

uint8_t entropy_seed_get(uint8_t *seed, uint8_t length) {
  length = MIN(length, sizeof(entropy_seed));
  memcpy(seed, entropy_seed, length);
  entropy_touch();
  return length;
}

void entropy_seed_get_hex(char *string, uint32_t length) {
  uint8_t buffer[SHA256_BLOCK_SIZE];
  entropy_seed_get(buffer, sizeof(buffer));
  to_hex(buffer, sizeof(buffer), string, length);
}

void entropy_seed_new() { entropy_get(entropy_seed, sizeof(entropy_seed)); }
