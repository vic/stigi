#ifndef STIGI_PARSE_H
#define STIGI_PARSE_H

#include "ast.h"
#include "common.h"
#include "lex.h"

#include <stddef.h>

typedef enum {
  STIGI_PARSE_OK = 0,
  STIGI_PARSE_ERR_PARSE = 1,
  STIGI_PARSE_ERR_NOT_IN_LANG = 2
} StigiParseDiagKind;

typedef struct {
  Status status;
  StigiParseDiagKind diag_kind;
  const char *diag_code; /* NULL, "parse", or "not-in-language" */
  const char *help;      /* non-NULL when STIGI_PARSE_ERR_NOT_IN_LANG */
  StigiSpan err_span;
  char msg[256];
  StigiUnit *unit;
} StigiParseResult;

StigiParseResult stigi_parse_unit(const char *src, size_t len, Arena *arena);

#endif
