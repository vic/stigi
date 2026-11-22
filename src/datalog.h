#ifndef STIGI_DATALOG_H
#define STIGI_DATALOG_H

#include "common.h"
#include <tupu.h>

typedef struct {
  TupuSession *sess;
} StigiDl;

Status stigi_dl_open(StigiDl *dl, const char *src, size_t len, const char *name,
                     uint64_t budget, size_t nthreads, Diag *diag);
Status stigi_dl_open_ex(StigiDl *dl, const char *src, size_t len, const char *name,
                        uint64_t budget, int prov, size_t nthreads, Diag *diag);
void stigi_dl_close(StigiDl *dl);
Status stigi_dl_reset(StigiDl *dl, Diag *diag);
int stigi_dl_rel_id(const StigiDl *dl, const char *name);
int stigi_dl_rel_ncols(const StigiDl *dl, int rel_id);
Status stigi_dl_intern(StigiDl *dl, const char *str, size_t len, uint32_t *out_id, Diag *diag);
const char *stigi_dl_sym_str(const StigiDl *dl, uint32_t id, size_t *out_len);
Status stigi_dl_add(StigiDl *dl, int rel_id, const int64_t *vals, int ncols, Diag *diag);
Status stigi_dl_add_i128(StigiDl *dl, int rel_id, const TupuI128 *vals, int ncols, Diag *diag);
Status stigi_dl_eval(StigiDl *dl, Diag *diag);
Status stigi_dl_replay(StigiDl *dl, Diag *diag);
uint32_t stigi_dl_count(const StigiDl *dl, int rel_id);
Status stigi_dl_read(const StigiDl *dl, int rel_id, uint32_t row, int64_t *out_vals, int ncols, Diag *diag);
Status stigi_dl_read_i128(const StigiDl *dl, int rel_id, uint32_t row, TupuI128 *out_vals, int ncols, Diag *diag);

#endif
