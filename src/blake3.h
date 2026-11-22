#ifndef STIGI_BLAKE3_H
#define STIGI_BLAKE3_H

#include <stddef.h>
#include <stdint.h>

#define STIGI_BLAKE3_OUT_LEN 32U

void blake3_hash(const uint8_t *in, size_t len, uint8_t out[32]);

#endif
