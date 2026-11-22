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
  STIGI_TYPE_TYPE_ARITY,
  STIGI_TYPE_PATTERN_ARITY,
  STIGI_TYPE_CANNOT_INFER,
  STIGI_TYPE_HANDLER_SIGNATURE,
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

#define STIGI_LINEAR_MAX_TERMS 32

/* Linear length-term normal form: c + sum_i (coeff_i * var_i), sorted by var */
typedef struct {
  int64_t c;
  struct {
    uint32_t var;
    int64_t coeff;
  } terms[STIGI_LINEAR_MAX_TERMS];
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
  STIGI_TY_INT_LIT,
  STIGI_TY_FILL_RESULT,
  STIGI_TY_GROW_RESULT,
  STIGI_TY_MAKE_RESULT,
  STIGI_TY_TEST_RESULT,
  STIGI_TY_ERROR,
  STIGI_TY_UNKNOWN
} StigiTyKind;

typedef struct StigiTy StigiTy;

struct StigiTy {
  StigiTyKind kind;
  uint32_t unit_idx; /* for ENUM / TYPEDEF */
  StigiTy *elem;     /* for ARRAY / CELL / PTR / TYPEDEF / FILL/GROW/MAKE_RESULT */
  StigiLinear len;   /* for ARRAY / PTR / FILL_RESULT / GROW_RESULT new cap */
  bool has_len;
  StigiLinear old_len; /* for GROW_RESULT GrowFailed(kept) old cap */
  bool has_old_len;
  uint64_t lit_val;  /* for INT_LIT */
  bool lit_neg;
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

typedef struct {
  StigiLayout layout;
  bool is_nullable_ptr;
  uint32_t null_case;
  uint32_t ptr_case;
  bool is_tag_only;
  bool tag_in_tail;
  uint64_t tag_size;
  uint64_t tag_offset;
  uint64_t union_align;
  uint64_t union_size;
} StigiEnumLayoutInfo;

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

/* Type equality, directional assignability (Never is bottom), & layout */
bool stigi_ty_equal(const StigiTy *a, const StigiTy *b);
bool stigi_ty_assignable(const StigiTy *expected, const StigiTy *actual);
StigiLayout stigi_ty_layout(const StigiImage *img, const StigiTy *ty);
StigiEnumLayoutInfo stigi_enum_layout_info(const StigiImage *img, uint32_t enum_uidx,
                                           StigiTy *const *type_args, uint32_t type_arg_count);
Status stigi_def_frame_bytes(const StigiImage *img, uint32_t unit_idx, uint64_t *out_bytes);

/* Full image type checker */
Status stigi_typecheck_image(const StigiImage *img, StigiTypeDiags *diags, Diag *diag);

#endif
