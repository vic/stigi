#ifndef STIGI_TYPES_H
#define STIGI_TYPES_H

#include "ast.h"
#include "common.h"
#include "names.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
  STIGI_TYPE_MISMATCH = 0,
  STIGI_TYPE_ARITY,
  STIGI_TYPE_ASSIGN_VAL,
  STIGI_TYPE_NOT_EXHAUSTIVE,
  STIGI_TYPE_WILDCARD_ARM,
  STIGI_TYPE_NESTED_PATTERN,
  STIGI_TYPE_RECURSIVE_TYPE,
  STIGI_TYPE_EXTERN_FORGES_FACTS,
  STIGI_TYPE_LITERAL_OUT_OF_RANGE,
  STIGI_TYPE_LENGTH_NOT_LINEAR
} StigiTypeCode;

typedef struct {
  StigiTypeCode code;
  StigiSpan span;
  char msg[256];
} StigiTypeDiag;

typedef struct {
  StigiTypeDiag *items;
  uint32_t count;
  uint32_t cap;
} StigiTypeDiags;

/* Linear length-term normal form: c + sum_i (coeff_i * var_i), sorted by var */
typedef struct {
  int64_t c;
  struct {
    uint32_t var;
    int64_t coeff;
  } terms[8];
  int nterms;
} StigiLinear;

typedef enum {
  STIGI_TY_U8 = 0,
  STIGI_TY_U32,
  STIGI_TY_U64,
  STIGI_TY_I32,
  STIGI_TY_I64,
  STIGI_TY_BOOL,
  STIGI_TY_UNIT,
  STIGI_TY_NEVER,
  STIGI_TY_TYPE,
  STIGI_TY_ARRAY,
  STIGI_TY_CELL,
  STIGI_TY_PTR,
  STIGI_TY_ENUM,
  STIGI_TY_TYPEDEF,
  STIGI_TY_VAR,
  STIGI_TY_TEST_RESULT,
  STIGI_TY_UNKNOWN
} StigiTyKind;

typedef struct StigiTy StigiTy;

struct StigiTy {
  StigiTyKind kind;
  uint32_t unit_idx; /* for ENUM / TYPEDEF */
  StigiTy *elem;     /* for ARRAY / CELL / PTR / TYPEDEF underlying */
  StigiLinear len;   /* for ARRAY / PTR */
  bool has_len;
  uint32_t var_id;   /* for VAR */
  StigiTy **type_args;
  uint32_t type_arg_count;
  StigiLinear *len_args;
  uint32_t len_arg_count;
};

typedef struct {
  uint64_t size;
  uint64_t align;
} StigiLayout;

void stigi_type_diags_init(StigiTypeDiags *d);
void stigi_type_diags_free(StigiTypeDiags *d);
const char *stigi_type_code_name(StigiTypeCode code);

/* Linear normalizer */
StigiLinear stigi_linear_const(int64_t c);
StigiLinear stigi_linear_var(uint32_t var);
Status stigi_linear_add(const StigiLinear *a, const StigiLinear *b, StigiLinear *out);
Status stigi_linear_sub(const StigiLinear *a, const StigiLinear *b, StigiLinear *out);
Status stigi_linear_mul(const StigiLinear *a, const StigiLinear *b, StigiLinear *out);
bool stigi_linear_equal(const StigiLinear *a, const StigiLinear *b);
bool stigi_linear_offset(const StigiLinear *a, const StigiLinear *b, int64_t *out_delta);
Status stigi_linear_from_expr(const StigiExpr *e, InternPool *intern, StigiLinear *out);

/* Type equality & layout */
bool stigi_ty_equal(const StigiTy *a, const StigiTy *b);
StigiLayout stigi_ty_layout(const StigiImage *img, const StigiTy *ty);
Status stigi_def_frame_bytes(const StigiImage *img, uint32_t unit_idx, uint64_t *out_bytes);

/* Full image type checker */
Status stigi_typecheck_image(const StigiImage *img, StigiTypeDiags *diags, Diag *diag);

#endif
