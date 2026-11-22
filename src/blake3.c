#include "blake3.h"

#include <string.h>

enum {
  BLAKE3_BLOCK_LEN = 64,
  BLAKE3_CHUNK_LEN = 1024,
  BLAKE3_CHUNK_START = 1 << 0,
  BLAKE3_CHUNK_END = 1 << 1,
  BLAKE3_PARENT = 1 << 2,
  BLAKE3_ROOT = 1 << 3
};

static const uint32_t BLAKE3_IV[8] = {
  0x6A09E667U, 0xBB67AE85U, 0x3C6EF372U, 0xA54FF53AU,
  0x510E527FU, 0x9B05688CU, 0x1F83D9ABU, 0x5BE0CD19U
};

static const uint8_t MSG_PERM[16] = {
  2, 6, 3, 10, 7, 0, 4, 13, 1, 11, 12, 5, 9, 14, 15, 8
};

static uint32_t rotr32(uint32_t x, unsigned n) {
  return (x >> n) | (x << (32U - n));
}

static uint32_t load32_le(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
         ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void store32_le(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)(v & 0xffU);
  p[1] = (uint8_t)((v >> 8) & 0xffU);
  p[2] = (uint8_t)((v >> 16) & 0xffU);
  p[3] = (uint8_t)((v >> 24) & 0xffU);
}

static void store_words_le(uint8_t *out, const uint32_t *w, size_t nwords) {
  for (size_t i = 0; i < nwords; i++) {
    store32_le(out + i * 4U, w[i]);
  }
}

static void g_fn(uint32_t state[16], size_t a, size_t b, size_t c, size_t d,
                 uint32_t mx, uint32_t my) {
  state[a] = state[a] + state[b] + mx;
  state[d] = rotr32(state[d] ^ state[a], 16);
  state[c] = state[c] + state[d];
  state[b] = rotr32(state[b] ^ state[c], 12);
  state[a] = state[a] + state[b] + my;
  state[d] = rotr32(state[d] ^ state[a], 8);
  state[c] = state[c] + state[d];
  state[b] = rotr32(state[b] ^ state[c], 7);
}

static void round_fn(uint32_t state[16], const uint32_t m[16]) {
  g_fn(state, 0, 4, 8, 12, m[0], m[1]);
  g_fn(state, 1, 5, 9, 13, m[2], m[3]);
  g_fn(state, 2, 6, 10, 14, m[4], m[5]);
  g_fn(state, 3, 7, 11, 15, m[6], m[7]);
  g_fn(state, 0, 5, 10, 15, m[8], m[9]);
  g_fn(state, 1, 6, 11, 12, m[10], m[11]);
  g_fn(state, 2, 7, 8, 13, m[12], m[13]);
  g_fn(state, 3, 4, 9, 14, m[14], m[15]);
}

static void permute_msg(uint32_t m[16]) {
  uint32_t tmp[16];
  for (size_t i = 0; i < 16; i++) tmp[i] = m[MSG_PERM[i]];
  memcpy(m, tmp, sizeof tmp);
}

static void blake3_compress(const uint32_t cv[8], const uint8_t block[64],
                            uint8_t block_len, uint64_t counter,
                            uint8_t flags, uint32_t out[16]) {
  uint32_t m[16];
  for (size_t i = 0; i < 16; i++) {
    m[i] = load32_le(block + i * 4U);
  }

  uint32_t state[16];
  for (size_t i = 0; i < 8; i++) state[i] = cv[i];
  state[8] = BLAKE3_IV[0];
  state[9] = BLAKE3_IV[1];
  state[10] = BLAKE3_IV[2];
  state[11] = BLAKE3_IV[3];
  state[12] = (uint32_t)(counter & 0xffffffffULL);
  state[13] = (uint32_t)(counter >> 32);
  state[14] = (uint32_t)block_len;
  state[15] = (uint32_t)flags;

  for (size_t r = 0; r < 7; r++) {
    round_fn(state, m);
    if (r < 6) permute_msg(m);
  }

  for (size_t i = 0; i < 8; i++) {
    state[i] ^= state[i + 8];
    state[i + 8] ^= cv[i];
  }
  memcpy(out, state, sizeof state);
}

