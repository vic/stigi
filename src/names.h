#ifndef STIGI_NAMES_H
#define STIGI_NAMES_H

#include "ast.h"
#include "common.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
  STIGI_NAME_DUPLICATE = 0,
  STIGI_NAME_UNRESOLVED,
  STIGI_NAME_PRIVATE_REF,
  STIGI_NAME_PUB_ON_TEST,
  STIGI_NAME_PUB_UNUSED,
  STIGI_NAME_NO_READER,
  STIGI_NAME_CASE_PAIR,
  STIGI_NAME_AMBIGUOUS_GROUP
} StigiNameCode;

typedef struct {
  StigiNameCode code;
  StigiSpan span;
  char msg[256];
} StigiNameDiag;

typedef struct {
  StigiNameDiag *items;
  uint32_t count;
  uint32_t cap;
} StigiNameDiags;

typedef struct {
  uint32_t from;
  uint32_t to;
} StigiDepEdge;

typedef struct {
  uint32_t idx;
  uint32_t name_id;
  const char *name;
  size_t name_len;
  StigiUnit *ast;
  uint8_t unit_id[32];     /* runtime computation id (post-erasure, S119) */
  uint8_t verdict_key[32]; /* runtime_id + pre-erasure facts/obligations (S119) */
  bool is_pub;
  bool is_root;
  bool reachable;
  bool used_outside_ns;
} StigiImageUnit;

typedef struct {
  Arena arena;
  InternPool intern;
  StigiImageUnit *units;
  uint32_t unit_count;
  uint32_t unit_cap;
  /* Open-addressed O(1) exact and case-folded name -> unit_idx + 1 tables */
  uint32_t *slots;
  uint32_t slot_cap;
  uint32_t *ci_slots;
  uint32_t ci_slot_cap;
  /* Dependency edges + per-unit adjacency list (O(V+E) traversal) */
  StigiDepEdge *edges;
  uint32_t *edge_next;
  uint32_t edge_count;
  uint32_t edge_cap;
  uint32_t *adj_head;
  uint32_t adj_cap;
} StigiImage;

void stigi_name_diags_init(StigiNameDiags *d);
void stigi_name_diags_free(StigiNameDiags *d);
const char *stigi_name_code_name(StigiNameCode code);

void stigi_image_init(StigiImage *img);
void stigi_image_free(StigiImage *img);

Status stigi_image_add_source(StigiImage *img, const char *src, size_t len,
                              StigiNameDiags *diags, Diag *diag);

/* O(1) exact name lookup; returns unit index or -1 if not found */
int32_t stigi_image_find(const StigiImage *img, const char *name, size_t len);

/* Visibility check: pub or same namespace prefix (names.md 3) */
bool stigi_image_visible_from(const StigiImage *img, uint32_t target_idx, uint32_t from_idx);

/* Resolve all references, build dependency graph, check visibility/roots/pub-unused */
Status stigi_image_resolve(StigiImage *img, StigiNameDiags *diags, Diag *diag);

/* Resolve a reference string from a given unit using aliases + namepath search.
 * Returns target unit index, or -1 if not found in image. */
int32_t stigi_image_resolve_name(const StigiImage *img, uint32_t from_idx,
                                 const char *ref, size_t ref_len);

#endif
