#ifndef STIGI_EXTRACT_H
#define STIGI_EXTRACT_H

#include "ast.h"
#include "common.h"
#include "datalog.h"
#include "names.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define STIGI_MAX_FACT_COLS 6

typedef struct {
  bool is_num;
  TupuI128 num;
  const char *sym;
} StigiFactCol;

typedef struct {
  const char *rel;
  int ncols;
  StigiFactCol cols[STIGI_MAX_FACT_COLS];
} StigiFactRow;

typedef struct {
  Arena arena;
  StigiFactRow *rows;
  uint32_t count;
  uint32_t cap;
  uint32_t next_span; /* next free preorder number: instance roots take disjoint ranges (scopeSpan, S138) */
} StigiFactSet;

void stigi_facts_init(StigiFactSet *fs);
void stigi_facts_free(StigiFactSet *fs);

/* Helper to append symbol facts from the door (imageMode, Test/NegTest verdicts, faultReach) */
Status stigi_facts_add_sym(StigiFactSet *fs, const char *rel, int ncols,
                           const char *const *syms, Diag *diag);

/* Single-pass extraction over one unit, dependency-closed unit subgraph, or the entire image */
Status stigi_extract_unit(const StigiImage *img, uint32_t unit_idx,
                          StigiFactSet *fs, Diag *diag);
Status stigi_extract_unit_closed(const StigiImage *img, uint32_t unit_idx,
                                 bool kernel_mode, StigiFactSet *fs, Diag *diag);
Status stigi_extract_image(const StigiImage *img, StigiFactSet *fs, Diag *diag);

/* Load extracted facts directly into a reset StigiDl session using exact 128-bit integers */
Status stigi_facts_load_into_dl(const StigiFactSet *fs, StigiDl *dl, Diag *diag);

/* Format exact 128-bit signed integer into decimal string */
size_t stigi_i128_format(TupuI128 v, char out[48]);

/* Canonical sorted reprint oracle */
Status stigi_facts_print(const StigiFactSet *fs, char **out_str, size_t *out_len,
                         Arena *arena, Diag *diag);

/* Export .facts files to a directory (creates empty files for all .input rels in dl_src) */
Status stigi_facts_write_dir(const StigiFactSet *fs, const char *dir,
                             const char *dl_src, Diag *diag);

#endif
