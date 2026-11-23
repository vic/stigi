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

/* Chaining value of one chunk (0..1024 bytes); root only when it is the whole input. */
static inline void chunk_cv(const uint8_t *chunk, size_t chunk_len, uint64_t counter, uint8_t root,
                     uint32_t cv[8]) {
  for (size_t i = 0; i < 8; i++) cv[i] = BLAKE3_IV[i];
  size_t nblocks = chunk_len == 0 ? 1 : ((chunk_len - 1) / BLAKE3_BLOCK_LEN + 1);
  for (size_t b = 0; b < nblocks; b++) {
    size_t boff = b * BLAKE3_BLOCK_LEN;
    size_t blen = chunk_len - boff;
    if (blen > BLAKE3_BLOCK_LEN) blen = BLAKE3_BLOCK_LEN;

    /* A full block is read in place; only a short last block is copied and zero-padded. */
    uint8_t block[BLAKE3_BLOCK_LEN];
    const uint8_t *src = chunk + boff;
    if (blen < BLAKE3_BLOCK_LEN) {
      memset(block, 0, sizeof block);
      if (blen > 0) memcpy(block, src, blen);
      src = block;
    }

    uint8_t flags = 0;
    if (b == 0) flags |= BLAKE3_CHUNK_START;
    if (b + 1 == nblocks) flags |= BLAKE3_CHUNK_END | root;

    uint32_t words[16];
    blake3_compress(cv, src, (uint8_t)blen, counter, flags, words);
    for (size_t i = 0; i < 8; i++) cv[i] = words[i];
  }
}

/* out may alias right. */
static void parent_cv(const uint32_t left[8], const uint32_t right[8], uint8_t flags,
                      uint32_t out[8]) {
  uint8_t pblock[BLAKE3_BLOCK_LEN];
  store_words_le(pblock, left, 8);
  store_words_le(pblock + 32, right, 8);
  uint32_t words[16];
  blake3_compress(BLAKE3_IV, pblock, BLAKE3_BLOCK_LEN, 0, BLAKE3_PARENT | flags, words);
  for (size_t i = 0; i < 8; i++) out[i] = words[i];
}

/* A completed non-final chunk: merge one subtree per trailing zero bit of the chunk count. */
static void push_chunk(StigiBlake3 *h, const uint8_t *chunk) {
  uint32_t cv[8];
  chunk_cv(chunk, BLAKE3_CHUNK_LEN, h->chunk_counter, 0, cv);
  h->chunk_counter++;
  for (uint64_t total = h->chunk_counter; (total & 1U) == 0; total >>= 1) {
    h->cv_stack_len--;
    parent_cv(h->cv_stack[h->cv_stack_len], cv, 0, cv);
  }
  memcpy(h->cv_stack[h->cv_stack_len], cv, sizeof cv);
  h->cv_stack_len++;
}

void blake3_init(StigiBlake3 *h) {
  h->cv_stack_len = 0;
  h->chunk_counter = 0;
  h->chunk_len = 0;
}

/* A chunk is pushed only once more input follows it: the last chunk may be the root. */
void blake3_update(StigiBlake3 *h, const uint8_t *in, size_t len) {
  if (in == NULL) return;
  while (len > 0) {
    if (h->chunk_len == BLAKE3_CHUNK_LEN) {
      push_chunk(h, h->chunk);
      h->chunk_len = 0;
    }
    if (h->chunk_len == 0 && len > BLAKE3_CHUNK_LEN) {
      push_chunk(h, in);
      in += BLAKE3_CHUNK_LEN;
      len -= BLAKE3_CHUNK_LEN;
      continue;
    }
    size_t take = BLAKE3_CHUNK_LEN - h->chunk_len;
    if (take > len) take = len;
    memcpy(h->chunk + h->chunk_len, in, take);
    h->chunk_len += take;
    in += take;
    len -= take;
  }
}

/* The last chunk (tail, 0..1024 bytes) and the subtree stack give the root. */
static inline void finalize_from(const StigiBlake3 *h, const uint8_t *tail, size_t tail_len,
                          uint8_t out[32]) {
  uint32_t cv[8];
  chunk_cv(tail, tail_len, h->chunk_counter, h->cv_stack_len == 0 ? BLAKE3_ROOT : 0, cv);
  for (size_t i = h->cv_stack_len; i > 0; i--) {
    parent_cv(h->cv_stack[i - 1], cv, i == 1 ? BLAKE3_ROOT : 0, cv);
  }
  store_words_le(out, cv, 8);
}

void blake3_finalize(const StigiBlake3 *h, uint8_t out[32]) {
  finalize_from(h, h->chunk, h->chunk_len, out);
}

/* One-shot: the same chunk and tree steps as the streaming hasher, with the tail read in place. */
void blake3_hash(const uint8_t *in, size_t len, uint8_t out[32]) {
  if (out == NULL) return;
  if (in == NULL && len > 0) {
    memset(out, 0, 32);
    return;
  }
  StigiBlake3 h;
  blake3_init(&h);
  for (; len > BLAKE3_CHUNK_LEN; in += BLAKE3_CHUNK_LEN, len -= BLAKE3_CHUNK_LEN) push_chunk(&h, in);
  finalize_from(&h, in, len, out);
}
