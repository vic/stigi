#ifndef STIGI_PRINT_H
#define STIGI_PRINT_H

#include "ast.h"
#include "common.h"

#include <stddef.h>

Status stigi_print_unit(const StigiUnit *unit, char **out_str, size_t *out_len,
                        Arena *arena, Diag *diag);

#endif
