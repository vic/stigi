#ifndef STIGI_STORE_H
#define STIGI_STORE_H

#include "common.h"
#include "door.h"

#include <stddef.h>

/* Write canonical X/Y/z.sc per unit + sorted index (name, 64-char hex runtime_id, kind) */
Status stigi_store_dir(const StigiDoor *door, const char *dir,
                       char *err_buf, size_t err_cap);

/* Write a named subset of units (or all units when name_count == 0) per door.md:113 */
Status stigi_store_dir_subset(const StigiDoor *door, const char *dir,
                              const char *const *names, uint32_t name_count,
                              char *err_buf, size_t err_cap);

/* Load *.sc tree in one atomic transaction (preserving cached verdicts for unchanged keys) */
Status stigi_load_dir(StigiDoor *door, const char *dir,
                      StigiDoorResult *out, Diag *diag);

#endif
