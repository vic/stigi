#ifndef STIGI_STORE_H
#define STIGI_STORE_H

#include "common.h"
#include "door.h"

#include <stddef.h>

/* Write canonical X/Y/z.sc per unit + sorted index (name, 64-char hex runtime_id, kind) */
Status stigi_store_dir(const StigiDoor *door, const char *dir,
                       char *err_buf, size_t err_cap);

/* Load *.sc tree in one atomic transaction (preserving cached verdicts for unchanged keys) */
Status stigi_load_dir(StigiDoor *door, const char *dir,
                      StigiDoorResult *out, Diag *diag);

#endif
