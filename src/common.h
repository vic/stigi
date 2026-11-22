#ifndef STIGI_COMMON_H
#define STIGI_COMMON_H

#include <stddef.h>
#include <stdint.h>

#define STIGI_ARENA_BLOCK_SIZE (64U * 1024U)

typedef enum {
  STIGI_OK = 0,
  STIGI_ERR_INVALID = 1,
  STIGI_ERR_OOM = 2,
  STIGI_ERR_OVERFLOW = 3,
  STIGI_ERR_DATALOG = 4,
  STIGI_ERR_IO = 5
} Status;

typedef struct {
  Status code;
  int dl_code;
  char msg[256];
} Diag;

void diag_clear(Diag *d);
Status diag_set(Diag *d, Status code, int dl_code, const char *fmt, ...);

typedef struct ArenaBlock {
  _Alignas(max_align_t) struct ArenaBlock *next;
  size_t cap;
  size_t used;
} ArenaBlock;

typedef struct {
  ArenaBlock *head;
  ArenaBlock *cur;
} Arena;

#define arena_init stigi_arena_init
#define arena_alloc stigi_arena_alloc
#define arena_alloc_aligned stigi_arena_alloc_aligned
#define arena_reset stigi_arena_reset
#define arena_free stigi_arena_free

void arena_init(Arena *a);
void *arena_alloc(Arena *a, size_t size);
void *arena_alloc_aligned(Arena *a, size_t size, size_t align);
void arena_reset(Arena *a);
void arena_free(Arena *a);

typedef struct {
  const char *str;
  uint32_t len;
  uint64_t hash;
} InternEntry;

typedef struct {
  Arena str_arena;
  InternEntry *entries;
  uint32_t count;
  uint32_t cap;
  uint32_t *slots;
  uint32_t slot_cap;
} InternPool;

void intern_init(InternPool *p);
Status intern_str(InternPool *p, const char *s, size_t len, uint32_t *out_id, Diag *diag);
const char *intern_get(const InternPool *p, uint32_t id, size_t *out_len);
void intern_reset(InternPool *p);
void intern_free(InternPool *p);

#endif