void blake3_hash(const uint8_t *in, size_t len, uint8_t out[32]) {
  if (out == NULL) return;
  if (in == NULL && len > 0) {
    memset(out, 0, 32);
    return;
  }

  size_t num_chunks = len == 0 ? 1 : ((len - 1) / BLAKE3_CHUNK_LEN + 1);
  uint32_t cv_stack[64][8];
  size_t cv_stack_len = 0;

  for (size_t c = 0; c < num_chunks; c++) {
    size_t chunk_off = c * BLAKE3_CHUNK_LEN;
    size_t chunk_len = len - chunk_off;
    if (chunk_len > BLAKE3_CHUNK_LEN) chunk_len = BLAKE3_CHUNK_LEN;

    uint32_t cv[8];
    for (size_t i = 0; i < 8; i++) cv[i] = BLAKE3_IV[i];

    size_t nblocks = chunk_len == 0 ? 1 : ((chunk_len - 1) / BLAKE3_BLOCK_LEN + 1);
    for (size_t b = 0; b < nblocks; b++) {
      size_t boff = b * BLAKE3_BLOCK_LEN;
      size_t blen = chunk_len - boff;
      if (blen > BLAKE3_BLOCK_LEN) blen = BLAKE3_BLOCK_LEN;

      uint8_t block[BLAKE3_BLOCK_LEN];
      memset(block, 0, sizeof block);
      if (blen > 0 && in != NULL) {
        memcpy(block, in + chunk_off + boff, blen);
      }

      uint8_t flags = 0;
      if (b == 0) flags |= BLAKE3_CHUNK_START;
      if (b + 1 == nblocks) flags |= BLAKE3_CHUNK_END;
      if (num_chunks == 1 && b + 1 == nblocks) flags |= BLAKE3_ROOT;

      uint32_t words[16];
      blake3_compress(cv, block, (uint8_t)blen, (uint64_t)c, flags, words);
      for (size_t i = 0; i < 8; i++) cv[i] = words[i];
    }

    if (c + 1 < num_chunks) {
      for (size_t i = 0; i < 8; i++) cv_stack[cv_stack_len][i] = cv[i];
      cv_stack_len++;
      size_t total = c + 1;
      while ((total & 1U) == 0) {
        uint8_t pblock[BLAKE3_BLOCK_LEN];
        store_words_le(pblock, cv_stack[cv_stack_len - 2], 8);
        store_words_le(pblock + 32, cv_stack[cv_stack_len - 1], 8);
        uint32_t words[16];
        blake3_compress(BLAKE3_IV, pblock, BLAKE3_BLOCK_LEN, 0, BLAKE3_PARENT, words);
        cv_stack_len--;
        for (size_t i = 0; i < 8; i++) cv_stack[cv_stack_len - 1][i] = words[i];
        total >>= 1;
      }
    } else if (num_chunks > 1) {
      while (cv_stack_len > 0) {
        uint8_t pblock[BLAKE3_BLOCK_LEN];
        store_words_le(pblock, cv_stack[cv_stack_len - 1], 8);
        store_words_le(pblock + 32, cv, 8);
        cv_stack_len--;
        uint8_t pflags = BLAKE3_PARENT | (cv_stack_len == 0 ? BLAKE3_ROOT : 0);
        uint32_t words[16];
        blake3_compress(BLAKE3_IV, pblock, BLAKE3_BLOCK_LEN, 0, pflags, words);
        for (size_t i = 0; i < 8; i++) cv[i] = words[i];
      }
      store_words_le(out, cv, 8);
    } else {
      store_words_le(out, cv, 8);
    }
  }
}
