#include "common.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void diag_clear(Diag *d) {
  if (d == NULL) return;
  d->code = STIGI_OK;
  d->dl_code = -1;
  d->msg[0] = '\0';
}

Status diag_set(Diag *d, Status code, int dl_code, const char *fmt, ...) {
  if (d != NULL) {
    d->code = code;
    d->dl_code = dl_code;
    if (fmt != NULL) {
      va_list ap;
      va_start(ap, fmt);
      (void)vsnprintf(d->msg, sizeof d->msg, fmt, ap);
      va_end(ap);
    } else {
      d->msg[0] = '\0';
    }
  }
  return code;
}

static size_t align_up(size_t n, size_t align) {
  size_t rem = n % align;
  if (rem == 0) return n;
  size_t add = align - rem;
  if (n > SIZE_MAX - add) return SIZE_MAX;
  return n + add;
}

static ArenaBlock *arena_new_block(size_t cap) {
  if (cap > SIZE_MAX - sizeof(ArenaBlock)) return NULL;
  ArenaBlock *b = malloc(sizeof(ArenaBlock) + cap);
  if (b == NULL) return NULL;
  b->next = NULL;
  b->cap = cap;
  b->used = 0;
  return b;
}

void arena_init(Arena *a) {
  if (a == NULL) return;
  a->head = NULL;
  a->cur = NULL;
}

void *arena_alloc_aligned(Arena *a, size_t size, size_t align) {
  if (a == NULL || align == 0 || (align & (align - 1U)) != 0 || align > _Alignof(max_align_t)) {
    return NULL;
  }
  size_t req = size == 0 ? 1 : size;
  size_t aligned = align_up(req, align);
  if (aligned == SIZE_MAX) return NULL;

  if (a->cur != NULL) {
    size_t off = align_up(a->cur->used, align);
    if (off <= a->cur->cap && aligned <= a->cur->cap - off) {
      uint8_t *base = (uint8_t *)(a->cur + 1);
      a->cur->used = off + aligned;
      return base + off;
    }
  }

  bool oversized = aligned > STIGI_ARENA_BLOCK_SIZE;
  size_t bcap = oversized ? aligned : STIGI_ARENA_BLOCK_SIZE;
  ArenaBlock *nb = arena_new_block(bcap);
  if (nb == NULL) return NULL;
  nb->used = aligned;

  if (a->head == NULL) {
    a->head = nb;
    a->cur = nb;
  } else if (oversized) {
    /* Splice oversized block after head without abandoning free space in a->cur */
    nb->next = a->head->next;
    a->head->next = nb;
  } else {
    nb->next = a->cur->next;
    a->cur->next = nb;
    a->cur = nb;
  }
  return (uint8_t *)(nb + 1);
}

void *arena_alloc(Arena *a, size_t size) {
  return arena_alloc_aligned(a, size, _Alignof(max_align_t));
}

void arena_reset(Arena *a) {
  if (a == NULL || a->head == NULL) return;
  if (a->head->cap > STIGI_ARENA_BLOCK_SIZE) {
    arena_free(a);
    return;
  }
  ArenaBlock *b = a->head->next;
  while (b != NULL) {
    ArenaBlock *next = b->next;
    free(b);
    b = next;
  }
  a->head->next = NULL;
  a->head->used = 0;
  a->cur = a->head;
}

void arena_free(Arena *a) {
  if (a == NULL) return;
  ArenaBlock *b = a->head;
  while (b != NULL) {
    ArenaBlock *next = b->next;
    free(b);
    b = next;
  }
  a->head = NULL;
  a->cur = NULL;
}

static uint64_t hash_fnv_splitmix(const char *s, size_t len) {
  uint64_t h = 14695981039346656037ULL;
  for (size_t i = 0; i < len; i++) {
    h ^= (uint8_t)s[i];
    h *= 1099511628211ULL;
  }
  h ^= h >> 30;
  h *= 0xbf58476d1ce4e5b9ULL;
  h ^= h >> 27;
  h *= 0x94d049bb133111ebULL;
  h ^= h >> 31;
  return h;
}

