#include "datalog.h"

Status stigi_dl_open(StigiDl *dl, const char *src, size_t len, const char *name,
                     uint64_t budget, size_t nthreads, Diag *diag) {
  if (dl == NULL || (src == NULL && len > 0)) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_dl_open: invalid arguments");
  }
  dl->sess = NULL;
  char ebuf[256];
  int ecode = 0;
  TupuSession *s = tupu_open(src, len, name, budget, nthreads, ebuf, sizeof ebuf, &ecode);
  if (s == NULL) {
    return diag_set(diag, STIGI_ERR_DATALOG, ecode, "%s", ebuf[0] != '\0' ? ebuf : "tupu_open failed");
  }
  dl->sess = s;
  diag_clear(diag);
  return STIGI_OK;
}

void stigi_dl_close(StigiDl *dl) {
  if (dl == NULL) return;
  if (dl->sess != NULL) {
    tupu_close(dl->sess);
    dl->sess = NULL;
  }
}

Status stigi_dl_reset(StigiDl *dl, Diag *diag) {
  if (dl == NULL || dl->sess == NULL) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_dl_reset: null session");
  }
  if (tupu_reset(dl->sess) != 0) {
    return diag_set(diag, STIGI_ERR_DATALOG, -1, "tupu_reset failed");
  }
  return STIGI_OK;
}

int stigi_dl_rel_id(const StigiDl *dl, const char *name) {
  if (dl == NULL || dl->sess == NULL || name == NULL) return -1;
  return tupu_rel_id(dl->sess, name);
}

int stigi_dl_rel_ncols(const StigiDl *dl, int rel_id) {
  if (dl == NULL || dl->sess == NULL) return -1;
  return tupu_rel_ncols(dl->sess, rel_id);
}

Status stigi_dl_intern(StigiDl *dl, const char *str, size_t len, uint32_t *out_id, Diag *diag) {
  if (dl == NULL || dl->sess == NULL || out_id == NULL || (str == NULL && len > 0)) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_dl_intern: invalid arguments");
  }
  if (tupu_intern(dl->sess, str, len, out_id) != 0) {
    return diag_set(diag, STIGI_ERR_DATALOG, -1, "tupu_intern failed");
  }
  return STIGI_OK;
}

const char *stigi_dl_sym_str(const StigiDl *dl, uint32_t id, size_t *out_len) {
  if (dl == NULL || dl->sess == NULL) return NULL;
  return tupu_sym_str(dl->sess, id, out_len);
}

Status stigi_dl_add(StigiDl *dl, int rel_id, const int64_t *vals, int ncols, Diag *diag) {
  if (dl == NULL || dl->sess == NULL || (ncols > 0 && vals == NULL)) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_dl_add: invalid arguments");
  }
  if (tupu_add_row(dl->sess, rel_id, vals, ncols) != 0) {
    return diag_set(diag, STIGI_ERR_DATALOG, -1, "tupu_add_row failed (rel=%d, ncols=%d)", rel_id, ncols);
  }
  return STIGI_OK;
}

Status stigi_dl_eval(StigiDl *dl, Diag *diag) {
  if (dl == NULL || dl->sess == NULL) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_dl_eval: null session");
  }
  char ebuf[256];
  int ecode = 0;
  if (tupu_eval(dl->sess, &ecode, ebuf, sizeof ebuf) != 0) {
    return diag_set(diag, STIGI_ERR_DATALOG, ecode, "%s", ebuf[0] != '\0' ? ebuf : "tupu_eval failed");
  }
  return STIGI_OK;
}

uint32_t stigi_dl_count(const StigiDl *dl, int rel_id) {
  if (dl == NULL || dl->sess == NULL) return 0;
  return tupu_rel_count(dl->sess, rel_id);
}

Status stigi_dl_read(const StigiDl *dl, int rel_id, uint32_t row, int64_t *out_vals, int ncols, Diag *diag) {
  if (dl == NULL || dl->sess == NULL || (ncols > 0 && out_vals == NULL)) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_dl_read: invalid arguments");
  }
  if (tupu_rel_read_row(dl->sess, rel_id, row, out_vals, ncols) != 0) {
    return diag_set(diag, STIGI_ERR_DATALOG, -1, "tupu_rel_read_row failed (rel=%d, row=%u)", rel_id, row);
  }
  return STIGI_OK;
}
