#ifndef STIGI_KANREN_H
#define STIGI_KANREN_H

#include "ast.h"
#include "common.h"
#include "names.h"

#include <stdbool.h>
#include <stdint.h>

#define STIGI_KANREN_FUEL 256U
#define STIGI_KANREN_MAX_DEPTH 16U

typedef enum {
  STIGI_KANREN_FOUND = 0,     /* unique concrete given derivation found */
  STIGI_KANREN_LOCAL_EV,      /* satisfied directly by enclosing {Trait[A]} row */
  STIGI_KANREN_MISSING,       /* 0 derivations -> no-given */
  STIGI_KANREN_AMBIGUOUS,     /* >= 2 distinct derivations -> ambiguous-givens */
  STIGI_KANREN_FUEL_EXCEEDED  /* Bezem non-decreasing / cycle / fuel -> given-fuel */
} StigiKanrenStatus;

typedef struct StigiKanrenProof StigiKanrenProof;
struct StigiKanrenProof {
  StigiKanrenStatus status;
  int32_t given_uidx;         /* concrete STIGI_DECL_GIVEN unit index (for FOUND) */
  uint32_t local_ev_idx;      /* index into local assumptions (for LOCAL_EV) */
  StigiKanrenProof *subproofs;
  uint32_t subproof_count;
};

/* Bezem-bounded uKanren relational proposer (S125).
 * Resolves a trait goal `goal_type` (e.g. Stigi.Equal[U64] or Stigi.Equal[Demo.Maybe[A]])
 * in scope of unit `from_idx` with rigid type params `rigid_tparams` and local assumptions
 * `local_assumps`. Allocates any proof tree nodes in `arena`. */
StigiKanrenProof stigi_kanren_solve(const StigiImage *img, uint32_t from_idx,
                                    const StigiType *goal_type,
                                    const StigiParam *rigid_tparams, uint32_t rigid_count,
                                    const StigiCapParam *local_assumps, uint32_t local_count,
                                    Arena *arena);

/* Check whether a given's {...} clause contains any non-trait effect capability (given-row). */
bool stigi_kanren_given_is_effectful(const StigiImage *img, uint32_t given_uidx);

/* Check whether a type refers to a concrete handler or given instead of a capability/trait protocol. */
bool stigi_kanren_is_concrete_protocol(const StigiImage *img, uint32_t from_idx,
                                       const StigiType *ty);

#endif