static Status intern_rehash(InternPool *p, uint32_t new_cap, Diag *diag) {
  uint32_t *nslots = calloc(new_cap, sizeof *nslots);
  if (nslots == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "intern_rehash: out of memory");
  uint32_t mask = new_cap - 1U;
  for (uint32_t i = 0; i < p->count; i++) {
    uint32_t idx = (uint32_t)(p->entries[i].hash & mask);
    while (nslots[idx] != 0) idx = (idx + 1U) & mask;
    nslots[idx] = i + 1U;
  }
  free(p->slots);
  p->slots = nslots;
  p->slot_cap = new_cap;
  return STIGI_OK;
}

void intern_init(InternPool *p) {
  if (p == NULL) return;
  arena_init(&p->str_arena);
  p->entries = NULL;
  p->count = 0;
  p->cap = 0;
  p->slots = NULL;
  p->slot_cap = 0;
}

Status intern_str(InternPool *p, const char *s, size_t len, uint32_t *out_id, Diag *diag) {
  if (p == NULL || out_id == NULL || (s == NULL && len > 0)) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "intern_str: invalid arguments");
  }
  if (len >= (size_t)UINT32_MAX) {
    return diag_set(diag, STIGI_ERR_OVERFLOW, -1, "intern_str: string too long");
  }
  const char *src = s != NULL ? s : "";
  uint64_t h = hash_fnv_splitmix(src, len);

  if (p->slot_cap == 0) {
    Status st = intern_rehash(p, 64U, diag);
    if (st != STIGI_OK) return st;
  }

  uint32_t mask = p->slot_cap - 1U;
  uint32_t idx = (uint32_t)(h & mask);
  while (p->slots[idx] != 0) {
    uint32_t id = p->slots[idx] - 1U;
    const InternEntry *e = &p->entries[id];
    if (e->hash == h && e->len == (uint32_t)len && (len == 0 || memcmp(e->str, src, len) == 0)) {
      *out_id = id;
      return STIGI_OK;
    }
    idx = (idx + 1U) & mask;
  }

  if (p->count == UINT32_MAX - 1U) {
    return diag_set(diag, STIGI_ERR_OVERFLOW, -1, "intern_str: pool full");
  }

  if ((uint64_t)(p->count + 1U) * 4ULL >= (uint64_t)p->slot_cap * 3ULL) {
    if (p->slot_cap > UINT32_MAX / 2U) {
      return diag_set(diag, STIGI_ERR_OVERFLOW, -1, "intern_str: slot table overflow");
    }
    Status st = intern_rehash(p, p->slot_cap * 2U, diag);
    if (st != STIGI_OK) return st;
    mask = p->slot_cap - 1U;
    idx = (uint32_t)(h & mask);
    while (p->slots[idx] != 0) idx = (idx + 1U) & mask;
  }

  if (p->count == p->cap) {
    uint32_t ncap = p->cap == 0 ? 64U : (p->cap <= UINT32_MAX / 2U ? p->cap * 2U : UINT32_MAX - 1U);
    if (ncap <= p->cap) return diag_set(diag, STIGI_ERR_OVERFLOW, -1, "intern_str: entry capacity overflow");
    InternEntry *ne = realloc(p->entries, (size_t)ncap * sizeof *ne);
    if (ne == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "intern_str: out of memory");
    p->entries = ne;
    p->cap = ncap;
  }

  char *copy = arena_alloc_aligned(&p->str_arena, len + 1U, 1U);
  if (copy == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "intern_str: arena out of memory");
  if (len > 0) memcpy(copy, src, len);
  copy[len] = '\0';

  uint32_t id = p->count++;
  p->entries[id].str = copy;
  p->entries[id].len = (uint32_t)len;
  p->entries[id].hash = h;
  p->slots[idx] = id + 1U;
  *out_id = id;
  return STIGI_OK;
}

const char *intern_get(const InternPool *p, uint32_t id, size_t *out_len) {
  if (p == NULL || id >= p->count) return NULL;
  if (out_len != NULL) *out_len = (size_t)p->entries[id].len;
  return p->entries[id].str;
}

void intern_reset(InternPool *p) {
  if (p == NULL) return;
  p->count = 0;
  if (p->slots != NULL && p->slot_cap > 0) {
    memset(p->slots, 0, (size_t)p->slot_cap * sizeof *p->slots);
  }
  arena_reset(&p->str_arena);
}

void intern_free(InternPool *p) {
  if (p == NULL) return;
  arena_free(&p->str_arena);
  free(p->entries);
  free(p->slots);
  p->entries = NULL;
  p->slots = NULL;
  p->count = 0;
  p->cap = 0;
  p->slot_cap = 0;
}
