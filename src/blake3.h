#ifndef STIGI_BLAKE3_H
#define STIGI_BLAKE3_H

#include <stddef.h>
#include <stdint.h>

#define STIGI_BLAKE3_OUT_LEN 32U

/* Streaming hasher: no heap. cv_stack holds one chaining value per tree level (2^64 chunks max). */
typedef struct {
  uint32_t cv_stack[64][8];
  size_t cv_stack_len;
  uint64_t chunk_counter; /* chunks already pushed */
  uint8_t chunk[1024];
  size_t chunk_len;
} StigiBlake3;

void blake3_init(StigiBlake3 *h);
void blake3_update(StigiBlake3 *h, const uint8_t *in, size_t len);
void blake3_finalize(const StigiBlake3 *h, uint8_t out[32]);
void blake3_hash(const uint8_t *in, size_t len, uint8_t out[32]);

#endif
