#ifndef STIGI_EVAL_H
#define STIGI_EVAL_H

#include "ast.h"
#include "common.h"
#include "names.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define STIGI_DEFAULT_FUEL 1000000ULL

typedef enum {
  STIGI_VAL_UNIT = 0,
  STIGI_VAL_BOOL,
  STIGI_VAL_INT,
  STIGI_VAL_STR,
  STIGI_VAL_ENUM,
  STIGI_VAL_ARRAY,
  STIGI_VAL_TEST_OK
} StigiValKind;

typedef enum {
  STIGI_INT_UNTYPED = 0,
  STIGI_INT_U8,
  STIGI_INT_U32,
  STIGI_INT_U64,
  STIGI_INT_I32,
  STIGI_INT_I64
} StigiIntWidth;

typedef struct StigiVal StigiVal;

struct StigiVal {
  StigiValKind kind;
  bool is_unsigned;
  StigiIntWidth int_width;
  union {
    bool b;
    int64_t i;
    uint64_t u;
    StigiStr s;
    struct {
      int32_t enum_unit_idx;
      StigiStr case_name;
      StigiVal *fields;
      uint32_t field_count;
    } en;
    struct {
      StigiVal *elems;
      uint32_t len;
    } arr;
  } u;
};

typedef struct {
  Status status;
  const char *diag_code; /* NULL, "evaluation-fuel", "not-compile-time", "test-failed", "eval-error" */
  char msg[256];
  StigiVal value;
  uint64_t fuel_used;
} StigiEvalResult;

StigiEvalResult stigi_eval_unit(const StigiImage *img, uint32_t unit_idx,
                                uint64_t fuel_max, Arena *arena);
StigiEvalResult stigi_eval_test(const StigiImage *img, uint32_t unit_idx,
                                uint64_t fuel_max, Arena *arena);

#endif
