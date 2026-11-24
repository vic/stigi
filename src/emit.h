#ifndef STIGI_EMIT_H
#define STIGI_EMIT_H

#include "common.h"
#include "door.h"
#include "names.h"

#include <stddef.h>
#include <stdint.h>

typedef struct {
  uint32_t units_emitted;
  uint32_t defs_emitted;
  uint32_t enums_emitted;
  uint32_t lemmas_erased;
  uint32_t spectre_masks;
  uint32_t tail_loops;
  uint32_t free_edges;
  uint32_t free_at_exit;
  uint32_t reuses;
  uint32_t stack_objects;
  uint32_t pure_defs;
  uint32_t parallel_safe_sites;
} StigiEmitStats;

/* S163: RELEASE emits the closure of `main` only (no Test runs); TEST emits every reached
 * unit and runs every Test in unit-id order before `main` */
typedef enum { STIGI_EMIT_RELEASE, STIGI_EMIT_TEST } StigiEmitProfile;

/* Deterministically emit portable ISO C11 for the profile's units (sorted by 32-byte runtime_id) */
Status stigi_emit_c(const StigiDoor *door, StigiEmitProfile profile, char **out_c, size_t *out_len,
                    StigiEmitStats *stats, Diag *diag);

/* Parse GCC/Clang -fstack-usage .su file and verify every emitted function is static
 * and its stack usage is within its stated frameBytes budget */
Status stigi_emit_check_su(const char *su_path, const StigiImage *img, StigiEmitProfile profile,
                           char *err_buf, size_t err_cap);

#endif
