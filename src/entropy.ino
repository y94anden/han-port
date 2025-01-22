extern "C" {
#include "sha256.h"
#include "tools.h"
}

#include <stdint.h>
#include <string.h>

#define MAX_NR_SEEDS 4
#define MAX_ID_LEN 16

struct Seed_t {
  char id[MAX_ID_LEN];
  uint8_t seed[SHA256_BLOCK_SIZE];
};

static SHA256_CTX entropy_ctx;
static struct Seed_t entropy_seeds[MAX_NR_SEEDS];

#define MIN(a, b) (a < b ? a : b)

void entropy_setup() {
  sha256_init(&entropy_ctx);
  memset(entropy_seeds, 0, sizeof(entropy_seeds));
}

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

static struct Seed_t* get_seed(const char *id, bool create) {
  uint8_t i;
  for (i = 0; i < MAX_NR_SEEDS; i++) {
    if (entropy_seeds[i].id[0] == 0) {
      break;
    }
    if (strncmp(entropy_seeds[i].id, id, MAX_ID_LEN) == 0) {
      return &entropy_seeds[i];
    }
  }

  if (create && i < MAX_NR_SEEDS) {
    strncpy(entropy_seeds[i].id, id, MAX_ID_LEN);
    return &entropy_seeds[i];
  }
  return NULL;
}

uint8_t entropy_seed_get(const char *id, uint8_t *seed, uint8_t length) {
  struct Seed_t *s = get_seed(id, false);
  if (s == NULL) {
    memset(seed, 0, length);
    return 0;
  }

  length = MIN(length, SHA256_BLOCK_SIZE);
  memcpy(seed, s->seed, length);
  entropy_touch();
  return length;
}

void entropy_seed_get_hex(const char *id, char *string, uint32_t length) {
  uint8_t buffer[SHA256_BLOCK_SIZE];
  uint8_t len = entropy_seed_get(id, buffer, sizeof(buffer));
  to_hex(buffer, len, string, length);
}

void entropy_seed_new(const char *id) {
  struct Seed_t *s = get_seed(id, true); // Create new if needed and possible
  if (s == NULL) {
    return;
  }
  entropy_get(s->seed, SHA256_BLOCK_SIZE);
}

void entropy_seed_del(const char* id) {
  struct Seed_t *s = get_seed(id, true); // Create new if needed and possible
  if (s == NULL) {
    return;
  }
  memset(s, 0, sizeof(*s));
}