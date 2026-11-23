#include "types.h"
#include "eval.h"
#include "pattern.h"

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void stigi_type_diags_init(StigiTypeDiags *d) {
  if (d == NULL) return;
  d->items = NULL;
  d->count = 0;
  d->cap = 0;
}

void stigi_type_diags_free(StigiTypeDiags *d) {
  if (d == NULL) return;
  free(d->items);
  d->items = NULL;
  d->count = 0;
  d->cap = 0;
}

const char *stigi_type_code_name(StigiTypeCode code) {
  switch (code) {
    case STIGI_TYPE_MISMATCH: return "mismatch";
    case STIGI_TYPE_ARITY: return "arity";
    case STIGI_TYPE_TYPE_ARITY: return "type-arity";
    case STIGI_TYPE_PATTERN_ARITY: return "pattern-arity";
    case STIGI_TYPE_CANNOT_INFER: return "cannot-infer";
    case STIGI_TYPE_HANDLER_SIGNATURE: return "handler-signature";
    case STIGI_TYPE_ASSIGN_VAL: return "assign-val";
    case STIGI_TYPE_NOT_EXHAUSTIVE: return "not-exhaustive";
    case STIGI_TYPE_WILDCARD_ARM: return "wildcard-arm";
    case STIGI_TYPE_NESTED_PATTERN: return "nested-pattern";
    case STIGI_TYPE_RECURSIVE_TYPE: return "recursive-type";
    case STIGI_TYPE_EXTERN_FORGES_FACTS: return "extern-forges-facts";
    case STIGI_TYPE_LITERAL_OUT_OF_RANGE: return "literal-out-of-range";
    case STIGI_TYPE_LENGTH_NOT_LINEAR: return "length-not-linear";
    case STIGI_TYPE_NEVER_REACHED: return "never-reached";
  }
  return "unknown";
}

static Status add_tdiag(StigiTypeDiags *d, StigiTypeCode code, StigiSpan span,
                        Diag *diag, const char *fmt, ...) {
  if (d == NULL) return STIGI_OK;
  if (d->count == d->cap) {
    uint32_t ncap = d->cap == 0 ? 8U : d->cap * 2U;
    if (ncap <= d->cap) return diag_set(diag, STIGI_ERR_OVERFLOW, -1, "type diags overflow");
    StigiTypeDiag *ni = realloc(d->items, (size_t)ncap * sizeof *ni);
    if (ni == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "type diags out of memory");
    d->items = ni;
    d->cap = ncap;
  }
  StigiTypeDiag *item = &d->items[d->count++];
  item->code = code;
  item->span = span;
  va_list ap;
  va_start(ap, fmt);
  (void)vsnprintf(item->msg, sizeof item->msg, fmt, ap);
  va_end(ap);
  return STIGI_OK;
}

/* ---- Linear length-term normalizer ---- */

StigiLinear stigi_linear_const(int64_t c) {
  StigiLinear l;
  memset(&l, 0, sizeof l);
  l.c = c;
  l.nterms = 0;
  return l;
}

StigiLinear stigi_linear_var(uint32_t var) {
  StigiLinear l;
  memset(&l, 0, sizeof l);
  l.c = 0;
  l.terms[0].var = var;
  l.terms[0].coeff = 1;
  l.nterms = 1;
  return l;
}

static bool i64_add(int64_t a, int64_t b, int64_t *out) {
  if ((b > 0 && a > INT64_MAX - b) || (b < 0 && a < INT64_MIN - b)) return false;
  *out = a + b;
  return true;
}

static bool i64_sub(int64_t a, int64_t b, int64_t *out) {
  if ((b < 0 && a > INT64_MAX + b) || (b > 0 && a < INT64_MIN + b)) return false;
  *out = a - b;
  return true;
}

static bool i64_mul(int64_t a, int64_t b, int64_t *out) {
  if (a == 0 || b == 0) { *out = 0; return true; }
  if ((a == -1 && b == INT64_MIN) || (b == -1 && a == INT64_MIN)) return false;
  if (a > 0) {
    if (b > 0) { if (a > INT64_MAX / b) return false; }
    else { if (b < INT64_MIN / a) return false; }
  } else {
    if (b > 0) { if (a < INT64_MIN / b) return false; }
    else { if (a < INT64_MAX / b) return false; }
  }
  *out = a * b;
  return true;
}

static Status linear_combine(const StigiLinear *a, const StigiLinear *b,
                             int64_t b_sign, StigiLinear *out) {
  if (a == NULL || b == NULL || out == NULL) return STIGI_ERR_INVALID;
  StigiLinear r;
  memset(&r, 0, sizeof r);
  int64_t bc = 0;
  if (!i64_mul(b_sign, b->c, &bc) || !i64_add(a->c, bc, &r.c)) return STIGI_ERR_OVERFLOW;
  int i = 0, j = 0;
  while (i < a->nterms || j < b->nterms) {
    uint32_t v = 0;
    int64_t coef = 0;
    if (j >= b->nterms || (i < a->nterms && a->terms[i].var < b->terms[j].var)) {
      v = a->terms[i].var;
      coef = a->terms[i].coeff;
      i++;
    } else if (i >= a->nterms || b->terms[j].var < a->terms[i].var) {
      v = b->terms[j].var;
      if (!i64_mul(b_sign, b->terms[j].coeff, &coef)) return STIGI_ERR_OVERFLOW;
      j++;
    } else {
      v = a->terms[i].var;
      int64_t sb = 0;
      if (!i64_mul(b_sign, b->terms[j].coeff, &sb) || !i64_add(a->terms[i].coeff, sb, &coef)) {
        return STIGI_ERR_OVERFLOW;
      }
      i++;
      j++;
    }
    if (coef != 0) {
      if (r.nterms >= STIGI_LINEAR_MAX_TERMS) return STIGI_ERR_OVERFLOW;
      r.terms[r.nterms].var = v;
      r.terms[r.nterms].coeff = coef;
      r.nterms++;
    }
  }
  *out = r;
  return STIGI_OK;
}

Status stigi_linear_add(const StigiLinear *a, const StigiLinear *b, StigiLinear *out) {
  return linear_combine(a, b, 1, out);
}

Status stigi_linear_sub(const StigiLinear *a, const StigiLinear *b, StigiLinear *out) {
  return linear_combine(a, b, -1, out);
}

Status stigi_linear_mul(const StigiLinear *a, const StigiLinear *b, StigiLinear *out) {
  if (a == NULL || b == NULL || out == NULL) return STIGI_ERR_INVALID;
  if (a->nterms > 0 && b->nterms > 0) {
    return STIGI_ERR_INVALID;
  }
  const StigiLinear *lin = (a->nterms > 0) ? a : b;
  int64_t k = (a->nterms > 0) ? b->c : a->c;
  StigiLinear r;
  memset(&r, 0, sizeof r);
  if (!i64_mul(lin->c, k, &r.c)) return STIGI_ERR_OVERFLOW;
  if (k != 0) {
    for (int i = 0; i < lin->nterms; i++) {
      int64_t nc = 0;
      if (!i64_mul(lin->terms[i].coeff, k, &nc)) return STIGI_ERR_OVERFLOW;
      r.terms[r.nterms].var = lin->terms[i].var;
      r.terms[r.nterms].coeff = nc;
      r.nterms++;
    }
  }
  *out = r;
  return STIGI_OK;
}

bool stigi_linear_equal(const StigiLinear *a, const StigiLinear *b) {
  if (a == NULL || b == NULL) return false;
  if (a->c != b->c || a->nterms != b->nterms) return false;
  for (int i = 0; i < a->nterms; i++) {
    if (a->terms[i].var != b->terms[i].var || a->terms[i].coeff != b->terms[i].coeff) {
      return false;
    }
  }
  return true;
}

bool stigi_linear_offset(const StigiLinear *a, const StigiLinear *b, int64_t *out_delta) {
  if (a == NULL || b == NULL) return false;
  if (a->nterms != b->nterms) return false;
  for (int i = 0; i < a->nterms; i++) {
    if (a->terms[i].var != b->terms[i].var || a->terms[i].coeff != b->terms[i].coeff) {
      return false;
    }
  }
  int64_t delta = 0;
  if (!i64_sub(a->c, b->c, &delta)) return false;
  if (out_delta != NULL) *out_delta = delta;
  return true;
}

Status stigi_linear_from_expr(const StigiExpr *e, InternPool *intern, StigiLinear *out) {
  if (e == NULL || intern == NULL || out == NULL) return STIGI_ERR_INVALID;
  switch (e->kind) {
    case STIGI_EXPR_LIT_INT:
      if (e->u.lit_int.overflow || e->u.lit_int.value > (uint64_t)INT64_MAX) {
        return STIGI_ERR_OVERFLOW;
      }
      *out = stigi_linear_const((int64_t)e->u.lit_int.value);
      return STIGI_OK;
    case STIGI_EXPR_LOCAL:
    case STIGI_EXPR_GLOBAL: {
      uint32_t id = 0;
      Diag d;
      diag_clear(&d);
      Status st = intern_str(intern, e->u.ident.name.ptr, e->u.ident.name.len, &id, &d);
      if (st != STIGI_OK) return st;
      *out = stigi_linear_var(id);
      return STIGI_OK;
    }
    case STIGI_EXPR_UNOP:
      if (e->u.unop.op == STIGI_OP_NEG) {
        StigiLinear inner;
        Status st = stigi_linear_from_expr(e->u.unop.operand, intern, &inner);
        if (st != STIGI_OK) return st;
        StigiLinear neg1 = stigi_linear_const(-1);
        return stigi_linear_mul(&neg1, &inner, out);
      }
      return STIGI_ERR_INVALID;
    case STIGI_EXPR_BINOP: {
      StigiLinear l, r;
      Status st = stigi_linear_from_expr(e->u.binop.lhs, intern, &l);
      if (st != STIGI_OK) return st;
      st = stigi_linear_from_expr(e->u.binop.rhs, intern, &r);
      if (st != STIGI_OK) return st;
      if (e->u.binop.op == STIGI_OP_ADD) return stigi_linear_add(&l, &r, out);
      if (e->u.binop.op == STIGI_OP_SUB) return stigi_linear_sub(&l, &r, out);
      if (e->u.binop.op == STIGI_OP_MUL) return stigi_linear_mul(&l, &r, out);
      if (e->u.binop.op == STIGI_OP_DIV && l.nterms == 0 && r.nterms == 0 && r.c != 0) {
        if (l.c == INT64_MIN && r.c == -1) return STIGI_ERR_OVERFLOW;
        *out = stigi_linear_const(l.c / r.c);
        return STIGI_OK;
      }
      if (e->u.binop.op == STIGI_OP_MOD && l.nterms == 0 && r.nterms == 0 && r.c != 0) {
        if (l.c == INT64_MIN && r.c == -1) return STIGI_ERR_OVERFLOW;
        *out = stigi_linear_const(l.c % r.c);
        return STIGI_OK;
      }
      return STIGI_ERR_INVALID;
    }
    default:
      return STIGI_ERR_INVALID;
  }
}

/* ---- Type helpers, equality, assignability & layout ---- */

static bool str_eq_c(StigiStr s, const char *lit) {
  size_t n = strlen(lit);
  return s.len == n && memcmp(s.ptr, lit, n) == 0;
}

static bool str_eq_ss(StigiStr a, StigiStr b) {
  return a.len == b.len && (a.len == 0 || memcmp(a.ptr, b.ptr, a.len) == 0);
}

static bool is_int_ty(const StigiTy *t) {
  if (t == NULL) return false;
  return t->kind == STIGI_TY_U8 || t->kind == STIGI_TY_U32 || t->kind == STIGI_TY_U64 ||
         t->kind == STIGI_TY_I32 || t->kind == STIGI_TY_I64 || t->kind == STIGI_TY_INT_LIT;
}

static bool int_lit_fits_ty(uint64_t val, bool is_neg, const StigiTy *u) {
  if (u == NULL) return false;
  switch (u->kind) {
    case STIGI_TY_U8: return !is_neg && val <= 255ULL;
    case STIGI_TY_U32: return !is_neg && val <= 4294967295ULL;
    case STIGI_TY_U64: return !is_neg;
    case STIGI_TY_I32:
      return is_neg ? (val <= 2147483648ULL) : (val <= 2147483647ULL);
    case STIGI_TY_I64:
      return is_neg ? (val <= 9223372036854775808ULL) : (val <= 9223372036854775807ULL);
    case STIGI_TY_INT_LIT:
      return true;
    default:
      return false;
  }
}

static bool is_unsigned_int_ty(const StigiTy *t) {
  if (t == NULL) return false;
  return t->kind == STIGI_TY_U8 || t->kind == STIGI_TY_U32 || t->kind == STIGI_TY_U64;
}

static const StigiTy *unwrap_ty_underlying(const StigiTy *t) {
  int d = 0;
  while (t != NULL && t->kind == STIGI_TY_TYPEDEF && t->elem != NULL && d++ < 16) {
    t = t->elem;
  }
  return t;
}

static bool is_numeric_ty(const StigiTy *t) {
  return is_int_ty(unwrap_ty_underlying(t));
}

/* Strict structural normal-form equality: STIGI_TY_NEVER is NOT equal to non-Never,
 * and STIGI_TY_UNKNOWN is NOT an accept-all wildcard! */
bool stigi_ty_equal(const StigiTy *a, const StigiTy *b) {
  if (a == b) return true;
  if (a == NULL || b == NULL) return false;
  if (a->kind == STIGI_TY_ERROR || b->kind == STIGI_TY_ERROR) return true;
  if (a->kind == STIGI_TY_UNKNOWN || b->kind == STIGI_TY_UNKNOWN) return false;
  if (a->kind != b->kind) return false;
  switch (a->kind) {
    case STIGI_TY_ENUM:
    case STIGI_TY_TYPEDEF:
      if (a->unit_idx != b->unit_idx) return false;
      if (a->type_arg_count != b->type_arg_count) return false;
      for (uint32_t i = 0; i < a->type_arg_count; i++) {
        if (!stigi_ty_equal(a->type_args[i], b->type_args[i])) return false;
      }
      if (a->len_arg_count != b->len_arg_count) return false;
      for (uint32_t i = 0; i < a->len_arg_count; i++) {
        if (!stigi_linear_equal(&a->len_args[i], &b->len_args[i])) return false;
      }
      return true;
    case STIGI_TY_VAR:
      return a->var_id == b->var_id;
    case STIGI_TY_CELL:
      return stigi_ty_equal(a->elem, b->elem);
    case STIGI_TY_ARRAY:
    case STIGI_TY_PTR:
      if (!stigi_ty_equal(a->elem, b->elem)) return false;
      if (a->has_len != b->has_len) return false;
      if (a->has_len) return stigi_linear_equal(&a->len, &b->len);
      return true;
    default:
      return true;
  }
}

/* Directional assignability: Never is bottom (assignable TO any expected type,
 * NEVER assignable FROM a non-Never value). Refined typedef converts to/from its underlying
 * type (guarded by Datalog refinement-unproved), but two distinct nominal typedefs never match. */
bool stigi_ty_assignable(const StigiTy *expected, const StigiTy *actual) {
  if (expected == NULL || actual == NULL) return false;
  if (expected->kind == STIGI_TY_ERROR || actual->kind == STIGI_TY_ERROR) return true;
  if (actual->kind == STIGI_TY_NEVER) return true;
  if (expected->kind == STIGI_TY_NEVER) return false;
  if (stigi_ty_equal(expected, actual)) return true;
  if (expected->kind == STIGI_TY_TYPEDEF && expected->elem != NULL &&
      actual->kind != STIGI_TY_TYPEDEF) {
    return stigi_ty_assignable(expected->elem, actual);
  }
  if (actual->kind == STIGI_TY_TYPEDEF && actual->elem != NULL &&
      expected->kind != STIGI_TY_TYPEDEF) {
    return stigi_ty_assignable(expected, actual->elem);
  }
  if (actual->kind == STIGI_TY_INT_LIT && is_int_ty(expected)) {
    return int_lit_fits_ty(actual->lit_val, actual->lit_neg, expected);
  }
  if (expected->kind == STIGI_TY_INT_LIT && is_int_ty(actual)) {
    return int_lit_fits_ty(expected->lit_val, expected->lit_neg, actual);
  }
  if (expected->kind == STIGI_TY_CELL && actual->kind == STIGI_TY_CELL) {
    return stigi_ty_assignable(expected->elem, actual->elem);
  }
  if (expected->kind == STIGI_TY_ARRAY && actual->kind == STIGI_TY_ARRAY) {
    if (!stigi_ty_assignable(expected->elem, actual->elem)) return false;
    if (expected->has_len != actual->has_len) return false;
    if (expected->has_len) return stigi_linear_equal(&expected->len, &actual->len);
    return true;
  }
  return false;
}

static uint64_t align_up_u64(uint64_t v, uint64_t align) {
  if (align <= 1) return v;
  uint64_t rem = v % align;
  return rem == 0 ? v : v + (align - rem);
}

/* ---- Dynamic TyCtx ---- */

typedef struct {
  StigiStr name;
  StigiTy *ty;
  bool is_mut;
  bool is_type_param;
  StigiTy *type_subst; /* non-NULL during generic instantiation */
  uint32_t binder_id;
  bool has_lin_subst;
  StigiLinear lin_subst;
  int32_t cap_unit_idx; /* >= 0 if capability binder resolved to unit, -2 builtin, -1 other */
} TyBinding;

typedef struct TyCtx {
  const StigiImage *img;
  uint32_t uidx;
  Arena *arena;
  InternPool *lin_intern;
  StigiTypeDiags *diags;
  Diag *diag;
  TyBinding *locals;
  uint32_t local_count;
  uint32_t local_cap;
  uint32_t next_binder_id;
  bool in_extern_dsl;
} TyCtx;

static void ty_ctx_free(TyCtx *ctx) {
  free(ctx->locals);
  ctx->locals = NULL;
  ctx->local_count = ctx->local_cap = 0;
}

static const TyBinding *find_local(const TyCtx *ctx, StigiStr name) {
  if (ctx == NULL || name.len == 0) return NULL;
  for (uint32_t i = ctx->local_count; i > 0; i--) {
    if (str_eq_ss(ctx->locals[i - 1].name, name)) {
      return &ctx->locals[i - 1];
    }
  }
  return NULL;
}

static TyBinding *push_ty_binding(TyCtx *ctx, StigiStr name) {
  if (name.len == 0) return NULL;
  if (ctx->local_count == ctx->local_cap) {
    uint32_t ncap = ctx->local_cap == 0 ? 32U : ctx->local_cap * 2U;
    if (ncap <= ctx->local_cap) return NULL;
    TyBinding *nl = realloc(ctx->locals, (size_t)ncap * sizeof *nl);
    if (nl == NULL) return NULL;
    ctx->locals = nl;
    ctx->local_cap = ncap;
  }
  TyBinding *b = &ctx->locals[ctx->local_count++];
  memset(b, 0, sizeof *b);
  b->name = name;
  b->binder_id = ++ctx->next_binder_id;
  b->cap_unit_idx = -1;
  return b;
}

static void push_ty_local(TyCtx *ctx, StigiStr name, StigiTy *ty, bool is_mut) {
  TyBinding *b = push_ty_binding(ctx, name);
  if (b != NULL) {
    b->ty = ty;
    b->is_mut = is_mut;
  }
}

static void push_ty_local_lin(TyCtx *ctx, StigiStr name, StigiTy *ty, bool is_mut,
                              bool has_lin, StigiLinear lin) {
  TyBinding *b = push_ty_binding(ctx, name);
  if (b != NULL) {
    b->ty = ty;
    b->is_mut = is_mut;
    b->has_lin_subst = has_lin;
    b->lin_subst = lin;
  }
}

static uint32_t push_ty_type_param(TyCtx *ctx, StigiStr name, StigiTy *subst) {
  TyBinding *b = push_ty_binding(ctx, name);
  if (b == NULL) return 0;
  b->is_type_param = true;
  b->type_subst = subst;
  return b->binder_id;
}

static void push_ty_cap_binder(TyCtx *ctx, StigiStr name, int32_t cap_unit_idx) {
  if (ctx->local_count == ctx->local_cap) {
    uint32_t ncap = ctx->local_cap == 0 ? 32U : ctx->local_cap * 2U;
    if (ncap <= ctx->local_cap) return;
    TyBinding *nl = realloc(ctx->locals, (size_t)ncap * sizeof *nl);
    if (nl == NULL) return;
    ctx->locals = nl;
    ctx->local_cap = ncap;
  }
  TyBinding *b = &ctx->locals[ctx->local_count++];
  memset(b, 0, sizeof *b);
  b->name = name;
  b->binder_id = ++ctx->next_binder_id;
  b->cap_unit_idx = cap_unit_idx;
}

static int32_t find_cap_in_scope(const TyCtx *ctx, int32_t cap_unit_idx) {
  if (ctx == NULL || cap_unit_idx < 0) return -1;
  for (uint32_t i = ctx->local_count; i > 0; i--) {
    if (ctx->locals[i - 1].cap_unit_idx == cap_unit_idx) return cap_unit_idx;
  }
  return -1;
}

static StigiTy *alloc_ty(Arena *a, StigiTyKind kind) {
  StigiTy *t = arena_alloc(a, sizeof *t);
  if (t != NULL) {
    memset(t, 0, sizeof *t);
    t->kind = kind;
  }
  return t;
}

static Status linear_from_expr_ctx(const StigiExpr *e, const TyCtx *ctx,
                                   InternPool *intern, StigiLinear *out);

static StigiTy *resolve_ast_type_ctx(const StigiImage *img, uint32_t from_idx,
                                     const StigiType *ast_ty, Arena *arena,
                                     InternPool *lin_intern, const TyCtx *tctx,
                                     StigiTypeDiags *diags, Diag *diag, int depth);

static StigiTy *resolve_ast_type(const StigiImage *img, uint32_t from_idx,
                                 const StigiType *ast_ty, Arena *arena,
                                 InternPool *lin_intern, StigiTypeDiags *diags,
                                 Diag *diag, int depth) {
  return resolve_ast_type_ctx(img, from_idx, ast_ty, arena, lin_intern, NULL, diags, diag, depth);
}

/* ---- Layout computation (memory.md 8) ---- */

static StigiLayout ty_layout_rec(const StigiImage *img, const StigiTy *ty, int depth);

static StigiEnumLayoutInfo enum_layout_info_rec(const StigiImage *img, uint32_t enum_uidx,
                                                StigiTy *const *type_args,
                                                uint32_t type_arg_count, int depth) {
  StigiEnumLayoutInfo info;
  memset(&info, 0, sizeof info);
  info.layout = (StigiLayout){1, 1};
  info.tag_size = 1;
  info.union_align = 1;
  if (img == NULL || enum_uidx >= img->unit_count || depth > 32) return info;
  const StigiDecl *d = img->units[enum_uidx].ast->decl;
  if (d->kind != STIGI_DECL_ENUM) return info;

  uint32_t case_count = d->u.enum_decl.case_count;
  uint64_t tag_size = (case_count <= 256U) ? 1ULL : (case_count <= 65536U ? 2ULL : 4ULL);
  uint64_t tag_align = tag_size;
  info.tag_size = tag_size;

  Arena tmp;
  arena_init(&tmp);
  InternPool lin;
  intern_init(&lin);
  TyCtx ectx;
  memset(&ectx, 0, sizeof ectx);
  ectx.img = img;
  ectx.uidx = enum_uidx;
  ectx.arena = &tmp;
  ectx.lin_intern = &lin;
  for (uint32_t tp = 0; tp < d->u.enum_decl.type_param_count; tp++) {
    StigiTy *subst = (tp < type_arg_count && type_args != NULL) ? type_args[tp] : NULL;
    (void)push_ty_type_param(&ectx, d->u.enum_decl.type_params[tp].name, subst);
  }

  /* memory.md 8: 1-word nullable-pointer optimization:
   * 1 field-less case + 1 case holding a single Array or Cell field */
  if (case_count == 2U) {
    uint32_t f0 = d->u.enum_decl.cases[0].field_count;
    uint32_t f1 = d->u.enum_decl.cases[1].field_count;
    if ((f0 == 0U && f1 == 1U) || (f1 == 0U && f0 == 1U)) {
      uint32_t nc = (f0 == 0U) ? 0U : 1U;
      uint32_t pc = (f0 == 0U) ? 1U : 0U;
      StigiTy *fty = resolve_ast_type_ctx(img, enum_uidx, d->u.enum_decl.cases[pc].fields[0].type,
                                          &tmp, &lin, &ectx, NULL, NULL, 0);
      const StigiTy *ufty = unwrap_ty_underlying(fty);
      if (ufty != NULL && (ufty->kind == STIGI_TY_ARRAY || ufty->kind == STIGI_TY_CELL)) {
        ty_ctx_free(&ectx);
        intern_free(&lin);
        arena_free(&tmp);
        info.is_nullable_ptr = true;
        info.null_case = nc;
        info.ptr_case = pc;
        info.tag_size = 0;
        info.tag_offset = 0;
        info.layout = (StigiLayout){8, 8};
        return info;
      }
    }
  }

  uint64_t union_align = 1;
  uint64_t union_size = 0;
  uint64_t max_raw_end = 0;
  bool has_any_fields = false;

  for (uint32_t c = 0; c < case_count; c++) {
    const StigiEnumCase *ec = &d->u.enum_decl.cases[c];
    if (ec->field_count == 0) continue;
    StigiLayout *flays = arena_alloc(&tmp, (size_t)ec->field_count * sizeof *flays);
    uint32_t *order = arena_alloc(&tmp, (size_t)ec->field_count * sizeof *order);
    if (flays == NULL || order == NULL) continue;
    uint32_t non_empty = 0;
    for (uint32_t f = 0; f < ec->field_count; f++) {
      StigiTy *fty = resolve_ast_type_ctx(img, enum_uidx, ec->fields[f].type,
                                          &tmp, &lin, &ectx, NULL, NULL, 0);
      flays[f] = ty_layout_rec(img, fty, depth + 1);
      order[f] = f;
      if (flays[f].size > 0) non_empty++;
    }
    if (non_empty == 0) continue;
    has_any_fields = true;
    for (uint32_t i = 1; i < ec->field_count; i++) {
      uint32_t key = order[i];
      uint64_t ka = flays[key].align;
      uint32_t j = i;
      while (j > 0 && flays[order[j - 1]].align < ka) {
        order[j] = order[j - 1];
        j--;
      }
      order[j] = key;
    }
    uint64_t c_align = 1;
    uint64_t off = 0;
    for (uint32_t i = 0; i < ec->field_count; i++) {
      StigiLayout fl = flays[order[i]];
      if (fl.size == 0) continue;
      if (fl.align > c_align) c_align = fl.align;
      off = align_up_u64(off, fl.align) + fl.size;
    }
    uint64_t c_size = align_up_u64(off, c_align);
    if (off > max_raw_end) max_raw_end = off;
    if (c_align > union_align) union_align = c_align;
    if (c_size > union_size) union_size = c_size;
  }

  ty_ctx_free(&ectx);
  intern_free(&lin);
  arena_free(&tmp);

  if (!has_any_fields) {
    info.is_tag_only = true;
    info.tag_offset = 0;
    info.layout = (StigiLayout){tag_size, tag_align};
    return info;
  }

  info.union_align = union_align;
  info.union_size = align_up_u64(union_size, union_align);
  uint64_t total_align = union_align > tag_align ? union_align : tag_align;
  uint64_t padded_union = align_up_u64(info.union_size, total_align);
  uint64_t cand_tag_off = align_up_u64(max_raw_end, tag_align);
  if (max_raw_end > 0 && cand_tag_off + tag_size <= padded_union) {
    info.tag_in_tail = true;
    info.tag_offset = cand_tag_off;
    info.layout = (StigiLayout){padded_union, total_align};
  } else {
    info.tag_in_tail = false;
    info.tag_offset = align_up_u64(info.union_size, tag_align);
    info.layout = (StigiLayout){align_up_u64(info.tag_offset + tag_size, total_align), total_align};
  }
  return info;
}

StigiEnumLayoutInfo stigi_enum_layout_info(const StigiImage *img, uint32_t enum_uidx,
                                           StigiTy *const *type_args, uint32_t type_arg_count) {
  return enum_layout_info_rec(img, enum_uidx, type_args, type_arg_count, 0);
}

static StigiLayout ty_layout_rec(const StigiImage *img, const StigiTy *ty, int depth) {
  StigiLayout z = {0, 1};
  if (ty == NULL || depth > 32) return z;
  switch (ty->kind) {
    case STIGI_TY_U8:
    case STIGI_TY_BOOL:
      return (StigiLayout){1, 1};
    case STIGI_TY_U32:
    case STIGI_TY_I32:
      return (StigiLayout){4, 4};
    case STIGI_TY_U64:
    case STIGI_TY_I64:
    case STIGI_TY_PTR:
    case STIGI_TY_CELL:
    case STIGI_TY_ARRAY:
    case STIGI_TY_INT_LIT:
    case STIGI_TY_FILL_RESULT:
    case STIGI_TY_MAKE_RESULT:
      return (StigiLayout){8, 8};
    case STIGI_TY_GROW_RESULT:
      return (StigiLayout){16, 8};
    case STIGI_TY_UNIT:
    case STIGI_TY_NEVER:
    case STIGI_TY_TYPE:
    case STIGI_TY_VAR:
    case STIGI_TY_TEST_RESULT:
    case STIGI_TY_ERROR:
    case STIGI_TY_UNKNOWN:
      return (StigiLayout){0, 1};
    case STIGI_TY_TYPEDEF:
      return ty_layout_rec(img, ty->elem, depth + 1);
    case STIGI_TY_ENUM:
      return enum_layout_info_rec(img, ty->unit_idx, ty->type_args, ty->type_arg_count, depth + 1).layout;
  }
  return z;
}

StigiLayout stigi_ty_layout(const StigiImage *img, const StigiTy *ty) {
  return ty_layout_rec(img, ty, 0);
}

static void frame_walk_expr(const StigiImage *img, uint32_t uidx, const StigiExpr *e,
                            Arena *tmp, InternPool *lin, uint64_t *bytes, bool *has_call) {
  if (e == NULL) return;
  switch (e->kind) {
    case STIGI_EXPR_VAL:
      if (e->u.val_stmt.type != NULL) {
        StigiTy *ty = resolve_ast_type(img, uidx, e->u.val_stmt.type, tmp, lin, NULL, NULL, 0);
        StigiLayout l = stigi_ty_layout(img, ty);
        *bytes = align_up_u64(*bytes, l.align) + l.size;
      } else {
        *bytes = align_up_u64(*bytes, 8) + 8;
      }
      frame_walk_expr(img, uidx, e->u.val_stmt.init, tmp, lin, bytes, has_call);
      break;
    case STIGI_EXPR_VAR:
      if (e->u.var_stmt.type != NULL) {
        StigiTy *ty = resolve_ast_type(img, uidx, e->u.var_stmt.type, tmp, lin, NULL, NULL, 0);
        StigiLayout l = stigi_ty_layout(img, ty);
        *bytes = align_up_u64(*bytes, l.align) + l.size;
      } else {
        *bytes = align_up_u64(*bytes, 8) + 8;
      }
      frame_walk_expr(img, uidx, e->u.var_stmt.init, tmp, lin, bytes, has_call);
      break;
    case STIGI_EXPR_CALL:
      if (has_call != NULL && e->u.call.callee != NULL &&
          (e->u.call.callee->kind == STIGI_EXPR_GLOBAL ||
           e->u.call.callee->kind == STIGI_EXPR_LOCAL)) {
        StigiStr cn = e->u.call.callee->u.ident.name;
        int32_t tgt = stigi_image_resolve_name(img, uidx, cn.ptr, cn.len);
        if (tgt >= 0 && (uint32_t)tgt != uidx) {
          StigiDeclKind dk = img->units[tgt].ast->decl->kind;
          if (dk == STIGI_DECL_DEF || dk == STIGI_DECL_EXTERN) *has_call = true;
        }
      }
      for (uint32_t i = 0; i < e->u.call.arg_count; i++) {
        frame_walk_expr(img, uidx, e->u.call.args[i], tmp, lin, bytes, has_call);
      }
      break;
    case STIGI_EXPR_BINOP:
      frame_walk_expr(img, uidx, e->u.binop.lhs, tmp, lin, bytes, has_call);
      frame_walk_expr(img, uidx, e->u.binop.rhs, tmp, lin, bytes, has_call);
      break;
    case STIGI_EXPR_UNOP:
      frame_walk_expr(img, uidx, e->u.unop.operand, tmp, lin, bytes, has_call);
      break;
    case STIGI_EXPR_ASSIGN:
      frame_walk_expr(img, uidx, e->u.assign_stmt.value, tmp, lin, bytes, has_call);
      break;
    case STIGI_EXPR_BLOCK:
      for (uint32_t i = 0; i < e->u.block.count; i++) {
        frame_walk_expr(img, uidx, e->u.block.stmts[i], tmp, lin, bytes, has_call);
      }
      break;
    case STIGI_EXPR_IF:
      frame_walk_expr(img, uidx, e->u.if_expr.cond, tmp, lin, bytes, has_call);
      frame_walk_expr(img, uidx, e->u.if_expr.then_br, tmp, lin, bytes, has_call);
      frame_walk_expr(img, uidx, e->u.if_expr.else_br, tmp, lin, bytes, has_call);
      break;
    case STIGI_EXPR_WHILE:
      frame_walk_expr(img, uidx, e->u.while_expr.cond, tmp, lin, bytes, has_call);
      frame_walk_expr(img, uidx, e->u.while_expr.body, tmp, lin, bytes, has_call);
      break;
    case STIGI_EXPR_MATCH:
      frame_walk_expr(img, uidx, e->u.match_expr.scrutinee, tmp, lin, bytes, has_call);
      if (e->u.match_expr.scrutinee != NULL &&
          e->u.match_expr.scrutinee->kind == STIGI_EXPR_CALL &&
          (e->u.match_expr.scrutinee->u.call.callee->kind == STIGI_EXPR_GLOBAL ||
           e->u.match_expr.scrutinee->u.call.callee->kind == STIGI_EXPR_LOCAL)) {
        StigiStr cn = e->u.match_expr.scrutinee->u.call.callee->u.ident.name;
        if ((cn.len == 13 && memcmp(cn.ptr, "Array.tryFill", 13) == 0) ||
            (cn.len == 13 && memcmp(cn.ptr, "Array.tryGrow", 13) == 0) ||
            (cn.len == 12 && memcmp(cn.ptr, "Cell.tryMake", 12) == 0)) {
          /* Account for call-save spill/temps and constant-size stack-promoted arrays */
          *bytes = align_up_u64(*bytes, 16) + 48;
          if (cn.len == 13 && memcmp(cn.ptr, "Array.tryFill", 13) == 0 &&
              e->u.match_expr.scrutinee->u.call.arg_count == 2 &&
              e->u.match_expr.scrutinee->u.call.args[0]->kind == STIGI_EXPR_LIT_INT) {
            uint64_t n = e->u.match_expr.scrutinee->u.call.args[0]->u.lit_int.value;
            if (n > 0 && n <= 256ULL) {
              uint64_t esz = 8ULL;
              if (e->u.match_expr.scrutinee->u.call.args[1]->kind == STIGI_EXPR_LIT_CHAR) esz = 1ULL;
              else if (e->u.match_expr.scrutinee->u.call.type_arg_count > 0) {
                StigiTy *ety = resolve_ast_type(img, uidx,
                                                e->u.match_expr.scrutinee->u.call.type_args[0],
                                                tmp, lin, NULL, NULL, 0);
                StigiLayout el = stigi_ty_layout(img, ety);
                if (el.size > 0) esz = el.size;
              }
              uint64_t tot = n * esz;
              if (tot <= 256ULL) {
                *bytes = align_up_u64(*bytes, 64) + align_up_u64(tot, 64) + 64ULL;
              }
            }
          }
        }
      }
      for (uint32_t i = 0; i < e->u.match_expr.arm_count; i++) {
        const StigiPat *pat = &e->u.match_expr.arms[i].pat;
        if (pat->kind == STIGI_PAT_CTOR) {
          for (uint32_t f = 0; f < pat->field_count; f++) {
            if (!(pat->fields[f].len == 1 && pat->fields[f].ptr[0] == '_')) {
              *bytes = align_up_u64(*bytes, 8) + 8;
            }
          }
        } else if (pat->kind == STIGI_PAT_VAR) {
          *bytes = align_up_u64(*bytes, 8) + 8;
        }
        frame_walk_expr(img, uidx, e->u.match_expr.arms[i].body, tmp, lin, bytes, has_call);
      }
      break;
    case STIGI_EXPR_HANDLE: {
      /* Include handler state struct (val_params + vars) allocated in enclosing frame (capabilities.md 2) */
      int32_t hidx = stigi_image_resolve_name(img, uidx,
                                              e->u.handle_stmt.handler_name.ptr,
                                              e->u.handle_stmt.handler_name.len);
      if (hidx >= 0 && img->units[hidx].ast->decl->kind == STIGI_DECL_HANDLER) {
        const StigiDecl *hd = img->units[hidx].ast->decl;
        for (uint32_t p = 0; p < hd->u.handler_decl.val_param_count; p++) {
          StigiTy *ty = resolve_ast_type(img, (uint32_t)hidx,
                                         hd->u.handler_decl.val_params[p].type, tmp, lin, NULL, NULL, 0);
          StigiLayout l = stigi_ty_layout(img, ty);
          *bytes = align_up_u64(*bytes, l.align) + l.size;
        }
        for (uint32_t v = 0; v < hd->u.handler_decl.var_count; v++) {
          frame_walk_expr(img, (uint32_t)hidx, hd->u.handler_decl.vars[v], tmp, lin, bytes, has_call);
        }
      }
      frame_walk_expr(img, uidx, e->u.handle_stmt.body, tmp, lin, bytes, has_call);
      break;
    }
    default:
      break;
  }
}

Status stigi_def_frame_bytes(const StigiImage *img, uint32_t unit_idx, uint64_t *out_bytes) {
  if (img == NULL || unit_idx >= img->unit_count || out_bytes == NULL) return STIGI_ERR_INVALID;
  const StigiDecl *d = img->units[unit_idx].ast->decl;
  if (d->kind == STIGI_DECL_TEST) {
    Arena tmp;
    arena_init(&tmp);
    InternPool lin;
    intern_init(&lin);
    uint64_t bytes = 0;
    bool has_call = false;
    frame_walk_expr(img, unit_idx, stigi_val_body(d), &tmp, &lin, &bytes, &has_call);
    if (has_call) bytes = align_up_u64(bytes, 16) + 48ULL;
    intern_free(&lin);
    arena_free(&tmp);
    *out_bytes = align_up_u64(bytes, 8);
    return STIGI_OK;
  }
  if (d->kind != STIGI_DECL_DEF && d->kind != STIGI_DECL_LEMMA_DEF &&
      !(d->kind == STIGI_DECL_EXTERN && !d->u.extern_decl.is_val && d->u.extern_decl.body != NULL)) {
    *out_bytes = 0;
    return STIGI_OK;
  }
  const StigiDefHead *h = (d->kind == STIGI_DECL_EXTERN) ? &d->u.extern_decl.head
                                                         : &d->u.def_decl.head;
  const StigiExpr *body = (d->kind == STIGI_DECL_EXTERN) ? d->u.extern_decl.body
                                                         : stigi_def_body(d);
  Arena tmp;
  arena_init(&tmp);
  InternPool lin;
  intern_init(&lin);
  uint64_t bytes = 0;
  for (uint32_t i = 0; i < h->param_count; i++) {
    StigiTy *ty = resolve_ast_type(img, unit_idx, h->params[i].type,
                                   &tmp, &lin, NULL, NULL, 0);
    StigiLayout l = stigi_ty_layout(img, ty);
    bytes = align_up_u64(bytes, l.align) + l.size;
  }
  bool has_call = false;
  frame_walk_expr(img, unit_idx, body, &tmp, &lin, &bytes, &has_call);
  if (has_call) bytes = align_up_u64(bytes, 16) + 48ULL;
  intern_free(&lin);
  arena_free(&tmp);
  *out_bytes = align_up_u64(bytes, 8);
  return STIGI_OK;
}

/* ---- Recursive-type check (S43/S69: inline fields + Array/Cell element types) ---- */

typedef struct {
  uint32_t *items;
  uint32_t count;
  uint32_t cap;
  bool oom;
} DepVec;

static void dep_vec_push(DepVec *v, uint32_t x) {
  if (v->oom) return;
  if (v->count == v->cap) {
    uint32_t ncap = v->cap == 0 ? 16U : v->cap * 2U;
    if (ncap <= v->cap) { v->oom = true; return; }
    uint32_t *ni = realloc(v->items, (size_t)ncap * sizeof *ni);
    if (ni == NULL) { v->oom = true; return; }
    v->items = ni;
    v->cap = ncap;
  }
  v->items[v->count++] = x;
}

static void collect_type_deps(const StigiImage *img, uint32_t from_idx, const StigiType *ty,
                              DepVec *deps) {
  if (ty == NULL || deps->oom) return;
  if (ty->kind == STIGI_TYPE_ARRAY || ty->kind == STIGI_TYPE_CELL || ty->kind == STIGI_TYPE_PTR) {
    for (uint32_t i = 0; i < ty->type_arg_count; i++) {
      collect_type_deps(img, from_idx, ty->type_args[i], deps);
    }
    return;
  }
  int32_t tgt = stigi_image_resolve_name(img, from_idx, ty->name.ptr, ty->name.len);
  if (tgt >= 0) {
    const StigiDecl *d = img->units[tgt].ast->decl;
    if (d->kind == STIGI_DECL_ENUM || d->kind == STIGI_DECL_TYPE) {
      dep_vec_push(deps, (uint32_t)tgt);
    }
  }
  for (uint32_t i = 0; i < ty->type_arg_count; i++) {
    collect_type_deps(img, from_idx, ty->type_args[i], deps);
  }
}

static Status dfs_recursive_type(const StigiImage *img, uint32_t u, uint8_t *color,
                                 StigiTypeDiags *diags, Diag *diag) {
  color[u] = 1;
  const StigiDecl *d = img->units[u].ast->decl;
  DepVec deps;
  memset(&deps, 0, sizeof deps);

  if (d->kind == STIGI_DECL_ENUM) {
    for (uint32_t c = 0; c < d->u.enum_decl.case_count; c++) {
      for (uint32_t f = 0; f < d->u.enum_decl.cases[c].field_count; f++) {
        collect_type_deps(img, u, d->u.enum_decl.cases[c].fields[f].type, &deps);
      }
    }
  } else if (d->kind == STIGI_DECL_TYPE) {
    collect_type_deps(img, u, d->u.type_decl.rhs, &deps);
  }

  if (deps.oom) {
    free(deps.items);
    color[u] = 2;
    return diag_set(diag, STIGI_ERR_OOM, -1, "dfs_recursive_type: OOM");
  }

  Status st = STIGI_OK;
  for (uint32_t i = 0; i < deps.count && st == STIGI_OK; i++) {
    uint32_t v = deps.items[i];
    if (color[v] == 1) {
      st = add_tdiag(diags, STIGI_TYPE_RECURSIVE_TYPE, d->span, diag,
                     "recursive type cycle involving '%s' and '%s'",
                     img->units[u].name, img->units[v].name);
    } else if (color[v] == 0) {
      st = dfs_recursive_type(img, v, color, diags, diag);
    }
  }
  free(deps.items);
  color[u] = 2;
  return st;
}

/* ---- Hardened extern-forges-facts check (S110, fail closed!) ---- */

static bool type_carries_facts(const StigiImage *img, uint32_t from_idx,
                               const StigiType *ty, int depth) {
  if (ty == NULL) return false;
  if (depth > 16) return true; /* Fail CLOSED on deep/cyclic types */
  for (uint32_t i = 0; i < ty->type_arg_count; i++) {
    if (type_carries_facts(img, from_idx, ty->type_args[i], depth + 1)) return true;
  }
  if (ty->kind == STIGI_TYPE_ARRAY || ty->kind == STIGI_TYPE_CELL || ty->kind == STIGI_TYPE_PTR) {
    return false;
  }
  int32_t tgt = stigi_image_resolve_name(img, from_idx, ty->name.ptr, ty->name.len);
  if (tgt < 0) return false;
  const StigiDecl *d = img->units[tgt].ast->decl;
  if (d->kind == STIGI_DECL_TYPE) {
    if (d->u.type_decl.has_facts) return true;
    return type_carries_facts(img, (uint32_t)tgt, d->u.type_decl.rhs, depth + 1);
  }
  if (d->kind == STIGI_DECL_ENUM) {
    for (uint32_t c = 0; c < d->u.enum_decl.case_count; c++) {
      const StigiEnumCase *ec = &d->u.enum_decl.cases[c];
      if (ec->has_facts) return true;
      for (uint32_t f = 0; f < ec->field_count; f++) {
        if (type_carries_facts(img, (uint32_t)tgt, ec->fields[f].type, depth + 1)) return true;
      }
    }
  }
  return false;
}

static bool resolved_ty_is_enum_or_carries_facts(const StigiImage *img, const StigiTy *ty, int depth) {
  if (ty == NULL) return false;
  if (depth > 16) return true; /* Fail CLOSED */
  switch (ty->kind) {
    case STIGI_TY_ENUM:
      return true;
    case STIGI_TY_TYPEDEF: {
      if (ty->unit_idx < img->unit_count) {
        const StigiDecl *d = img->units[ty->unit_idx].ast->decl;
        if (d->kind == STIGI_DECL_TYPE && d->u.type_decl.has_facts) return true;
      }
      return resolved_ty_is_enum_or_carries_facts(img, ty->elem, depth + 1);
    }
    case STIGI_TY_ARRAY:
    case STIGI_TY_CELL:
    case STIGI_TY_PTR:
      return resolved_ty_is_enum_or_carries_facts(img, ty->elem, depth + 1);
    default:
      for (uint32_t i = 0; i < ty->type_arg_count; i++) {
        if (resolved_ty_is_enum_or_carries_facts(img, ty->type_args[i], depth + 1)) return true;
      }
      return false;
  }
}

static bool fact_atom_is_overwrites(const StigiImage *img, uint32_t uidx, StigiStr atom_name) {
  if (str_eq_c(atom_name, "overwrites") || str_eq_c(atom_name, "Stigi.Facts.overwrites")) {
    return true;
  }
  if (uidx < img->unit_count) {
    const StigiUnit *u = img->units[uidx].ast;
    for (uint32_t i = 0; i < u->alias_count; i++) {
      if (str_eq_ss(u->aliases[i].name, atom_name)) {
        return str_eq_c(u->aliases[i].target, "overwrites") ||
               str_eq_c(u->aliases[i].target, "Stigi.Facts.overwrites");
      }
    }
  }
  return false;
}

static void check_overwrites_group(const StigiImage *img, uint32_t uidx,
                                   const StigiDefHead *h, const StigiFactGroup *fg,
                                   Arena *arena, InternPool *lin_intern,
                                   StigiTypeDiags *diags, Diag *diag) {
  for (uint32_t i = 0; i < fg->count; i++) {
    const StigiFactItem *it = &fg->items[i];
    if (it->kind != STIGI_FACT_ATOM || it->arg_count < 1) continue;
    if (!fact_atom_is_overwrites(img, uidx, it->atom_name)) continue;
    const StigiExpr *arg0 = it->args[0];
    if (arg0->kind != STIGI_EXPR_LOCAL) continue;
    for (uint32_t p = 0; p < h->param_count; p++) {
      if (str_eq_ss(h->params[p].name, arg0->u.ident.name)) {
        /* Resolve fully through generic transparent type aliases */
        StigiTy *rty = resolve_ast_type(img, uidx, h->params[p].type, arena, lin_intern, NULL, NULL, 0);
        const StigiTy *u = unwrap_ty_underlying(rty);
        if (u != NULL &&
            (u->kind == STIGI_TY_ARRAY || u->kind == STIGI_TY_CELL || u->kind == STIGI_TY_PTR) &&
            resolved_ty_is_enum_or_carries_facts(img, u->elem, 0)) {
          (void)add_tdiag(diags, STIGI_TYPE_EXTERN_FORGES_FACTS, it->span, diag,
                          "body-less extern '%s' overwrites buffer '%.*s' of enum/refined elements",
                          img->units[uidx].name, (int)h->params[p].name.len, h->params[p].name.ptr);
        }
      }
    }
  }
}

/* ---- Linear extraction with TyCtx & top-level val comptime folding ---- */

static Status linear_from_expr_ctx(const StigiExpr *e, const TyCtx *ctx,
                                   InternPool *intern, StigiLinear *out) {
  if (e == NULL || intern == NULL || out == NULL) return STIGI_ERR_INVALID;
  switch (e->kind) {
    case STIGI_EXPR_LIT_INT:
      if (e->u.lit_int.overflow || e->u.lit_int.value > (uint64_t)INT64_MAX) {
        return STIGI_ERR_OVERFLOW;
      }
      *out = stigi_linear_const((int64_t)e->u.lit_int.value);
      return STIGI_OK;
    case STIGI_EXPR_LOCAL:
    case STIGI_EXPR_GLOBAL: {
      const TyBinding *b = find_local(ctx, e->u.ident.name);
      if (b != NULL) {
        if (b->is_mut) return STIGI_ERR_INVALID; /* length term never names a var */
        if (b->has_lin_subst) {
          *out = b->lin_subst;
          return STIGI_OK;
        }
        *out = stigi_linear_var(b->binder_id);
        return STIGI_OK;
      }
      /* Fold top-level immutable val via comptime evaluator (types.md 3 & 4) */
      if (ctx != NULL && ctx->img != NULL) {
        int32_t tgt = stigi_image_resolve_name(ctx->img, ctx->uidx,
                                               e->u.ident.name.ptr, e->u.ident.name.len);
        if (tgt >= 0) {
          const StigiDecl *d = ctx->img->units[tgt].ast->decl;
          if (d->kind == STIGI_DECL_VAL) {
            Arena ev_arena;
            arena_init(&ev_arena);
            StigiEvalResult er = stigi_eval_unit(ctx->img, (uint32_t)tgt, 10000ULL, &ev_arena);
            if (er.status == STIGI_OK && er.value.kind == STIGI_VAL_INT) {
              if (er.value.is_unsigned) {
                if (er.value.u.u <= (uint64_t)INT64_MAX) {
                  *out = stigi_linear_const((int64_t)er.value.u.u);
                  arena_free(&ev_arena);
                  return STIGI_OK;
                }
              } else {
                *out = stigi_linear_const(er.value.u.i);
                arena_free(&ev_arena);
                return STIGI_OK;
              }
            }
            arena_free(&ev_arena);
          }
        }
      }
      /* Scope free symbol by unit_idx to prevent cross-unit aliasing */
      char scoped[256];
      uint32_t uid = (ctx != NULL) ? ctx->uidx : 0U;
      int sn = snprintf(scoped, sizeof scoped, "%u:%.*s", uid,
                        (int)e->u.ident.name.len, e->u.ident.name.ptr);
      size_t slen = (sn > 0 && (size_t)sn < sizeof(scoped)) ? (size_t)sn : sizeof(scoped) - 1U;
      uint32_t id = 0;
      Diag d;
      diag_clear(&d);
      Status st = intern_str(intern, scoped, slen, &id, &d);
      if (st != STIGI_OK) return st;
      *out = stigi_linear_var(0x80000000U | id);
      return STIGI_OK;
    }
    case STIGI_EXPR_UNOP:
      if (e->u.unop.op == STIGI_OP_NEG) {
        StigiLinear inner;
        Status st = linear_from_expr_ctx(e->u.unop.operand, ctx, intern, &inner);
        if (st != STIGI_OK) return st;
        StigiLinear neg1 = stigi_linear_const(-1);
        return stigi_linear_mul(&neg1, &inner, out);
      }
      return STIGI_ERR_INVALID;
    case STIGI_EXPR_BINOP: {
      StigiLinear l, r;
      Status st = linear_from_expr_ctx(e->u.binop.lhs, ctx, intern, &l);
      if (st != STIGI_OK) return st;
      st = linear_from_expr_ctx(e->u.binop.rhs, ctx, intern, &r);
      if (st != STIGI_OK) return st;
      if (e->u.binop.op == STIGI_OP_ADD) return stigi_linear_add(&l, &r, out);
      if (e->u.binop.op == STIGI_OP_SUB) return stigi_linear_sub(&l, &r, out);
      if (e->u.binop.op == STIGI_OP_MUL) return stigi_linear_mul(&l, &r, out);
      if (e->u.binop.op == STIGI_OP_DIV && l.nterms == 0 && r.nterms == 0 && r.c != 0) {
        if (l.c == INT64_MIN && r.c == -1) return STIGI_ERR_OVERFLOW;
        *out = stigi_linear_const(l.c / r.c);
        return STIGI_OK;
      }
      if (e->u.binop.op == STIGI_OP_MOD && l.nterms == 0 && r.nterms == 0 && r.c != 0) {
        if (l.c == INT64_MIN && r.c == -1) return STIGI_ERR_OVERFLOW;
        *out = stigi_linear_const(l.c % r.c);
        return STIGI_OK;
      }
      return STIGI_ERR_INVALID;
    }
    default:
      return STIGI_ERR_INVALID;
  }
}

/* ---- AST Type -> StigiTy resolution with generic & transparent type expansion ---- */

static StigiTy *resolve_ast_type_ctx(const StigiImage *img, uint32_t from_idx,
                                     const StigiType *ast_ty, Arena *arena,
                                     InternPool *lin_intern, const TyCtx *tctx,
                                     StigiTypeDiags *diags, Diag *diag, int depth) {
  if (ast_ty == NULL || depth > 16) return alloc_ty(arena, STIGI_TY_ERROR);
  if (str_eq_c(ast_ty->name, "U8")) return alloc_ty(arena, STIGI_TY_U8);
  if (str_eq_c(ast_ty->name, "U32")) return alloc_ty(arena, STIGI_TY_U32);
  if (str_eq_c(ast_ty->name, "U64")) return alloc_ty(arena, STIGI_TY_U64);
  if (str_eq_c(ast_ty->name, "I32")) return alloc_ty(arena, STIGI_TY_I32);
  if (str_eq_c(ast_ty->name, "I64")) return alloc_ty(arena, STIGI_TY_I64);
  if (str_eq_c(ast_ty->name, "Bool")) return alloc_ty(arena, STIGI_TY_BOOL);
  if (str_eq_c(ast_ty->name, "Unit")) return alloc_ty(arena, STIGI_TY_UNIT);
  if (str_eq_c(ast_ty->name, "Never")) return alloc_ty(arena, STIGI_TY_NEVER);
  if (str_eq_c(ast_ty->name, "Type")) return alloc_ty(arena, STIGI_TY_TYPE);
  if (str_eq_c(ast_ty->name, "Stigi.Test.Result")) return alloc_ty(arena, STIGI_TY_TEST_RESULT);

  /* Check generic type parameter in scope */
  if (tctx != NULL && ast_ty->kind == STIGI_TYPE_NAMED) {
    const TyBinding *tb = find_local(tctx, ast_ty->name);
    if (tb != NULL && tb->is_type_param) {
      if (tb->type_subst != NULL) return tb->type_subst;
      StigiTy *vt = alloc_ty(arena, STIGI_TY_VAR);
      if (vt != NULL) vt->var_id = tb->binder_id;
      return vt;
    }
  }

  if (ast_ty->kind == STIGI_TYPE_ARRAY || ast_ty->kind == STIGI_TYPE_PTR) {
    StigiTy *t = alloc_ty(arena, ast_ty->kind == STIGI_TYPE_ARRAY ? STIGI_TY_ARRAY : STIGI_TY_PTR);
    if (t == NULL) return NULL;
    t->elem = ast_ty->type_arg_count > 0
                  ? resolve_ast_type_ctx(img, from_idx, ast_ty->type_args[0], arena, lin_intern,
                                         tctx, diags, diag, depth + 1)
                  : alloc_ty(arena, STIGI_TY_ERROR);
    if (ast_ty->len_arg_count > 0) {
      if (linear_from_expr_ctx(ast_ty->len_args[0], tctx, lin_intern, &t->len) == STIGI_OK) {
        t->has_len = true;
      } else if (diags != NULL) {
        (void)add_tdiag(diags, STIGI_TYPE_LENGTH_NOT_LINEAR, ast_ty->len_args[0]->span, diag,
                        "length term is not linear");
      }
    }
    return t;
  }

  if (ast_ty->kind == STIGI_TYPE_CELL) {
    StigiTy *t = alloc_ty(arena, STIGI_TY_CELL);
    if (t == NULL) return NULL;
    t->elem = ast_ty->type_arg_count > 0
                  ? resolve_ast_type_ctx(img, from_idx, ast_ty->type_args[0], arena, lin_intern,
                                         tctx, diags, diag, depth + 1)
                  : alloc_ty(arena, STIGI_TY_ERROR);
    return t;
  }

  int32_t tgt = stigi_image_resolve_name(img, from_idx, ast_ty->name.ptr, ast_ty->name.len);
  if (tgt >= 0) {
    const StigiDecl *d = img->units[tgt].ast->decl;
    if (d->kind == STIGI_DECL_ENUM) {
      if (ast_ty->type_arg_count != d->u.enum_decl.type_param_count && diags != NULL) {
        (void)add_tdiag(diags, STIGI_TYPE_TYPE_ARITY, ast_ty->span, diag,
                        "enum '%s' expects %u type args, got %u",
                        img->units[tgt].name, d->u.enum_decl.type_param_count,
                        ast_ty->type_arg_count);
      }
      StigiTy *t = alloc_ty(arena, STIGI_TY_ENUM);
      if (t == NULL) return NULL;
      t->unit_idx = (uint32_t)tgt;
      if (ast_ty->type_arg_count > 0) {
        t->type_args = arena_alloc(arena, (size_t)ast_ty->type_arg_count * sizeof *t->type_args);
        if (t->type_args != NULL) {
          t->type_arg_count = ast_ty->type_arg_count;
          for (uint32_t i = 0; i < ast_ty->type_arg_count; i++) {
            t->type_args[i] = resolve_ast_type_ctx(img, from_idx, ast_ty->type_args[i], arena,
                                                   lin_intern, tctx, diags, diag, depth + 1);
          }
        }
      }
      return t;
    }
    if (d->kind == STIGI_DECL_TYPE) {
      if (ast_ty->type_arg_count != d->u.type_decl.type_param_count && diags != NULL) {
        (void)add_tdiag(diags, STIGI_TYPE_TYPE_ARITY, ast_ty->span, diag,
                        "type '%s' expects %u type args, got %u",
                        img->units[tgt].name, d->u.type_decl.type_param_count,
                        ast_ty->type_arg_count);
      }
      /* Resolve caller's type_args and len_args, then substitute into type_decl.rhs */
      StigiTy **targs = NULL;
      if (ast_ty->type_arg_count > 0) {
        targs = arena_alloc(arena, (size_t)ast_ty->type_arg_count * sizeof *targs);
        if (targs != NULL) {
          for (uint32_t i = 0; i < ast_ty->type_arg_count; i++) {
            targs[i] = resolve_ast_type_ctx(img, from_idx, ast_ty->type_args[i], arena,
                                            lin_intern, tctx, diags, diag, depth + 1);
          }
        }
      }
      StigiLinear *largs = NULL;
      if (ast_ty->len_arg_count > 0) {
        largs = arena_alloc(arena, (size_t)ast_ty->len_arg_count * sizeof *largs);
        if (largs != NULL) {
          for (uint32_t i = 0; i < ast_ty->len_arg_count; i++) {
            if (linear_from_expr_ctx(ast_ty->len_args[i], tctx, lin_intern, &largs[i]) != STIGI_OK) {
              largs[i] = stigi_linear_const(0);
              if (diags != NULL) {
                (void)add_tdiag(diags, STIGI_TYPE_LENGTH_NOT_LINEAR, ast_ty->len_args[i]->span,
                                diag, "length term is not linear");
              }
            }
          }
        }
      }

      TyCtx sub;
      memset(&sub, 0, sizeof sub);
      sub.img = img;
      sub.uidx = (uint32_t)tgt;
      sub.arena = arena;
      sub.lin_intern = lin_intern;
      sub.diags = diags;
      sub.diag = diag;
      for (uint32_t i = 0; i < d->u.type_decl.type_param_count; i++) {
        StigiTy *sv = (targs != NULL && i < ast_ty->type_arg_count) ? targs[i] : NULL;
        (void)push_ty_type_param(&sub, d->u.type_decl.type_params[i].name, sv);
      }
      for (uint32_t i = 0; i < d->u.type_decl.val_param_count; i++) {
        bool has_l = (largs != NULL && i < ast_ty->len_arg_count);
        StigiLinear lv = has_l ? largs[i] : stigi_linear_const(0);
        push_ty_local_lin(&sub, d->u.type_decl.val_params[i].name, alloc_ty(arena, STIGI_TY_U64),
                          false, has_l, lv);
      }
      StigiTy *rhs = resolve_ast_type_ctx(img, (uint32_t)tgt, d->u.type_decl.rhs, arena,
                                          lin_intern, &sub, diags, diag, depth + 1);
      ty_ctx_free(&sub);

      if (!d->u.type_decl.has_facts) {
        return rhs; /* Transparent type definition normalizes directly to substituted RHS (types.md 2) */
      }
      StigiTy *t = alloc_ty(arena, STIGI_TY_TYPEDEF);
      if (t == NULL) return NULL;
      t->unit_idx = (uint32_t)tgt;
      t->elem = rhs;
      t->type_args = targs;
      t->type_arg_count = targs != NULL ? ast_ty->type_arg_count : 0;
      t->len_args = largs;
      t->len_arg_count = largs != NULL ? ast_ty->len_arg_count : 0;
      return t;
    }
  }

  return alloc_ty(arena, STIGI_TY_UNKNOWN);
}

/* ---- First-order unification & substitution for [A: Type] ---- */

static bool unify_ty(const StigiTy *pat, const StigiTy *actual,
                     const uint32_t *tp_ids, StigiTy **subst, uint32_t ntp) {
  if (pat == NULL || actual == NULL) return true;
  if (actual->kind == STIGI_TY_ERROR || actual->kind == STIGI_TY_UNKNOWN ||
      actual->kind == STIGI_TY_NEVER) {
    return true;
  }
  if (pat->kind == STIGI_TY_VAR) {
    for (uint32_t i = 0; i < ntp; i++) {
      if (tp_ids[i] == pat->var_id) {
        if (subst[i] == NULL) {
          subst[i] = (StigiTy *)actual;
          return true;
        }
        return stigi_ty_equal(subst[i], actual);
      }
    }
    return true;
  }
  if (pat->kind == STIGI_TY_ARRAY && actual->kind == STIGI_TY_ARRAY) {
    return unify_ty(pat->elem, actual->elem, tp_ids, subst, ntp);
  }
  if (pat->kind == STIGI_TY_CELL && actual->kind == STIGI_TY_CELL) {
    return unify_ty(pat->elem, actual->elem, tp_ids, subst, ntp);
  }
  if (pat->kind == STIGI_TY_PTR && actual->kind == STIGI_TY_PTR) {
    return unify_ty(pat->elem, actual->elem, tp_ids, subst, ntp);
  }
  if (pat->kind == STIGI_TY_ENUM && actual->kind == STIGI_TY_ENUM &&
      pat->unit_idx == actual->unit_idx && pat->type_arg_count == actual->type_arg_count) {
    for (uint32_t i = 0; i < pat->type_arg_count; i++) {
      if (!unify_ty(pat->type_args[i], actual->type_args[i], tp_ids, subst, ntp)) return false;
    }
    return true;
  }
  if (pat->kind == STIGI_TY_TYPEDEF && actual->kind == STIGI_TY_TYPEDEF &&
      pat->unit_idx == actual->unit_idx && pat->type_arg_count == actual->type_arg_count) {
    for (uint32_t i = 0; i < pat->type_arg_count; i++) {
      if (!unify_ty(pat->type_args[i], actual->type_args[i], tp_ids, subst, ntp)) return false;
    }
    return true;
  }
  return true;
}

/* ---- Integer literal range check ---- */

static void check_int_lit_range(TyCtx *ctx, uint64_t val, bool is_neg,
                                const StigiTy *expected, StigiSpan sp) {
  const StigiTy *u = unwrap_ty_underlying(expected);
  if (u == NULL || !is_int_ty(u)) return;
  if (!int_lit_fits_ty(val, is_neg, u)) {
    (void)add_tdiag(ctx->diags, STIGI_TYPE_LITERAL_OUT_OF_RANGE, sp, ctx->diag,
                    "integer literal out of range");
  }
}

/* ---- Expression type checker ---- */

static StigiTy *check_expr(TyCtx *ctx, const StigiExpr *e, const StigiTy *expected);

static StigiTy *check_call_with_head(TyCtx *ctx, uint32_t callee_uidx, const char *callee_name,
                                     const StigiDefHead *h, const StigiExpr *call_expr,
                                     const StigiTy *expected) {
  if (call_expr->u.call.arg_count != h->param_count) {
    (void)add_tdiag(ctx->diags, STIGI_TYPE_ARITY, call_expr->span, ctx->diag,
                    "call to '%s' expects %u args, got %u",
                    callee_name, h->param_count, call_expr->u.call.arg_count);
    for (uint32_t i = 0; i < call_expr->u.call.arg_count; i++) {
      (void)check_expr(ctx, call_expr->u.call.args[i], NULL);
    }
    return alloc_ty(ctx->arena, STIGI_TY_ERROR);
  }

  uint32_t ntp = h->type_param_count;
  StigiTy **tp_subst = ntp > 0 ? arena_alloc(ctx->arena, (size_t)ntp * sizeof *tp_subst) : NULL;
  uint32_t *tp_ids = ntp > 0 ? malloc((size_t)ntp * sizeof *tp_ids) : NULL;
  if (ntp > 0 && (tp_subst == NULL || tp_ids == NULL)) {
    free(tp_ids);
    return alloc_ty(ctx->arena, STIGI_TY_ERROR);
  }
  if (ntp > 0) memset(tp_subst, 0, (size_t)ntp * sizeof *tp_subst);

  if (call_expr->u.call.type_arg_count > 0) {
    if (call_expr->u.call.type_arg_count != ntp) {
      (void)add_tdiag(ctx->diags, STIGI_TYPE_TYPE_ARITY, call_expr->span, ctx->diag,
                      "call to '%s' expects %u type args, got %u",
                      callee_name, ntp, call_expr->u.call.type_arg_count);
    }
    for (uint32_t i = 0; i < ntp && i < call_expr->u.call.type_arg_count; i++) {
      tp_subst[i] = resolve_ast_type_ctx(ctx->img, ctx->uidx, call_expr->u.call.type_args[i],
                                         ctx->arena, ctx->lin_intern, ctx,
                                         ctx->diags, ctx->diag, 0);
    }
  }

  /* Bind caller-side StigiLinear for every argument so dependent lengths match out-of-order too */
  TyCtx sub_ctx;
  memset(&sub_ctx, 0, sizeof sub_ctx);
  sub_ctx.img = ctx->img;
  sub_ctx.uidx = callee_uidx;
  sub_ctx.arena = ctx->arena;
  sub_ctx.lin_intern = ctx->lin_intern;
  for (uint32_t i = 0; i < ntp; i++) {
    tp_ids[i] = push_ty_type_param(&sub_ctx, h->type_params[i].name, tp_subst[i]);
  }
  for (uint32_t i = 0; i < h->param_count; i++) {
    StigiLinear lin = stigi_linear_const(0);
    bool ok_lin = (linear_from_expr_ctx(call_expr->u.call.args[i], ctx, ctx->lin_intern, &lin) == STIGI_OK);
    push_ty_local_lin(&sub_ctx, h->params[i].name, NULL, false, ok_lin, lin);
  }

  /* If generic with omitted type args, infer via first-order unification from args + expected */
  StigiTy **arg_tys = h->param_count > 0
                          ? arena_alloc(ctx->arena, (size_t)h->param_count * sizeof *arg_tys)
                          : NULL;
  if (ntp > 0 && call_expr->u.call.type_arg_count == 0) {
    if (expected != NULL && h->ret_type != NULL) {
      StigiTy *ret_pat = resolve_ast_type_ctx(ctx->img, callee_uidx, h->ret_type,
                                              ctx->arena, ctx->lin_intern, &sub_ctx, NULL, NULL, 0);
      (void)unify_ty(ret_pat, expected, tp_ids, tp_subst, ntp);
    }
    for (uint32_t i = 0; i < h->param_count; i++) {
      StigiTy *pat = resolve_ast_type_ctx(ctx->img, callee_uidx, h->params[i].type,
                                          ctx->arena, ctx->lin_intern, &sub_ctx, NULL, NULL, 0);
      const StigiTy *hint = (pat != NULL && pat->kind != STIGI_TY_VAR) ? pat : NULL;
      StigiTy *aty = check_expr(ctx, call_expr->u.call.args[i], hint);
      if (arg_tys != NULL) arg_tys[i] = aty;
      (void)unify_ty(pat, aty, tp_ids, tp_subst, ntp);
    }
    for (uint32_t i = 0; i < ntp; i++) {
      if (tp_subst[i] == NULL) {
        (void)add_tdiag(ctx->diags, STIGI_TYPE_CANNOT_INFER, call_expr->span, ctx->diag,
                        "cannot infer type argument '%.*s' in call to '%s'",
                        (int)h->type_params[i].name.len, h->type_params[i].name.ptr, callee_name);
        tp_subst[i] = alloc_ty(ctx->arena, STIGI_TY_ERROR);
      }
      sub_ctx.locals[i].type_subst = tp_subst[i];
    }
  }

  for (uint32_t i = 0; i < h->param_count; i++) {
    StigiTy *pty = resolve_ast_type_ctx(ctx->img, callee_uidx, h->params[i].type,
                                        ctx->arena, ctx->lin_intern, &sub_ctx,
                                        ctx->diags, ctx->diag, 0);
    StigiTy *aty = (ntp > 0 && call_expr->u.call.type_arg_count == 0 && arg_tys != NULL)
                       ? arg_tys[i]
                       : check_expr(ctx, call_expr->u.call.args[i], pty);
    if (!stigi_ty_assignable(pty, aty)) {
      (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, call_expr->u.call.args[i]->span,
                      ctx->diag, "argument %u type mismatch in call to '%s'",
                      i + 1, callee_name);
    }
  }

  StigiTy *ret = h->ret_type != NULL
                     ? resolve_ast_type_ctx(ctx->img, callee_uidx, h->ret_type,
                                            ctx->arena, ctx->lin_intern, &sub_ctx,
                                            ctx->diags, ctx->diag, 0)
                     : alloc_ty(ctx->arena, STIGI_TY_UNIT);
  ty_ctx_free(&sub_ctx);
  free(tp_ids);
  return ret;
}

static StigiTy *check_enum_ctor(TyCtx *ctx, uint32_t eidx, uint32_t cidx,
                                const StigiExpr *call_expr, StigiSpan span,
                                const StigiTy *expected) {
  const StigiDecl *ed = ctx->img->units[eidx].ast->decl;
  const StigiEnumCase *ec = &ed->u.enum_decl.cases[cidx];
  uint32_t argc = (call_expr != NULL) ? call_expr->u.call.arg_count : 0;
  uint32_t expl_tac = (call_expr != NULL) ? call_expr->u.call.type_arg_count : 0;
  uint32_t ntp = ed->u.enum_decl.type_param_count;

  if (argc != ec->field_count) {
    (void)add_tdiag(ctx->diags, STIGI_TYPE_ARITY, span, ctx->diag,
                    "constructor '%.*s' expects %u fields, got %u",
                    (int)ec->name.len, ec->name.ptr, ec->field_count, argc);
    if (call_expr != NULL) {
      for (uint32_t i = 0; i < argc; i++) (void)check_expr(ctx, call_expr->u.call.args[i], NULL);
    }
    return alloc_ty(ctx->arena, STIGI_TY_ERROR);
  }

  StigiTy **tp_subst = ntp > 0 ? arena_alloc(ctx->arena, (size_t)ntp * sizeof *tp_subst) : NULL;
  uint32_t *tp_ids = ntp > 0 ? malloc((size_t)ntp * sizeof *tp_ids) : NULL;
  if (ntp > 0 && (tp_subst == NULL || tp_ids == NULL)) {
    free(tp_ids);
    return alloc_ty(ctx->arena, STIGI_TY_ERROR);
  }
  if (ntp > 0) memset(tp_subst, 0, (size_t)ntp * sizeof *tp_subst);

  if (expl_tac > 0) {
    if (expl_tac != ntp) {
      (void)add_tdiag(ctx->diags, STIGI_TYPE_TYPE_ARITY, span, ctx->diag,
                      "constructor '%.*s' expects %u type args, got %u",
                      (int)ec->name.len, ec->name.ptr, ntp, expl_tac);
    }
    for (uint32_t i = 0; i < ntp && i < expl_tac; i++) {
      tp_subst[i] = resolve_ast_type_ctx(ctx->img, ctx->uidx, call_expr->u.call.type_args[i],
                                         ctx->arena, ctx->lin_intern, ctx,
                                         ctx->diags, ctx->diag, 0);
    }
  } else if (ntp > 0 && expected != NULL) {
    const StigiTy *ue = unwrap_ty_underlying(expected);
    if (ue != NULL && ue->kind == STIGI_TY_ENUM && ue->unit_idx == eidx) {
      for (uint32_t i = 0; i < ntp && i < ue->type_arg_count; i++) {
        tp_subst[i] = ue->type_args[i];
      }
    }
  }

  TyCtx sub;
  memset(&sub, 0, sizeof sub);
  sub.img = ctx->img;
  sub.uidx = eidx;
  sub.arena = ctx->arena;
  sub.lin_intern = ctx->lin_intern;
  for (uint32_t i = 0; i < ntp; i++) {
    tp_ids[i] = push_ty_type_param(&sub, ed->u.enum_decl.type_params[i].name, tp_subst[i]);
  }
  for (uint32_t f = 0; f < ec->field_count; f++) {
    StigiLinear lin = stigi_linear_const(0);
    bool ok_lin = (call_expr != NULL &&
                   linear_from_expr_ctx(call_expr->u.call.args[f], ctx, ctx->lin_intern, &lin) == STIGI_OK);
    push_ty_local_lin(&sub, ec->fields[f].name, NULL, false, ok_lin, lin);
  }

  StigiTy **arg_tys = argc > 0 ? arena_alloc(ctx->arena, (size_t)argc * sizeof *arg_tys) : NULL;
  if (ntp > 0) {
    for (uint32_t f = 0; f < argc; f++) {
      StigiTy *fpat = resolve_ast_type_ctx(ctx->img, eidx, ec->fields[f].type,
                                           ctx->arena, ctx->lin_intern, &sub, NULL, NULL, 0);
      const StigiTy *hint = (fpat != NULL && fpat->kind != STIGI_TY_VAR) ? fpat : NULL;
      StigiTy *aty = check_expr(ctx, call_expr->u.call.args[f], hint);
      if (arg_tys != NULL) arg_tys[f] = aty;
      (void)unify_ty(fpat, aty, tp_ids, tp_subst, ntp);
    }
    for (uint32_t i = 0; i < ntp; i++) {
      if (tp_subst[i] == NULL) {
        (void)add_tdiag(ctx->diags, STIGI_TYPE_CANNOT_INFER, span, ctx->diag,
                        "cannot infer type argument '%.*s' for enum '%s'",
                        (int)ed->u.enum_decl.type_params[i].name.len,
                        ed->u.enum_decl.type_params[i].name.ptr, ctx->img->units[eidx].name);
        tp_subst[i] = alloc_ty(ctx->arena, STIGI_TY_ERROR);
      }
      sub.locals[i].type_subst = tp_subst[i];
    }
  }

  for (uint32_t f = 0; f < argc; f++) {
    StigiTy *fty = resolve_ast_type_ctx(ctx->img, eidx, ec->fields[f].type,
                                        ctx->arena, ctx->lin_intern, &sub,
                                        ctx->diags, ctx->diag, 0);
    StigiTy *aty = (ntp > 0 && arg_tys != NULL) ? arg_tys[f]
                                                : check_expr(ctx, call_expr->u.call.args[f], fty);
    if (!stigi_ty_assignable(fty, aty)) {
      (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, call_expr->u.call.args[f]->span,
                      ctx->diag, "field %u type mismatch in constructor '%.*s'",
                      f + 1, (int)ec->name.len, ec->name.ptr);
    }
  }

  ty_ctx_free(&sub);
  free(tp_ids);

  StigiTy *res = alloc_ty(ctx->arena, STIGI_TY_ENUM);
  if (res != NULL) {
    res->unit_idx = eidx;
    res->type_args = tp_subst;
    res->type_arg_count = ntp;
  }
  return res;
}

static StigiTy *check_expr(TyCtx *ctx, const StigiExpr *e, const StigiTy *expected) {
  if (e == NULL) return alloc_ty(ctx->arena, STIGI_TY_UNIT);
  switch (e->kind) {
    case STIGI_EXPR_LIT_INT:
      if (e->u.lit_int.overflow) {
        (void)add_tdiag(ctx->diags, STIGI_TYPE_LITERAL_OUT_OF_RANGE, e->span, ctx->diag,
                        "integer literal exceeds U64 range");
      }
      if (expected != NULL) {
        const StigiTy *u = unwrap_ty_underlying(expected);
        if (is_int_ty(u)) {
          check_int_lit_range(ctx, e->u.lit_int.value, false, u, e->span);
          return alloc_ty(ctx->arena, u->kind);
        }
      }
      return alloc_ty(ctx->arena, STIGI_TY_U64);
    case STIGI_EXPR_LIT_CHAR:
      return alloc_ty(ctx->arena, STIGI_TY_U8);
    case STIGI_EXPR_LIT_STR: {
      StigiTy *t = alloc_ty(ctx->arena, STIGI_TY_ARRAY);
      t->elem = alloc_ty(ctx->arena, STIGI_TY_U8);
      t->len = stigi_linear_const((int64_t)e->u.lit_str.decoded_len);
      t->has_len = true;
      return t;
    }
    case STIGI_EXPR_LIT_BOOL:
      return alloc_ty(ctx->arena, STIGI_TY_BOOL);
    case STIGI_EXPR_LIT_UNIT:
      return alloc_ty(ctx->arena, STIGI_TY_UNIT);
    case STIGI_EXPR_LOCAL:
    case STIGI_EXPR_GLOBAL: {
      const TyBinding *b = find_local(ctx, e->u.ident.name);
      if (b != NULL && b->ty != NULL) return b->ty;
      int32_t tgt = stigi_image_resolve_name(ctx->img, ctx->uidx,
                                             e->u.ident.name.ptr, e->u.ident.name.len);
      if (tgt >= 0) {
        const StigiDecl *d = ctx->img->units[tgt].ast->decl;
        if (d->kind == STIGI_DECL_VAL) {
          return resolve_ast_type(ctx->img, (uint32_t)tgt, d->u.val_decl.type,
                                  ctx->arena, ctx->lin_intern, ctx->diags, ctx->diag, 0);
        }
        if (d->kind == STIGI_DECL_EXTERN && d->u.extern_decl.is_val) {
          return resolve_ast_type(ctx->img, (uint32_t)tgt, d->u.extern_decl.val_type,
                                  ctx->arena, ctx->lin_intern, ctx->diags, ctx->diag, 0);
        }
      }
      /* Nullary enum constructor (qualified or unqualified) */
      uint32_t cidx = 0;
      bool amb = false;
      int32_t eidx = stigi_image_resolve_enum_case(ctx->img, ctx->uidx, e->u.ident.name.ptr,
                                                   e->u.ident.name.len, &cidx, &amb);
      if (eidx >= 0) {
        return check_enum_ctor(ctx, (uint32_t)eidx, cidx, NULL, e->span, expected);
      }
      return alloc_ty(ctx->arena, STIGI_TY_UNKNOWN);
    }
    case STIGI_EXPR_UNOP: {
      if (e->u.unop.op == STIGI_OP_NEG && e->u.unop.operand->kind == STIGI_EXPR_LIT_INT) {
        if (e->u.unop.operand->u.lit_int.overflow) {
          (void)add_tdiag(ctx->diags, STIGI_TYPE_LITERAL_OUT_OF_RANGE, e->span, ctx->diag,
                          "integer literal out of range");
        }
        const StigiTy *u = unwrap_ty_underlying(expected);
        if (u != NULL && is_int_ty(u)) {
          check_int_lit_range(ctx, e->u.unop.operand->u.lit_int.value, true, u, e->span);
          return alloc_ty(ctx->arena, u->kind);
        }
        check_int_lit_range(ctx, e->u.unop.operand->u.lit_int.value, true,
                            alloc_ty(ctx->arena, STIGI_TY_I64), e->span);
        return alloc_ty(ctx->arena, STIGI_TY_I64);
      }
      StigiTy *opnd = check_expr(ctx, e->u.unop.operand,
                                 e->u.unop.op == STIGI_OP_NOT ? alloc_ty(ctx->arena, STIGI_TY_BOOL) : expected);
      if (e->u.unop.op == STIGI_OP_NOT) {
        StigiTy *bty = alloc_ty(ctx->arena, STIGI_TY_BOOL);
        if (!stigi_ty_assignable(bty, opnd)) {
          (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, e->span, ctx->diag,
                          "'!' requires Bool operand");
        }
        return bty;
      }
      /* Unary '-' requires signed integer */
      const StigiTy *u = unwrap_ty_underlying(opnd);
      if (opnd->kind != STIGI_TY_ERROR && opnd->kind != STIGI_TY_NEVER &&
          (!is_int_ty(u) || is_unsigned_int_ty(u))) {
        (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, e->span, ctx->diag,
                        "unary '-' requires a signed integer operand");
      }
      return opnd;
    }
    case STIGI_EXPR_BINOP: {
      StigiOp op = e->u.binop.op;
      if (op == STIGI_OP_AND || op == STIGI_OP_OR) {
        StigiTy *bty = alloc_ty(ctx->arena, STIGI_TY_BOOL);
        StigiTy *lt = check_expr(ctx, e->u.binop.lhs, bty);
        StigiTy *rt = check_expr(ctx, e->u.binop.rhs, bty);
        if (!stigi_ty_assignable(bty, lt) || !stigi_ty_assignable(bty, rt)) {
          (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, e->span, ctx->diag,
                          "logical operator requires Bool operands");
        }
        return bty;
      }
      const StigiTy *hint = (op >= STIGI_OP_ADD && op <= STIGI_OP_MOD) ? expected : NULL;
      StigiTy *lt = check_expr(ctx, e->u.binop.lhs, hint);
      const StigiTy *rhint = (lt->kind != STIGI_TY_UNKNOWN && lt->kind != STIGI_TY_ERROR &&
                              lt->kind != STIGI_TY_NEVER) ? lt : hint;
      StigiTy *rt = check_expr(ctx, e->u.binop.rhs, rhint);
      if (!stigi_ty_assignable(lt, rt) && !stigi_ty_assignable(rt, lt)) {
        (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, e->span, ctx->diag,
                        "binary operator operand type mismatch");
      }
      if ((op >= STIGI_OP_ADD && op <= STIGI_OP_MOD) ||
          (op >= STIGI_OP_LT && op <= STIGI_OP_GE)) {
        if (lt->kind != STIGI_TY_ERROR && lt->kind != STIGI_TY_NEVER && !is_numeric_ty(lt)) {
          (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, e->span, ctx->diag,
                          "arithmetic/ordering operator requires integer operands");
        }
      }
      if (op >= STIGI_OP_LT && op <= STIGI_OP_NE) {
        return alloc_ty(ctx->arena, STIGI_TY_BOOL);
      }
      const StigiTy *ul = unwrap_ty_underlying(lt);
      return ul != NULL ? (StigiTy *)ul : lt;
    }
    case STIGI_EXPR_IF: {
      StigiTy *bty = alloc_ty(ctx->arena, STIGI_TY_BOOL);
      StigiTy *ct = check_expr(ctx, e->u.if_expr.cond, bty);
      if (!stigi_ty_assignable(bty, ct)) {
        (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, e->u.if_expr.cond->span, ctx->diag,
                        "'if' condition must be Bool");
      }
      StigiTy *tt = check_expr(ctx, e->u.if_expr.then_br, expected);
      const StigiTy *ehint = expected != NULL ? expected
                                              : (tt->kind != STIGI_TY_NEVER ? tt : NULL);
      StigiTy *et = check_expr(ctx, e->u.if_expr.else_br, ehint);
      if (tt->kind == STIGI_TY_NEVER) return et;
      if (et->kind == STIGI_TY_NEVER) return tt;
      if (!stigi_ty_assignable(tt, et) && !stigi_ty_assignable(et, tt)) {
        (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, e->span, ctx->diag,
                        "'if' branch types do not match");
      }
      return tt;
    }
    case STIGI_EXPR_WHILE: {
      StigiTy *bty = alloc_ty(ctx->arena, STIGI_TY_BOOL);
      StigiTy *ct = check_expr(ctx, e->u.while_expr.cond, bty);
      if (!stigi_ty_assignable(bty, ct)) {
        (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, e->u.while_expr.cond->span, ctx->diag,
                        "'while' condition must be Bool");
      }
      if (e->u.while_expr.decreases != NULL) {
        StigiTy *u64 = alloc_ty(ctx->arena, STIGI_TY_U64);
        StigiTy *dt = check_expr(ctx, e->u.while_expr.decreases, u64);
        if (!stigi_ty_assignable(u64, dt)) {
          (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, e->u.while_expr.decreases->span,
                          ctx->diag, "'decreases' measure must have type U64");
        }
      }
      (void)check_expr(ctx, e->u.while_expr.body, NULL);
      return alloc_ty(ctx->arena, STIGI_TY_UNIT);
    }
    case STIGI_EXPR_VAL: {
      StigiTy *ann = e->u.val_stmt.type != NULL
                         ? resolve_ast_type_ctx(ctx->img, ctx->uidx, e->u.val_stmt.type,
                                                ctx->arena, ctx->lin_intern, ctx,
                                                ctx->diags, ctx->diag, 0)
                         : NULL;
      StigiTy *it = check_expr(ctx, e->u.val_stmt.init, ann);
      if (ann != NULL && !stigi_ty_assignable(ann, it)) {
        (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, e->span, ctx->diag,
                        "val initializer type does not match annotation");
      }
      StigiTy *bound_ty = ann != NULL ? ann : it;
      if (e->u.val_stmt.pat.kind == STIGI_PAT_VAR) {
        StigiLinear lin = stigi_linear_const(0);
        bool ok_lin = (linear_from_expr_ctx(e->u.val_stmt.init, ctx, ctx->lin_intern, &lin) == STIGI_OK);
        push_ty_local_lin(ctx, e->u.val_stmt.pat.name, bound_ty, false, ok_lin, lin);
      } else if (e->u.val_stmt.pat.kind == STIGI_PAT_CTOR) {
        const StigiPat *pat = &e->u.val_stmt.pat;
        const StigiTy *ub = unwrap_ty_underlying(bound_ty);
        if (ub != NULL && ub->kind == STIGI_TY_ENUM && ub->unit_idx < ctx->img->unit_count) {
          const StigiDecl *ed = ctx->img->units[ub->unit_idx].ast->decl;
          if (ed->u.enum_decl.case_count != 1) {
            (void)add_tdiag(ctx->diags, STIGI_TYPE_NOT_EXHAUSTIVE, pat->span, ctx->diag,
                            "destructuring 'val' requires a single-case enum");
          }
          uint32_t cidx = 0;
          bool amb = false;
          int32_t re = stigi_image_resolve_enum_case(ctx->img, ctx->uidx, pat->name.ptr,
                                                     pat->name.len, &cidx, &amb);
          if (re == (int32_t)ub->unit_idx) {
            const StigiEnumCase *ec = &ed->u.enum_decl.cases[cidx];
            if (pat->field_count != ec->field_count) {
              (void)add_tdiag(ctx->diags, STIGI_TYPE_PATTERN_ARITY, pat->span, ctx->diag,
                              "pattern '%.*s' expects %u fields, got %u",
                              (int)ec->name.len, ec->name.ptr, ec->field_count, pat->field_count);
            }
            TyCtx fctx;
            memset(&fctx, 0, sizeof fctx);
            fctx.img = ctx->img;
            fctx.uidx = ub->unit_idx;
            fctx.arena = ctx->arena;
            fctx.lin_intern = ctx->lin_intern;
            for (uint32_t tp = 0; tp < ed->u.enum_decl.type_param_count; tp++) {
              StigiTy *sv = tp < ub->type_arg_count ? ub->type_args[tp] : NULL;
              (void)push_ty_type_param(&fctx, ed->u.enum_decl.type_params[tp].name, sv);
            }
            for (uint32_t f = 0; f < pat->field_count && f < ec->field_count; f++) {
              StigiTy *fty = resolve_ast_type_ctx(ctx->img, ub->unit_idx, ec->fields[f].type,
                                                  ctx->arena, ctx->lin_intern, &fctx,
                                                  ctx->diags, ctx->diag, 0);
              push_ty_local(ctx, pat->fields[f], fty, false);
              if (ctx->local_count > 0) {
                push_ty_local_lin(&fctx, ec->fields[f].name, fty, false, true,
                                  stigi_linear_var(ctx->locals[ctx->local_count - 1].binder_id));
              }
            }
            ty_ctx_free(&fctx);
          } else {
            (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, pat->span, ctx->diag,
                            "pattern '%.*s' does not match enum '%s'",
                            (int)pat->name.len, pat->name.ptr, ctx->img->units[ub->unit_idx].name);
          }
        } else if (bound_ty->kind != STIGI_TY_ERROR) {
          (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, pat->span, ctx->diag,
                          "constructor pattern in 'val' requires an enum initializer");
        }
      }
      return alloc_ty(ctx->arena, STIGI_TY_UNIT);
    }
    case STIGI_EXPR_VAR: {
      StigiTy *ann = e->u.var_stmt.type != NULL
                         ? resolve_ast_type_ctx(ctx->img, ctx->uidx, e->u.var_stmt.type,
                                                ctx->arena, ctx->lin_intern, ctx,
                                                ctx->diags, ctx->diag, 0)
                         : NULL;
      StigiTy *it = check_expr(ctx, e->u.var_stmt.init, ann);
      if (ann != NULL && !stigi_ty_assignable(ann, it)) {
        (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, e->span, ctx->diag,
                        "var initializer type does not match annotation");
      }
      push_ty_local(ctx, e->u.var_stmt.name, ann != NULL ? ann : it, true);
      return alloc_ty(ctx->arena, STIGI_TY_UNIT);
    }
    case STIGI_EXPR_ASSIGN: {
      const TyBinding *b = find_local(ctx, e->u.assign_stmt.name);
      if (b == NULL || !b->is_mut) {
        (void)add_tdiag(ctx->diags, STIGI_TYPE_ASSIGN_VAL, e->span, ctx->diag,
                        "cannot assign to immutable or non-local binding '%.*s'",
                        (int)e->u.assign_stmt.name.len, e->u.assign_stmt.name.ptr);
      }
      StigiTy *vt = check_expr(ctx, e->u.assign_stmt.value, b != NULL ? b->ty : NULL);
      if (b != NULL && b->ty != NULL && !stigi_ty_assignable(b->ty, vt)) {
        (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, e->span, ctx->diag,
                        "assignment type mismatch");
      }
      return alloc_ty(ctx->arena, STIGI_TY_UNIT);
    }
    case STIGI_EXPR_INDEX_READ: {
      StigiTy *tt = check_expr(ctx, e->u.index_expr.target, NULL);
      StigiTy *u64 = alloc_ty(ctx->arena, STIGI_TY_U64);
      StigiTy *it = check_expr(ctx, e->u.index_expr.index, u64);
      if (!stigi_ty_assignable(u64, it)) {
        (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, e->u.index_expr.index->span, ctx->diag,
                        "index expression must have type U64");
      }
      const StigiTy *ut = unwrap_ty_underlying(tt);
      if (ut != NULL && (ut->kind == STIGI_TY_ARRAY || ut->kind == STIGI_TY_CELL ||
                         ut->kind == STIGI_TY_PTR)) {
        return ut->elem != NULL ? ut->elem : alloc_ty(ctx->arena, STIGI_TY_ERROR);
      }
      if (tt->kind != STIGI_TY_ERROR) {
        (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, e->span, ctx->diag,
                        "cannot index non-array/cell value");
      }
      return alloc_ty(ctx->arena, STIGI_TY_ERROR);
    }
    case STIGI_EXPR_INDEX_WRITE: {
      StigiTy *tt = check_expr(ctx, e->u.index_expr.target, NULL);
      StigiTy *u64 = alloc_ty(ctx->arena, STIGI_TY_U64);
      StigiTy *it = check_expr(ctx, e->u.index_expr.index, u64);
      if (!stigi_ty_assignable(u64, it)) {
        (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, e->u.index_expr.index->span, ctx->diag,
                        "index write offset must have type U64");
      }
      const StigiTy *ut = unwrap_ty_underlying(tt);
      StigiTy *elem_ty = NULL;
      if (ut != NULL && (ut->kind == STIGI_TY_ARRAY || ut->kind == STIGI_TY_CELL ||
                         ut->kind == STIGI_TY_PTR)) {
        elem_ty = ut->elem;
      } else if (tt->kind != STIGI_TY_ERROR) {
        (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, e->span, ctx->diag,
                        "index write target must be an Array or Cell");
      }
      StigiTy *vt = check_expr(ctx, e->u.index_expr.value, elem_ty);
      if (elem_ty != NULL && !stigi_ty_assignable(elem_ty, vt)) {
        (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, e->u.index_expr.value->span, ctx->diag,
                        "index write value type does not match element type");
      }
      return alloc_ty(ctx->arena, STIGI_TY_UNIT);
    }
    case STIGI_EXPR_CALL: {
      /* C-DSL `size[A]` returns U64 */
      if (e->u.call.callee->kind == STIGI_EXPR_LOCAL &&
          str_eq_c(e->u.call.callee->u.ident.name, "size") &&
          e->u.call.type_arg_count == 1 && !e->u.call.has_parens) {
        (void)resolve_ast_type_ctx(ctx->img, ctx->uidx, e->u.call.type_args[0],
                                   ctx->arena, ctx->lin_intern, ctx, ctx->diags, ctx->diag, 0);
        return alloc_ty(ctx->arena, STIGI_TY_U64);
      }
      /* Local array / cell element read */
      if (e->u.call.callee->kind == STIGI_EXPR_LOCAL) {
        const TyBinding *b = find_local(ctx, e->u.call.callee->u.ident.name);
        if (b != NULL && b->ty != NULL) {
          const StigiTy *ub = unwrap_ty_underlying(b->ty);
          if (ub != NULL && (ub->kind == STIGI_TY_ARRAY || ub->kind == STIGI_TY_CELL ||
                             ub->kind == STIGI_TY_PTR)) {
            if (e->u.call.arg_count != 1) {
              (void)add_tdiag(ctx->diags, STIGI_TYPE_ARITY, e->span, ctx->diag,
                              "array/cell indexing expects 1 argument, got %u",
                              e->u.call.arg_count);
            } else {
              StigiTy *u64 = alloc_ty(ctx->arena, STIGI_TY_U64);
              StigiTy *it = check_expr(ctx, e->u.call.args[0], u64);
              if (!stigi_ty_assignable(u64, it)) {
                (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, e->u.call.args[0]->span,
                                ctx->diag, "array index must have type U64");
              }
            }
            return ub->elem != NULL ? ub->elem : alloc_ty(ctx->arena, STIGI_TY_ERROR);
          }
        }
      }
      if (e->u.call.callee->kind == STIGI_EXPR_LOCAL ||
          e->u.call.callee->kind == STIGI_EXPR_GLOBAL) {
        StigiStr cn = e->u.call.callee->u.ident.name;
        if (str_eq_c(cn, "Stigi.Test.expect")) {
          if (e->u.call.arg_count != 3) {
            (void)add_tdiag(ctx->diags, STIGI_TYPE_ARITY, e->span, ctx->diag,
                            "Stigi.Test.expect requires 3 arguments");
          } else {
            StigiTy *t0 = check_expr(ctx, e->u.call.args[0], NULL);
            StigiTy *t1 = check_expr(ctx, e->u.call.args[1], t0);
            (void)check_expr(ctx, e->u.call.args[2], NULL);
            if (!stigi_ty_assignable(t0, t1) && !stigi_ty_assignable(t1, t0)) {
              (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, e->span, ctx->diag,
                              "Stigi.Test.expect operands must have same type");
            }
          }
          return alloc_ty(ctx->arena, STIGI_TY_TEST_RESULT);
        }
        if (str_eq_c(cn, "Stigi.NegTest.expect")) {
          for (uint32_t i = 0; i < e->u.call.arg_count; i++) {
            (void)check_expr(ctx, e->u.call.args[i], NULL);
          }
          return alloc_ty(ctx->arena, STIGI_TY_NEVER);
        }
        /* Builtin integer & bit conversions */
        if (str_eq_c(cn, "U64.ofU8") || str_eq_c(cn, "U64.ofI64") ||
            str_eq_c(cn, "U64.ofI32") || str_eq_c(cn, "U64.ofU32")) {
          StigiTyKind ik = STIGI_TY_U32;
          if (str_eq_c(cn, "U64.ofU8")) ik = STIGI_TY_U8;
          else if (str_eq_c(cn, "U64.ofI64")) ik = STIGI_TY_I64;
          else if (str_eq_c(cn, "U64.ofI32")) ik = STIGI_TY_I32;
          StigiTy *in_ty = alloc_ty(ctx->arena, ik);
          if (e->u.call.arg_count == 1) {
            StigiTy *at = check_expr(ctx, e->u.call.args[0], in_ty);
            if (!stigi_ty_assignable(in_ty, at)) {
              (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, e->span, ctx->diag, "conversion arg mismatch");
            }
          }
          return alloc_ty(ctx->arena, STIGI_TY_U64);
        }
        if (str_eq_c(cn, "U8.ofU64") || str_eq_c(cn, "U32.ofU64") ||
            str_eq_c(cn, "I64.ofU64") || str_eq_c(cn, "I32.ofU64")) {
          StigiTy *u64 = alloc_ty(ctx->arena, STIGI_TY_U64);
          if (e->u.call.arg_count == 1) {
            StigiTy *at = check_expr(ctx, e->u.call.args[0], u64);
            if (!stigi_ty_assignable(u64, at)) {
              (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, e->span, ctx->diag, "conversion arg mismatch");
            }
          }
          StigiTyKind rk = STIGI_TY_U32;
          if (str_eq_c(cn, "U8.ofU64")) rk = STIGI_TY_U8;
          else if (str_eq_c(cn, "I64.ofU64")) rk = STIGI_TY_I64;
          else if (str_eq_c(cn, "I32.ofU64")) rk = STIGI_TY_I32;
          return alloc_ty(ctx->arena, rk);
        }
        if (str_eq_c(cn, "U32.wrappingAdd") || str_eq_c(cn, "U32.wrappingSubtract") ||
            str_eq_c(cn, "U32.wrappingMultiply") || str_eq_c(cn, "U32.rotateRight") ||
            str_eq_c(cn, "U32.bitwiseXor") || str_eq_c(cn, "U32.bitwiseAnd") ||
            str_eq_c(cn, "U32.bitwiseOr") || str_eq_c(cn, "U32.shiftLeft") ||
            str_eq_c(cn, "U32.shiftRight")) {
          StigiTy *u32 = alloc_ty(ctx->arena, STIGI_TY_U32);
          for (uint32_t i = 0; i < e->u.call.arg_count; i++) {
            StigiTy *at = check_expr(ctx, e->u.call.args[i], u32);
            if (!stigi_ty_assignable(u32, at)) {
              (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, e->u.call.args[i]->span,
                              ctx->diag, "U32 bitwise/wrapping op expects U32");
            }
          }
          return u32;
        }
        if (str_eq_c(cn, "U64.mix") || str_eq_c(cn, "U64.wrappingAdd") ||
            str_eq_c(cn, "U64.wrappingSubtract") || str_eq_c(cn, "U64.wrappingMultiply") ||
            str_eq_c(cn, "U64.bitwiseAnd") || str_eq_c(cn, "U64.bitwiseOr") ||
            str_eq_c(cn, "U64.bitwiseXor") || str_eq_c(cn, "U64.shiftLeft") ||
            str_eq_c(cn, "U64.shiftRight")) {
          StigiTy *u64 = alloc_ty(ctx->arena, STIGI_TY_U64);
          for (uint32_t i = 0; i < e->u.call.arg_count; i++) {
            StigiTy *at = check_expr(ctx, e->u.call.args[i], u64);
            if (!stigi_ty_assignable(u64, at)) {
              (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, e->u.call.args[i]->span,
                              ctx->diag, "U64 bitwise/wrapping op expects U64");
            }
          }
          return u64;
        }
        /* Builtin region allocation calls: Array.tryFill, Array.tryGrow, Cell.tryMake */
        if (str_eq_c(cn, "Array.tryFill")) {
          StigiTy *res = alloc_ty(ctx->arena, STIGI_TY_FILL_RESULT);
          if (e->u.call.arg_count != 2) {
            (void)add_tdiag(ctx->diags, STIGI_TYPE_ARITY, e->span, ctx->diag,
                            "Array.tryFill expects 2 arguments (cap, init), got %u",
                            e->u.call.arg_count);
            for (uint32_t i = 0; i < e->u.call.arg_count; i++) {
              (void)check_expr(ctx, e->u.call.args[i], NULL);
            }
            return res;
          }
          StigiTy *u64 = alloc_ty(ctx->arena, STIGI_TY_U64);
          StigiTy *ct = check_expr(ctx, e->u.call.args[0], u64);
          if (!stigi_ty_assignable(u64, ct)) {
            (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, e->u.call.args[0]->span,
                            ctx->diag, "Array.tryFill capacity must have type U64");
          }
          if (linear_from_expr_ctx(e->u.call.args[0], ctx, ctx->lin_intern, &res->len) == STIGI_OK) {
            res->has_len = true;
          }
          StigiTy *elem_hint = NULL;
          if (e->u.call.type_arg_count == 1) {
            elem_hint = resolve_ast_type_ctx(ctx->img, ctx->uidx, e->u.call.type_args[0],
                                             ctx->arena, ctx->lin_intern, ctx,
                                             ctx->diags, ctx->diag, 0);
          } else if (e->u.call.type_arg_count > 1) {
            (void)add_tdiag(ctx->diags, STIGI_TYPE_TYPE_ARITY, e->span, ctx->diag,
                            "Array.tryFill expects at most 1 type argument");
          }
          const StigiExpr *init_e = e->u.call.args[1];
          if (elem_hint == NULL && init_e->kind == STIGI_EXPR_LIT_INT &&
              !init_e->u.lit_int.overflow) {
            StigiTy *lt = alloc_ty(ctx->arena, STIGI_TY_INT_LIT);
            lt->lit_val = init_e->u.lit_int.value;
            lt->lit_neg = false;
            res->elem = lt;
          } else if (elem_hint == NULL && init_e->kind == STIGI_EXPR_UNOP &&
                     init_e->u.unop.op == STIGI_OP_NEG &&
                     init_e->u.unop.operand != NULL &&
                     init_e->u.unop.operand->kind == STIGI_EXPR_LIT_INT &&
                     !init_e->u.unop.operand->u.lit_int.overflow) {
            StigiTy *lt = alloc_ty(ctx->arena, STIGI_TY_INT_LIT);
            lt->lit_val = init_e->u.unop.operand->u.lit_int.value;
            lt->lit_neg = true;
            res->elem = lt;
          } else {
            StigiTy *it = check_expr(ctx, init_e, elem_hint);
            if (elem_hint != NULL && !stigi_ty_assignable(elem_hint, it)) {
              (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, init_e->span, ctx->diag,
                              "Array.tryFill initializer does not match element type");
            }
            res->elem = elem_hint != NULL ? elem_hint : it;
          }
          return res;
        }
        if (str_eq_c(cn, "Array.tryGrow")) {
          StigiTy *res = alloc_ty(ctx->arena, STIGI_TY_GROW_RESULT);
          if (e->u.call.arg_count != 3) {
            (void)add_tdiag(ctx->diags, STIGI_TYPE_ARITY, e->span, ctx->diag,
                            "Array.tryGrow expects 3 arguments (arr, nextCap, fill), got %u",
                            e->u.call.arg_count);
            for (uint32_t i = 0; i < e->u.call.arg_count; i++) {
              (void)check_expr(ctx, e->u.call.args[i], NULL);
            }
            return res;
          }
          StigiTy *at = check_expr(ctx, e->u.call.args[0], NULL);
          const StigiTy *ua = unwrap_ty_underlying(at);
          StigiTy *elem_ty = alloc_ty(ctx->arena, STIGI_TY_ERROR);
          if (ua != NULL && ua->kind == STIGI_TY_ARRAY) {
            elem_ty = ua->elem != NULL ? ua->elem : alloc_ty(ctx->arena, STIGI_TY_ERROR);
            res->old_len = ua->len;
            res->has_old_len = ua->has_len;
          } else if (at->kind != STIGI_TY_ERROR) {
            (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, e->u.call.args[0]->span,
                            ctx->diag, "Array.tryGrow first argument must be an Array");
          }
          StigiTy *u64 = alloc_ty(ctx->arena, STIGI_TY_U64);
          StigiTy *ct = check_expr(ctx, e->u.call.args[1], u64);
          if (!stigi_ty_assignable(u64, ct)) {
            (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, e->u.call.args[1]->span,
                            ctx->diag, "Array.tryGrow new capacity must have type U64");
          }
          if (linear_from_expr_ctx(e->u.call.args[1], ctx, ctx->lin_intern, &res->len) == STIGI_OK) {
            res->has_len = true;
          }
          StigiTy *ft = check_expr(ctx, e->u.call.args[2], elem_ty);
          if (!stigi_ty_assignable(elem_ty, ft)) {
            (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, e->u.call.args[2]->span,
                            ctx->diag, "Array.tryGrow fill value does not match element type");
          }
          res->elem = elem_ty;
          return res;
        }
        if (str_eq_c(cn, "Cell.tryMake")) {
          StigiTy *res = alloc_ty(ctx->arena, STIGI_TY_MAKE_RESULT);
          if (e->u.call.arg_count != 1) {
            (void)add_tdiag(ctx->diags, STIGI_TYPE_ARITY, e->span, ctx->diag,
                            "Cell.tryMake expects 1 argument (init), got %u",
                            e->u.call.arg_count);
            for (uint32_t i = 0; i < e->u.call.arg_count; i++) {
              (void)check_expr(ctx, e->u.call.args[i], NULL);
            }
            return res;
          }
          StigiTy *elem_hint = NULL;
          if (e->u.call.type_arg_count == 1) {
            elem_hint = resolve_ast_type_ctx(ctx->img, ctx->uidx, e->u.call.type_args[0],
                                             ctx->arena, ctx->lin_intern, ctx,
                                             ctx->diags, ctx->diag, 0);
          }
          const StigiExpr *init_e = e->u.call.args[0];
          if (elem_hint == NULL && init_e->kind == STIGI_EXPR_LIT_INT &&
              !init_e->u.lit_int.overflow) {
            StigiTy *lt = alloc_ty(ctx->arena, STIGI_TY_INT_LIT);
            lt->lit_val = init_e->u.lit_int.value;
            lt->lit_neg = false;
            res->elem = lt;
          } else {
            StigiTy *it = check_expr(ctx, init_e, elem_hint);
            if (elem_hint != NULL && !stigi_ty_assignable(elem_hint, it)) {
              (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, init_e->span, ctx->diag,
                              "Cell.tryMake initializer does not match element type");
            }
            res->elem = elem_hint != NULL ? elem_hint : it;
          }
          return res;
        }

        /* Capability operation call `binder.op(...)` or unnamed `Cap.op(...)` (S125) */
        const char *dot = NULL;
        for (size_t i = 0; i < cn.len; i++) {
          if (cn.ptr[i] == '.') dot = cn.ptr + i;
        }
        if (dot != NULL) {
          StigiStr prefix = { cn.ptr, (size_t)(dot - cn.ptr) };
          StigiStr op_name = { dot + 1, cn.len - prefix.len - 1U };
          int32_t cap_idx = -1;
          const TyBinding *cb = find_local(ctx, prefix);
          if (cb != NULL && cb->cap_unit_idx >= 0) {
            cap_idx = cb->cap_unit_idx;
          } else {
            int32_t cand_cap = stigi_image_resolve_name(ctx->img, ctx->uidx,
                                                        prefix.ptr, prefix.len);
            if (cand_cap >= 0 &&
                ctx->img->units[cand_cap].ast->decl->kind == STIGI_DECL_CAPABILITY &&
                find_cap_in_scope(ctx, cand_cap) >= 0) {
              cap_idx = cand_cap;
            } else if (cand_cap >= 0 &&
                       ctx->img->units[cand_cap].ast->decl->kind == STIGI_DECL_TRAIT) {
              cap_idx = cand_cap;
            }
          }
          if (cap_idx >= 0) {
            uint32_t cidx = (uint32_t)cap_idx;
            const StigiDecl *cd = ctx->img->units[cidx].ast->decl;
            if (cd->kind == STIGI_DECL_CAPABILITY) {
              for (uint32_t m = 0; m < cd->u.cap_decl.member_count; m++) {
                const StigiDecl *op = cd->u.cap_decl.members[m];
                if (op->kind == STIGI_DECL_DEF && str_eq_ss(op->name, op_name)) {
                  return check_call_with_head(ctx, cidx, ctx->img->units[cidx].name,
                                              &op->u.def_decl.head, e, expected);
                }
              }
            } else if (cd->kind == STIGI_DECL_TRAIT) {
              for (uint32_t m = 0; m < cd->u.trait_decl.member_count; m++) {
                const StigiDecl *op = cd->u.trait_decl.members[m];
                if (op->kind == STIGI_DECL_DEF && str_eq_ss(op->name, op_name)) {
                  /* Combine trait's type parameters with op's head so inference works cleanly */
                  StigiDefHead combined = op->u.def_decl.head;
                  if (cd->u.trait_decl.type_param_count > 0 && combined.type_param_count == 0) {
                    combined.type_params = cd->u.trait_decl.type_params;
                    combined.type_param_count = cd->u.trait_decl.type_param_count;
                  }
                  return check_call_with_head(ctx, cidx, ctx->img->units[cidx].name,
                                              &combined, e, expected);
                }
              }
            }
          }
        }

        /* Top-level def / lemma / extern */
        int32_t tgt = stigi_image_resolve_name(ctx->img, ctx->uidx, cn.ptr, cn.len);
        if (tgt >= 0) {
          const StigiDecl *d = ctx->img->units[tgt].ast->decl;
          const StigiDefHead *h = NULL;
          if (d->kind == STIGI_DECL_DEF || d->kind == STIGI_DECL_LEMMA_DEF) {
            h = &d->u.def_decl.head;
          } else if (d->kind == STIGI_DECL_EXTERN && !d->u.extern_decl.is_val) {
            h = &d->u.extern_decl.head;
          }
          if (h != NULL) {
            return check_call_with_head(ctx, (uint32_t)tgt, ctx->img->units[tgt].name,
                                        h, e, expected);
          }
        }

        /* Enum constructor call (qualified EnumName.CaseName or unqualified CaseName) */
        uint32_t cidx = 0;
        bool amb = false;
        int32_t eidx = stigi_image_resolve_enum_case(ctx->img, ctx->uidx, cn.ptr, cn.len,
                                                     &cidx, &amb);
        if (eidx >= 0) {
          return check_enum_ctor(ctx, (uint32_t)eidx, cidx, e, e->span, expected);
        }
      }
      for (uint32_t i = 0; i < e->u.call.arg_count; i++) {
        (void)check_expr(ctx, e->u.call.args[i], NULL);
      }
      return alloc_ty(ctx->arena, STIGI_TY_UNKNOWN);
    }
    case STIGI_EXPR_MATCH: {
      StigiTy *sc = check_expr(ctx, e->u.match_expr.scrutinee, NULL);
      const StigiTy *usc = unwrap_ty_underlying(sc);
      bool has_wild = false;
      bool saw_true = false, saw_false = false;
      const StigiDecl *enum_d = NULL;
      uint32_t enum_uidx = 0;
      uint32_t case_count = 0;
      bool *covered = NULL;
      if (usc != NULL && usc->kind == STIGI_TY_ENUM && usc->unit_idx < ctx->img->unit_count) {
        enum_uidx = usc->unit_idx;
        enum_d = ctx->img->units[enum_uidx].ast->decl;
        if (enum_d->kind == STIGI_DECL_ENUM) {
          case_count = enum_d->u.enum_decl.case_count;
          if (case_count > 0) covered = calloc(case_count, sizeof *covered);
        } else {
          enum_d = NULL;
        }
      }

      bool is_alloc_res = (usc != NULL &&
                           (usc->kind == STIGI_TY_FILL_RESULT ||
                            usc->kind == STIGI_TY_GROW_RESULT ||
                            usc->kind == STIGI_TY_MAKE_RESULT));
      bool alloc_cov[2] = { false, false };

      StigiTy *res_ty = NULL;
      for (uint32_t i = 0; i < e->u.match_expr.arm_count; i++) {
        const StigiPat *pat = &e->u.match_expr.arms[i].pat;
        uint32_t saved = ctx->local_count;
        if (pat->kind == STIGI_PAT_WILD) {
          has_wild = true;
          if (enum_d != NULL || is_alloc_res) {
            (void)add_tdiag(ctx->diags, STIGI_TYPE_WILDCARD_ARM, pat->span, ctx->diag,
                            "wildcard '_' arm is not allowed in enum match");
          }
        } else if (pat->kind == STIGI_PAT_BOOL) {
          if (usc != NULL && usc->kind != STIGI_TY_BOOL && usc->kind != STIGI_TY_ERROR) {
            (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, pat->span, ctx->diag,
                            "boolean pattern requires Bool scrutinee");
          }
          if (pat->bool_val) saw_true = true;
          else saw_false = true;
        } else if (pat->kind == STIGI_PAT_INT) {
          if (pat->int_overflow) {
            (void)add_tdiag(ctx->diags, STIGI_TYPE_LITERAL_OUT_OF_RANGE, pat->span, ctx->diag,
                            "integer pattern literal out of range");
          } else if (usc != NULL && is_int_ty(usc)) {
            check_int_lit_range(ctx, pat->int_val, false, usc, pat->span);
          }
        } else if (pat->kind == STIGI_PAT_VAR) {
          push_ty_local(ctx, pat->name, sc, false);
          has_wild = true;
        } else if (pat->kind == STIGI_PAT_CTOR) {
          for (uint32_t f = 0; f < pat->field_count; f++) {
            if (pat->fields[f].len > 0 && pat->fields[f].ptr[0] >= 'A' &&
                pat->fields[f].ptr[0] <= 'Z') {
              (void)add_tdiag(ctx->diags, STIGI_TYPE_NESTED_PATTERN, pat->span, ctx->diag,
                              "nested pattern '%.*s' is not allowed",
                              (int)pat->fields[f].len, pat->fields[f].ptr);
            }
          }
          if (enum_d != NULL) {
            /* Match both short case name `Appended` and qualified `AppendResult.Appended` */
            uint32_t found_c = UINT32_MAX;
            for (uint32_t c = 0; c < case_count; c++) {
              const StigiEnumCase *ec = &enum_d->u.enum_decl.cases[c];
              if (str_eq_ss(ec->name, pat->name)) {
                found_c = c;
                break;
              }
            }
            if (found_c == UINT32_MAX) {
              uint32_t rc = 0;
              bool amb = false;
              int32_t re = stigi_image_resolve_enum_case(ctx->img, ctx->uidx, pat->name.ptr,
                                                         pat->name.len, &rc, &amb);
              if (re == (int32_t)enum_uidx) found_c = rc;
            }
            if (found_c == UINT32_MAX) {
              (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, pat->span, ctx->diag,
                              "case '%.*s' does not belong to enum '%s'",
                              (int)pat->name.len, pat->name.ptr, ctx->img->units[enum_uidx].name);
            } else {
              const StigiEnumCase *ec = &enum_d->u.enum_decl.cases[found_c];
              if (covered != NULL) {
                covered[found_c] = true;
              }
              if (pat->field_count != ec->field_count) {
                (void)add_tdiag(ctx->diags, STIGI_TYPE_PATTERN_ARITY, pat->span, ctx->diag,
                                "case '%.*s' expects %u pattern fields, got %u",
                                (int)ec->name.len, ec->name.ptr, ec->field_count, pat->field_count);
              }
              TyCtx fctx;
              memset(&fctx, 0, sizeof fctx);
              fctx.img = ctx->img;
              fctx.uidx = enum_uidx;
              fctx.arena = ctx->arena;
              fctx.lin_intern = ctx->lin_intern;
              for (uint32_t tp = 0; tp < enum_d->u.enum_decl.type_param_count; tp++) {
                StigiTy *sv = tp < usc->type_arg_count ? usc->type_args[tp] : NULL;
                (void)push_ty_type_param(&fctx, enum_d->u.enum_decl.type_params[tp].name, sv);
              }
              /* Pre-bind pattern binders so dependent field types resolve in any order */
              uint32_t base_local = ctx->local_count;
              uint32_t nf = pat->field_count < ec->field_count ? pat->field_count : ec->field_count;
              for (uint32_t f = 0; f < nf; f++) {
                push_ty_local(ctx, pat->fields[f], NULL, false);
                uint32_t bid = (ctx->local_count > base_local)
                                   ? ctx->locals[ctx->local_count - 1].binder_id : 0U;
                push_ty_local_lin(&fctx, ec->fields[f].name, NULL, false, bid != 0,
                                  stigi_linear_var(bid));
              }
              for (uint32_t f = 0; f < nf; f++) {
                StigiTy *fty = resolve_ast_type_ctx(ctx->img, enum_uidx, ec->fields[f].type,
                                                    ctx->arena, ctx->lin_intern, &fctx,
                                                    ctx->diags, ctx->diag, 0);
                if (pat->fields[f].len > 0 && !(pat->fields[f].len == 1 && pat->fields[f].ptr[0] == '_')) {
                  for (uint32_t li = base_local; li < ctx->local_count; li++) {
                    if (str_eq_ss(ctx->locals[li].name, pat->fields[f])) {
                      ctx->locals[li].ty = fty;
                      break;
                    }
                  }
                }
              }
              ty_ctx_free(&fctx);
            }
          } else if (is_alloc_res) {
            if (usc->kind == STIGI_TY_FILL_RESULT) {
              if (str_eq_c(pat->name, "Filled")) {
                if (alloc_cov[0]) {
                  (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, pat->span, ctx->diag,
                                  "duplicate match arm for case 'Filled'");
                }
                alloc_cov[0] = true;
                if (pat->field_count != 1) {
                  (void)add_tdiag(ctx->diags, STIGI_TYPE_PATTERN_ARITY, pat->span, ctx->diag,
                                  "case 'Filled' expects 1 field, got %u", pat->field_count);
                } else {
                  StigiTy *arr = alloc_ty(ctx->arena, STIGI_TY_ARRAY);
                  arr->elem = usc->elem != NULL ? usc->elem : alloc_ty(ctx->arena, STIGI_TY_ERROR);
                  arr->len = usc->len;
                  arr->has_len = usc->has_len;
                  push_ty_local(ctx, pat->fields[0], arr, false);
                }
              } else if (str_eq_c(pat->name, "FillFailed")) {
                if (alloc_cov[1]) {
                  (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, pat->span, ctx->diag,
                                  "duplicate match arm for case 'FillFailed'");
                }
                alloc_cov[1] = true;
                if (pat->field_count != 0) {
                  (void)add_tdiag(ctx->diags, STIGI_TYPE_PATTERN_ARITY, pat->span, ctx->diag,
                                  "case 'FillFailed' expects 0 fields, got %u", pat->field_count);
                }
              } else {
                (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, pat->span, ctx->diag,
                                "case '%.*s' does not belong to Array.tryFill result",
                                (int)pat->name.len, pat->name.ptr);
              }
            } else if (usc->kind == STIGI_TY_GROW_RESULT) {
              if (str_eq_c(pat->name, "Grown")) {
                if (alloc_cov[0]) {
                  (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, pat->span, ctx->diag,
                                  "duplicate match arm for case 'Grown'");
                }
                alloc_cov[0] = true;
                if (pat->field_count != 1) {
                  (void)add_tdiag(ctx->diags, STIGI_TYPE_PATTERN_ARITY, pat->span, ctx->diag,
                                  "case 'Grown' expects 1 field, got %u", pat->field_count);
                } else {
                  StigiTy *arr = alloc_ty(ctx->arena, STIGI_TY_ARRAY);
                  arr->elem = usc->elem != NULL ? usc->elem : alloc_ty(ctx->arena, STIGI_TY_ERROR);
                  arr->len = usc->len;
                  arr->has_len = usc->has_len;
                  push_ty_local(ctx, pat->fields[0], arr, false);
                }
              } else if (str_eq_c(pat->name, "GrowFailed")) {
                if (alloc_cov[1]) {
                  (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, pat->span, ctx->diag,
                                  "duplicate match arm for case 'GrowFailed'");
                }
                alloc_cov[1] = true;
                if (pat->field_count != 1) {
                  (void)add_tdiag(ctx->diags, STIGI_TYPE_PATTERN_ARITY, pat->span, ctx->diag,
                                  "case 'GrowFailed' expects 1 field, got %u", pat->field_count);
                } else {
                  StigiTy *arr = alloc_ty(ctx->arena, STIGI_TY_ARRAY);
                  arr->elem = usc->elem != NULL ? usc->elem : alloc_ty(ctx->arena, STIGI_TY_ERROR);
                  arr->len = usc->old_len;
                  arr->has_len = usc->has_old_len;
                  push_ty_local(ctx, pat->fields[0], arr, false);
                }
              } else {
                (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, pat->span, ctx->diag,
                                "case '%.*s' does not belong to Array.tryGrow result",
                                (int)pat->name.len, pat->name.ptr);
              }
            } else if (usc->kind == STIGI_TY_MAKE_RESULT) {
              if (str_eq_c(pat->name, "Filled")) {
                if (alloc_cov[0]) {
                  (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, pat->span, ctx->diag,
                                  "duplicate match arm for case 'Filled'");
                }
                alloc_cov[0] = true;
                if (pat->field_count != 1) {
                  (void)add_tdiag(ctx->diags, STIGI_TYPE_PATTERN_ARITY, pat->span, ctx->diag,
                                  "case 'Filled' expects 1 field, got %u", pat->field_count);
                } else {
                  StigiTy *cell = alloc_ty(ctx->arena, STIGI_TY_CELL);
                  cell->elem = usc->elem != NULL ? usc->elem : alloc_ty(ctx->arena, STIGI_TY_ERROR);
                  push_ty_local(ctx, pat->fields[0], cell, false);
                }
              } else if (str_eq_c(pat->name, "FillFailed")) {
                if (alloc_cov[1]) {
                  (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, pat->span, ctx->diag,
                                  "duplicate match arm for case 'FillFailed'");
                }
                alloc_cov[1] = true;
                if (pat->field_count != 0) {
                  (void)add_tdiag(ctx->diags, STIGI_TYPE_PATTERN_ARITY, pat->span, ctx->diag,
                                  "case 'FillFailed' expects 0 fields, got %u", pat->field_count);
                }
              } else {
                (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, pat->span, ctx->diag,
                                "case '%.*s' does not belong to Cell.tryMake result",
                                (int)pat->name.len, pat->name.ptr);
              }
            }
          }
        }
        StigiTy *at = check_expr(ctx, e->u.match_expr.arms[i].body, expected);
        ctx->local_count = saved;
        if (res_ty == NULL || res_ty->kind == STIGI_TY_NEVER) res_ty = at;
        else if (at->kind != STIGI_TY_NEVER &&
                 !stigi_ty_assignable(res_ty, at) && !stigi_ty_assignable(at, res_ty)) {
          (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, e->u.match_expr.arms[i].span,
                          ctx->diag, "match arm type mismatch");
        }
      }
      if (is_alloc_res) {
        if (!alloc_cov[0] || !alloc_cov[1]) {
          (void)add_tdiag(ctx->diags, STIGI_TYPE_NOT_EXHAUSTIVE, e->span, ctx->diag,
                          "allocation result match must cover both success and failure cases");
        }
      } else if (enum_d != NULL && covered != NULL && !has_wild) {
        for (uint32_t c = 0; c < case_count; c++) {
          if (!covered[c]) {
            (void)add_tdiag(ctx->diags, STIGI_TYPE_NOT_EXHAUSTIVE, e->span, ctx->diag,
                            "match on '%s' is not exhaustive (missing case '%.*s')",
                            ctx->img->units[enum_uidx].name,
                            (int)enum_d->u.enum_decl.cases[c].name.len,
                            enum_d->u.enum_decl.cases[c].name.ptr);
          }
        }
      } else if (usc != NULL && usc->kind == STIGI_TY_BOOL && !has_wild &&
                 !(saw_true && saw_false)) {
        (void)add_tdiag(ctx->diags, STIGI_TYPE_NOT_EXHAUSTIVE, e->span, ctx->diag,
                        "Bool match must cover both 'true' and 'false'");
      } else if (usc != NULL && is_int_ty(usc) && !has_wild) {
        (void)add_tdiag(ctx->diags, STIGI_TYPE_NOT_EXHAUSTIVE, e->span, ctx->diag,
                        "integer match must end with 'case _'");
      }
      free(covered);
      return res_ty != NULL ? res_ty : alloc_ty(ctx->arena, STIGI_TY_UNKNOWN);
    }
    case STIGI_EXPR_BLOCK: {
      uint32_t saved = ctx->local_count;
      StigiTy *last = alloc_ty(ctx->arena, STIGI_TY_UNIT);
      for (uint32_t i = 0; i < e->u.block.count; i++) {
        const StigiTy *exp = (i + 1 == e->u.block.count) ? expected : NULL;
        last = check_expr(ctx, e->u.block.stmts[i], exp);
      }
      ctx->local_count = saved;
      return last;
    }
    case STIGI_EXPR_ASSERT:
      return e->u.assert_expr.expr != NULL
                 ? check_expr(ctx, e->u.assert_expr.expr, expected)
                 : alloc_ty(ctx->arena, STIGI_TY_UNIT);
    case STIGI_EXPR_HANDLE: {
      int32_t hidx = stigi_image_resolve_name(ctx->img, ctx->uidx,
                                              e->u.handle_stmt.handler_name.ptr,
                                              e->u.handle_stmt.handler_name.len);
      int32_t cap_u = -1;
      if (hidx >= 0 && ctx->img->units[hidx].ast->decl->kind == STIGI_DECL_HANDLER) {
        const StigiDecl *hd = ctx->img->units[hidx].ast->decl;
        cap_u = stigi_image_resolve_name(ctx->img, (uint32_t)hidx,
                                         hd->u.handler_decl.for_cap.ptr,
                                         hd->u.handler_decl.for_cap.len);
        if (e->u.handle_stmt.arg_count != hd->u.handler_decl.val_param_count) {
          (void)add_tdiag(ctx->diags, STIGI_TYPE_ARITY, e->span, ctx->diag,
                          "handler '%s' expects %u arguments, got %u",
                          ctx->img->units[hidx].name, hd->u.handler_decl.val_param_count,
                          e->u.handle_stmt.arg_count);
        } else {
          for (uint32_t i = 0; i < e->u.handle_stmt.arg_count; i++) {
            StigiTy *pty = resolve_ast_type(ctx->img, (uint32_t)hidx,
                                            hd->u.handler_decl.val_params[i].type,
                                            ctx->arena, ctx->lin_intern, ctx->diags, ctx->diag, 0);
            StigiTy *aty = check_expr(ctx, e->u.handle_stmt.args[i], pty);
            if (!stigi_ty_assignable(pty, aty)) {
              (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, e->u.handle_stmt.args[i]->span,
                              ctx->diag, "handler argument %u type mismatch", i + 1);
            }
          }
        }
      } else {
        for (uint32_t i = 0; i < e->u.handle_stmt.arg_count; i++) {
          (void)check_expr(ctx, e->u.handle_stmt.args[i], NULL);
        }
      }
      if (e->u.handle_stmt.body != NULL) {
        uint32_t saved = ctx->local_count;
        if (cap_u >= 0 || e->u.handle_stmt.binder.len > 0) {
          push_ty_cap_binder(ctx, e->u.handle_stmt.binder, cap_u);
        }
        StigiTy *bt = check_expr(ctx, e->u.handle_stmt.body, expected);
        ctx->local_count = saved;
        return bt;
      }
      if (cap_u >= 0 || e->u.handle_stmt.binder.len > 0) {
        push_ty_cap_binder(ctx, e->u.handle_stmt.binder, cap_u);
      }
      return alloc_ty(ctx->arena, STIGI_TY_UNIT);
    }
    default:
      return alloc_ty(ctx->arena, STIGI_TY_UNIT);
  }
}

/* ---- Check def/lemma/extern-body head & body (supports out-of-order dependent params!) ---- */

static void check_def_like(const StigiImage *img, uint32_t uidx, const StigiDefHead *h,
                           const StigiExpr *body, bool is_lemma, bool in_extern_dsl,
                           Arena *arena, InternPool *lin_intern,
                           StigiTypeDiags *diags, Diag *diag) {
  TyCtx ctx;
  memset(&ctx, 0, sizeof ctx);
  ctx.img = img;
  ctx.uidx = uidx;
  ctx.arena = arena;
  ctx.lin_intern = lin_intern;
  ctx.diags = diags;
  ctx.diag = diag;
  ctx.in_extern_dsl = in_extern_dsl;

  for (uint32_t tp = 0; tp < h->type_param_count; tp++) {
    (void)push_ty_type_param(&ctx, h->type_params[tp].name, NULL);
  }
  for (uint32_t g = 0; g < h->pre_group_count; g++) {
    if (!h->pre_groups[g].is_fact) {
      for (uint32_t c = 0; c < h->pre_groups[g].cap_count; c++) {
        if (h->pre_groups[g].caps[c].cap_type != NULL) {
          int32_t cidx = stigi_image_resolve_name(img, uidx,
                                                  h->pre_groups[g].caps[c].cap_type->name.ptr,
                                                  h->pre_groups[g].caps[c].cap_type->name.len);
          push_ty_cap_binder(&ctx, h->pre_groups[g].caps[c].name, cidx);
        }
      }
    }
  }
  /* Pre-push ALL value param names first so out-of-order dependent lengths resolve (types.md 3) */
  uint32_t pbase = ctx.local_count;
  for (uint32_t p = 0; p < h->param_count; p++) {
    push_ty_local(&ctx, h->params[p].name, NULL, false);
  }
  for (uint32_t p = 0; p < h->param_count; p++) {
    StigiTy *pty = resolve_ast_type_ctx(img, uidx, h->params[p].type, arena, lin_intern,
                                        &ctx, diags, diag, 0);
    if (pbase + p < ctx.local_count) ctx.locals[pbase + p].ty = pty;
  }

  StigiTy *ret_ty = h->ret_type != NULL
                        ? resolve_ast_type_ctx(img, uidx, h->ret_type, arena, lin_intern,
                                               &ctx, diags, diag, 0)
                        : alloc_ty(arena, STIGI_TY_UNIT);
  if (h->decreases != NULL) {
    StigiTy *u64 = alloc_ty(arena, STIGI_TY_U64);
    StigiTy *dt = check_expr(&ctx, h->decreases, u64);
    if (!stigi_ty_assignable(u64, dt)) {
      (void)add_tdiag(diags, STIGI_TYPE_MISMATCH, h->decreases->span, diag,
                      "'decreases' measure must have type U64");
    }
  }
  if (body != NULL) {
    StigiTy *body_ty = check_expr(&ctx, body, is_lemma ? NULL : ret_ty);
    if (!is_lemma && !stigi_ty_assignable(ret_ty, body_ty)) {
      (void)add_tdiag(diags, STIGI_TYPE_MISMATCH, img->units[uidx].ast->decl->span, diag,
                      "body of '%s' does not match declared return type", img->units[uidx].name);
    }
  }
  ty_ctx_free(&ctx);
}

/* ---- Check handler against capability signature (capabilities.md 2 & obligations.md) ---- */

static void check_handler_decl(const StigiImage *img, uint32_t uidx, const StigiDecl *d,
                               Arena *arena, InternPool *lin_intern,
                               StigiTypeDiags *diags, Diag *diag) {
  TyCtx hctx;
  memset(&hctx, 0, sizeof hctx);
  hctx.img = img;
  hctx.uidx = uidx;
  hctx.arena = arena;
  hctx.lin_intern = lin_intern;
  hctx.diags = diags;
  hctx.diag = diag;

  for (uint32_t tp = 0; tp < d->u.handler_decl.type_param_count; tp++) {
    (void)push_ty_type_param(&hctx, d->u.handler_decl.type_params[tp].name, NULL);
  }
  for (uint32_t c = 0; c < d->u.handler_decl.cap_param_count; c++) {
    if (d->u.handler_decl.cap_params[c].name.len > 0 &&
        d->u.handler_decl.cap_params[c].cap_type != NULL) {
      int32_t cidx = stigi_image_resolve_name(img, uidx,
                                              d->u.handler_decl.cap_params[c].cap_type->name.ptr,
                                              d->u.handler_decl.cap_params[c].cap_type->name.len);
      push_ty_cap_binder(&hctx, d->u.handler_decl.cap_params[c].name, cidx);
    }
  }
  uint32_t vp_base = hctx.local_count;
  for (uint32_t p = 0; p < d->u.handler_decl.val_param_count; p++) {
    push_ty_local(&hctx, d->u.handler_decl.val_params[p].name, NULL, false);
  }
  for (uint32_t p = 0; p < d->u.handler_decl.val_param_count; p++) {
    StigiTy *pty = resolve_ast_type_ctx(img, uidx, d->u.handler_decl.val_params[p].type,
                                        arena, lin_intern, &hctx, diags, diag, 0);
    if (vp_base + p < hctx.local_count) hctx.locals[vp_base + p].ty = pty;
  }
  for (uint32_t v = 0; v < d->u.handler_decl.var_count; v++) {
    (void)check_expr(&hctx, d->u.handler_decl.vars[v], NULL);
  }

  /* Match every handler op against the target capability */
  int32_t cap_idx = stigi_image_resolve_name(img, uidx, d->u.handler_decl.for_cap.ptr,
                                             d->u.handler_decl.for_cap.len);
  const StigiDecl *cap_d = (cap_idx >= 0 && img->units[cap_idx].ast->decl->kind == STIGI_DECL_CAPABILITY)
                               ? img->units[cap_idx].ast->decl : NULL;

  for (uint32_t i = 0; i < d->u.handler_decl.def_count; i++) {
    const StigiDecl *op = d->u.handler_decl.defs[i];
    const StigiDefHead *oh = &op->u.def_decl.head;
    uint32_t saved = hctx.local_count;
    for (uint32_t tp = 0; tp < oh->type_param_count; tp++) {
      (void)push_ty_type_param(&hctx, oh->type_params[tp].name, NULL);
    }
    uint32_t obase = hctx.local_count;
    for (uint32_t p = 0; p < oh->param_count; p++) {
      push_ty_local(&hctx, oh->params[p].name, NULL, false);
    }
    for (uint32_t p = 0; p < oh->param_count; p++) {
      StigiTy *pty = resolve_ast_type_ctx(img, uidx, oh->params[p].type, arena, lin_intern,
                                          &hctx, diags, diag, 0);
      if (obase + p < hctx.local_count) hctx.locals[obase + p].ty = pty;
    }
    StigiTy *oret = oh->ret_type != NULL
                        ? resolve_ast_type_ctx(img, uidx, oh->ret_type, arena, lin_intern,
                                               &hctx, diags, diag, 0)
                        : alloc_ty(arena, STIGI_TY_UNIT);
    StigiTy *obody = check_expr(&hctx, stigi_def_body(op), oret);
    if (!stigi_ty_assignable(oret, obody)) {
      (void)add_tdiag(diags, STIGI_TYPE_MISMATCH, op->span, diag,
                      "handler operation '%.*s' body does not match return type",
                      (int)op->name.len, op->name.ptr);
    }

    if (cap_d != NULL) {
      const StigiDecl *cap_op = NULL;
      for (uint32_t m = 0; m < cap_d->u.cap_decl.member_count; m++) {
        const StigiDecl *cm = cap_d->u.cap_decl.members[m];
        if (cm->kind == STIGI_DECL_DEF && str_eq_ss(cm->name, op->name)) {
          cap_op = cm;
          break;
        }
      }
      if (cap_op == NULL) {
        (void)add_tdiag(diags, STIGI_TYPE_HANDLER_SIGNATURE, op->span, diag,
                        "handler '%s' defines unknown operation '%.*s' for capability '%s'",
                        img->units[uidx].name, (int)op->name.len, op->name.ptr,
                        img->units[cap_idx].name);
      } else {
        const StigiDefHead *ch = &cap_op->u.def_decl.head;
        bool sig_ok = (oh->param_count == ch->param_count &&
                       oh->type_param_count == ch->type_param_count);
        if (sig_ok) {
          TyCtx cctx;
          memset(&cctx, 0, sizeof cctx);
          cctx.img = img;
          cctx.uidx = (uint32_t)cap_idx;
          cctx.arena = arena;
          cctx.lin_intern = lin_intern;
          for (uint32_t tp = 0; tp < ch->type_param_count; tp++) {
            StigiTy *sv = alloc_ty(arena, STIGI_TY_VAR);
            if (sv != NULL && saved + tp < hctx.local_count) {
              sv->var_id = hctx.locals[saved + tp].binder_id;
            }
            (void)push_ty_type_param(&cctx, ch->type_params[tp].name, sv);
          }
          for (uint32_t p = 0; p < ch->param_count; p++) {
            uint32_t hbid = (obase + p < hctx.local_count) ? hctx.locals[obase + p].binder_id : 0U;
            push_ty_local_lin(&cctx, ch->params[p].name, NULL, false, hbid != 0,
                              stigi_linear_var(hbid));
          }
          for (uint32_t p = 0; p < ch->param_count; p++) {
            StigiTy *ct = resolve_ast_type_ctx(img, (uint32_t)cap_idx, ch->params[p].type,
                                               arena, lin_intern, &cctx, NULL, NULL, 0);
            StigiTy *ht = (obase + p < hctx.local_count) ? hctx.locals[obase + p].ty : NULL;
            if (!stigi_ty_equal(ct, ht)) sig_ok = false;
          }
          StigiTy *cret = ch->ret_type != NULL
                              ? resolve_ast_type_ctx(img, (uint32_t)cap_idx, ch->ret_type,
                                                     arena, lin_intern, &cctx, NULL, NULL, 0)
                              : alloc_ty(arena, STIGI_TY_UNIT);
          if (!stigi_ty_equal(cret, oret)) sig_ok = false;
          ty_ctx_free(&cctx);
        }
        if (!sig_ok) {
          (void)add_tdiag(diags, STIGI_TYPE_HANDLER_SIGNATURE, op->span, diag,
                          "handler operation '%.*s' signature does not match capability '%s'",
                          (int)op->name.len, op->name.ptr, img->units[cap_idx].name);
        }
      }
    }
    hctx.local_count = saved;
  }

  /* Verify every capability operation is implemented by the handler */
  if (cap_d != NULL) {
    for (uint32_t m = 0; m < cap_d->u.cap_decl.member_count; m++) {
      const StigiDecl *cm = cap_d->u.cap_decl.members[m];
      if (cm->kind != STIGI_DECL_DEF) continue;
      bool found = false;
      for (uint32_t i = 0; i < d->u.handler_decl.def_count; i++) {
        if (str_eq_ss(d->u.handler_decl.defs[i]->name, cm->name)) {
          found = true;
          break;
        }
      }
      if (!found) {
        (void)add_tdiag(diags, STIGI_TYPE_HANDLER_SIGNATURE, d->span, diag,
                        "handler '%s' is missing capability operation '%.*s'",
                        img->units[uidx].name, (int)cm->name.len, cm->name.ptr);
      }
    }
  }
  ty_ctx_free(&hctx);
}

static void check_given_decl(const StigiImage *img, uint32_t uidx, const StigiDecl *d,
                             Arena *arena, InternPool *lin_intern,
                             StigiTypeDiags *diags, Diag *diag) {
  int32_t tr_idx = stigi_image_resolve_name(img, uidx, d->u.given_decl.trait_name.ptr,
                                            d->u.given_decl.trait_name.len);
  const StigiDecl *tr_d = NULL;
  if (tr_idx >= 0 && img->units[tr_idx].ast->decl->kind == STIGI_DECL_TRAIT) {
    tr_d = img->units[tr_idx].ast->decl;
  } else {
    (void)add_tdiag(diags, STIGI_TYPE_HANDLER_SIGNATURE, d->span, diag,
                    "given '%s' target '%.*s' is not a trait",
                    img->units[uidx].name, (int)d->u.given_decl.trait_name.len,
                    d->u.given_decl.trait_name.ptr);
  }

  TyCtx gctx;
  memset(&gctx, 0, sizeof gctx);
  gctx.img = img;
  gctx.uidx = uidx;
  gctx.arena = arena;
  gctx.lin_intern = lin_intern;
  gctx.diags = diags;
  gctx.diag = diag;

  for (uint32_t tp = 0; tp < d->u.given_decl.type_param_count; tp++) {
    (void)push_ty_type_param(&gctx, d->u.given_decl.type_params[tp].name, NULL);
  }
  for (uint32_t cp = 0; cp < d->u.given_decl.cap_param_count; cp++) {
    if (d->u.given_decl.cap_params[cp].cap_type != NULL) {
      int32_t cidx = stigi_image_resolve_name(img, uidx,
                                              d->u.given_decl.cap_params[cp].cap_type->name.ptr,
                                              d->u.given_decl.cap_params[cp].cap_type->name.len);
      push_ty_cap_binder(&gctx, d->u.given_decl.cap_params[cp].name, cidx);
    }
  }

  /* Resolve trait_type args in given's scope for member signature verification */
  StigiTy **tr_targs = NULL;
  uint32_t tr_targ_cnt = 0;
  if (d->u.given_decl.trait_type != NULL && d->u.given_decl.trait_type->type_arg_count > 0) {
    tr_targ_cnt = d->u.given_decl.trait_type->type_arg_count;
    tr_targs = arena_alloc(arena, (size_t)tr_targ_cnt * sizeof *tr_targs);
    for (uint32_t k = 0; k < tr_targ_cnt && tr_targs != NULL; k++) {
      tr_targs[k] = resolve_ast_type_ctx(img, uidx, d->u.given_decl.trait_type->type_args[k],
                                         arena, lin_intern, &gctx, diags, diag, 0);
    }
  }

  for (uint32_t i = 0; i < d->u.given_decl.def_count; i++) {
    const StigiDecl *op = d->u.given_decl.defs[i];
    if (op->kind != STIGI_DECL_DEF) continue;
    uint32_t saved = gctx.local_count;
    const StigiDefHead *oh = &op->u.def_decl.head;
    for (uint32_t tp = 0; tp < oh->type_param_count; tp++) {
      (void)push_ty_type_param(&gctx, oh->type_params[tp].name, NULL);
    }
    uint32_t obase = gctx.local_count;
    for (uint32_t p = 0; p < oh->param_count; p++) {
      push_ty_local(&gctx, oh->params[p].name, NULL, false);
    }
    for (uint32_t p = 0; p < oh->param_count; p++) {
      StigiTy *pt = resolve_ast_type_ctx(img, uidx, oh->params[p].type, arena, lin_intern,
                                         &gctx, diags, diag, 0);
      if (obase + p < gctx.local_count) gctx.locals[obase + p].ty = pt;
    }
    StigiTy *oret = oh->ret_type != NULL
                        ? resolve_ast_type_ctx(img, uidx, oh->ret_type, arena, lin_intern,
                                               &gctx, diags, diag, 0)
                        : alloc_ty(arena, STIGI_TY_UNIT);
    const StigiExpr *obody = stigi_def_body(op);
    if (obody != NULL) {
      StigiTy *bt = check_expr(&gctx, obody, oret);
      if (!stigi_ty_assignable(oret, bt)) {
        (void)add_tdiag(diags, STIGI_TYPE_MISMATCH, op->span, diag,
                        "given method '%.*s' body type does not match return type",
                        (int)op->name.len, op->name.ptr);
      }
    }
    if (tr_d != NULL) {
      const StigiDecl *tr_op = NULL;
      for (uint32_t m = 0; m < tr_d->u.trait_decl.member_count; m++) {
        const StigiDecl *tm = tr_d->u.trait_decl.members[m];
        if (tm->kind == STIGI_DECL_DEF && str_eq_ss(tm->name, op->name)) {
          tr_op = tm;
          break;
        }
      }
      if (tr_op == NULL) {
        (void)add_tdiag(diags, STIGI_TYPE_HANDLER_SIGNATURE, op->span, diag,
                        "given '%s' defines unknown method '%.*s' for trait '%s'",
                        img->units[uidx].name, (int)op->name.len, op->name.ptr,
                        img->units[tr_idx].name);
      } else {
        const StigiDefHead *th = &tr_op->u.def_decl.head;
        bool sig_ok = (oh->param_count == th->param_count);
        if (sig_ok) {
          TyCtx tctx;
          memset(&tctx, 0, sizeof tctx);
          tctx.img = img;
          tctx.uidx = (uint32_t)tr_idx;
          tctx.arena = arena;
          tctx.lin_intern = lin_intern;
          for (uint32_t tp = 0; tp < tr_d->u.trait_decl.type_param_count; tp++) {
            StigiTy *subst = (tr_targs != NULL && tp < tr_targ_cnt) ? tr_targs[tp] : NULL;
            (void)push_ty_type_param(&tctx, tr_d->u.trait_decl.type_params[tp].name, subst);
          }
          for (uint32_t p = 0; p < th->param_count; p++) {
            StigiTy *tt = resolve_ast_type_ctx(img, (uint32_t)tr_idx, th->params[p].type,
                                               arena, lin_intern, &tctx, NULL, NULL, 0);
            StigiTy *gt = (obase + p < gctx.local_count) ? gctx.locals[obase + p].ty : NULL;
            if (!stigi_ty_equal(tt, gt)) sig_ok = false;
          }
          StigiTy *tret = th->ret_type != NULL
                              ? resolve_ast_type_ctx(img, (uint32_t)tr_idx, th->ret_type,
                                                     arena, lin_intern, &tctx, NULL, NULL, 0)
                              : alloc_ty(arena, STIGI_TY_UNIT);
          if (!stigi_ty_equal(tret, oret)) sig_ok = false;
          ty_ctx_free(&tctx);
        }
        if (!sig_ok) {
          (void)add_tdiag(diags, STIGI_TYPE_HANDLER_SIGNATURE, op->span, diag,
                          "given method '%.*s' signature does not match trait '%s'",
                          (int)op->name.len, op->name.ptr, img->units[tr_idx].name);
        }
      }
    }
    gctx.local_count = saved;
  }

  if (tr_d != NULL) {
    for (uint32_t m = 0; m < tr_d->u.trait_decl.member_count; m++) {
      const StigiDecl *tm = tr_d->u.trait_decl.members[m];
      if (tm->kind != STIGI_DECL_DEF) continue;
      bool found = false;
      for (uint32_t i = 0; i < d->u.given_decl.def_count; i++) {
        if (str_eq_ss(d->u.given_decl.defs[i]->name, tm->name)) {
          found = true;
          break;
        }
      }
      if (!found) {
        (void)add_tdiag(diags, STIGI_TYPE_HANDLER_SIGNATURE, d->span, diag,
                        "given '%s' is missing trait method '%.*s'",
                        img->units[uidx].name, (int)tm->name.len, tm->name.ptr);
      }
    }
  }
  ty_ctx_free(&gctx);
}

Status stigi_typecheck_image(const StigiImage *img, StigiTypeDiags *diags, Diag *diag) {
  if (img == NULL) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_typecheck_image: invalid arguments");
  }

  /* 0. Elaborate S121-S123 patterns, multi-head defs, val-else, and pipes (Maranget) */
  Status est = stigi_elaborate_image((StigiImage *)img, diags, diag);
  if (est != STIGI_OK) return est;

  /* 1. Check recursive types (S43/S69) across all enum and type units */
  if (img->unit_count > 0) {
    uint8_t *color = calloc(img->unit_count, sizeof *color);
    if (color == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "out of memory");
    for (uint32_t i = 0; i < img->unit_count; i++) {
      const StigiDecl *d = img->units[i].ast->decl;
      if ((d->kind == STIGI_DECL_ENUM || d->kind == STIGI_DECL_TYPE) && color[i] == 0) {
        Status st = dfs_recursive_type(img, i, color, diags, diag);
        if (st != STIGI_OK) { free(color); return st; }
      }
    }
    free(color);
  }

  /* 2. Check extern-forges-facts (S110) and type check every unit */
  Arena arena;
  arena_init(&arena);
  InternPool lin_intern;
  intern_init(&lin_intern);

  for (uint32_t i = 0; i < img->unit_count; i++) {
    const StigiDecl *d = img->units[i].ast->decl;

    if (d->kind == STIGI_DECL_EXTERN && d->u.extern_decl.is_val) {
      if (type_carries_facts(img, i, d->u.extern_decl.val_type, 0)) {
        (void)add_tdiag(diags, STIGI_TYPE_EXTERN_FORGES_FACTS, d->span, diag,
                        "extern val '%s' has a type carrying facts", img->units[i].name);
      }
      (void)resolve_ast_type(img, i, d->u.extern_decl.val_type, &arena, &lin_intern,
                             diags, diag, 0);
      continue;
    }

    if (d->kind == STIGI_DECL_EXTERN && !d->u.extern_decl.is_val) {
      const StigiDefHead *h = &d->u.extern_decl.head;
      if (d->u.extern_decl.body == NULL) {
        if (type_carries_facts(img, i, h->ret_type, 0)) {
          (void)add_tdiag(diags, STIGI_TYPE_EXTERN_FORGES_FACTS, d->span, diag,
                          "body-less extern '%s' returns a type carrying facts", img->units[i].name);
        }
        for (uint32_t g = 0; g < h->pre_group_count; g++) {
          if (h->pre_groups[g].is_fact) {
            check_overwrites_group(img, i, h, &h->pre_groups[g].facts,
                                   &arena, &lin_intern, diags, diag);
          }
        }
        for (uint32_t g = 0; g < h->post_val_fact_count; g++) {
          check_overwrites_group(img, i, h, &h->post_val_facts[g],
                                 &arena, &lin_intern, diags, diag);
        }
        for (uint32_t g = 0; g < h->exit_fact_count; g++) {
          check_overwrites_group(img, i, h, &h->exit_facts[g],
                                 &arena, &lin_intern, diags, diag);
        }
      }
      check_def_like(img, i, h, d->u.extern_decl.body, false, d->u.extern_decl.body != NULL,
                     &arena, &lin_intern, diags, diag);
      continue;
    }

    if (d->kind == STIGI_DECL_DEF || d->kind == STIGI_DECL_LEMMA_DEF) {
      check_def_like(img, i, &d->u.def_decl.head, stigi_def_body(d),
                     d->kind == STIGI_DECL_LEMMA_DEF, false, &arena, &lin_intern, diags, diag);
    } else if (d->kind == STIGI_DECL_VAL || d->kind == STIGI_DECL_TEST) {
      TyCtx ctx;
      memset(&ctx, 0, sizeof ctx);
      ctx.img = img;
      ctx.uidx = i;
      ctx.arena = &arena;
      ctx.lin_intern = &lin_intern;
      ctx.diags = diags;
      ctx.diag = diag;
      StigiTy *decl_ty = resolve_ast_type(img, i, d->u.val_decl.type, &arena, &lin_intern,
                                          diags, diag, 0);
      StigiTy *body_ty = check_expr(&ctx, stigi_val_body(d), decl_ty);
      if (!stigi_ty_assignable(decl_ty, body_ty)) {
        (void)add_tdiag(diags, STIGI_TYPE_MISMATCH, d->span, diag,
                        "initializer of val '%s' does not match declared type",
                        img->units[i].name);
      }
      ty_ctx_free(&ctx);
    } else if (d->kind == STIGI_DECL_ENUM) {
      TyCtx ectx;
      memset(&ectx, 0, sizeof ectx);
      ectx.img = img;
      ectx.uidx = i;
      ectx.arena = &arena;
      ectx.lin_intern = &lin_intern;
      ectx.diags = diags;
      ectx.diag = diag;
      for (uint32_t tp = 0; tp < d->u.enum_decl.type_param_count; tp++) {
        (void)push_ty_type_param(&ectx, d->u.enum_decl.type_params[tp].name, NULL);
      }
      for (uint32_t c = 0; c < d->u.enum_decl.case_count; c++) {
        const StigiEnumCase *ec = &d->u.enum_decl.cases[c];
        uint32_t saved = ectx.local_count;
        for (uint32_t f = 0; f < ec->field_count; f++) {
          push_ty_local(&ectx, ec->fields[f].name, NULL, false);
        }
        for (uint32_t f = 0; f < ec->field_count; f++) {
          (void)resolve_ast_type_ctx(img, i, ec->fields[f].type, &arena, &lin_intern,
                                     &ectx, diags, diag, 0);
        }
        ectx.local_count = saved;
      }
      ty_ctx_free(&ectx);
    } else if (d->kind == STIGI_DECL_TYPE) {
      TyCtx tctx;
      memset(&tctx, 0, sizeof tctx);
      tctx.img = img;
      tctx.uidx = i;
      tctx.arena = &arena;
      tctx.lin_intern = &lin_intern;
      tctx.diags = diags;
      tctx.diag = diag;
      for (uint32_t tp = 0; tp < d->u.type_decl.type_param_count; tp++) {
        (void)push_ty_type_param(&tctx, d->u.type_decl.type_params[tp].name, NULL);
      }
      for (uint32_t vp = 0; vp < d->u.type_decl.val_param_count; vp++) {
        StigiTy *vt = resolve_ast_type_ctx(img, i, d->u.type_decl.val_params[vp].type,
                                           &arena, &lin_intern, &tctx, diags, diag, 0);
        push_ty_local(&tctx, d->u.type_decl.val_params[vp].name, vt, false);
      }
      (void)resolve_ast_type_ctx(img, i, d->u.type_decl.rhs, &arena, &lin_intern,
                                 &tctx, diags, diag, 0);
      ty_ctx_free(&tctx);
    } else if (d->kind == STIGI_DECL_HANDLER) {
      check_handler_decl(img, i, d, &arena, &lin_intern, diags, diag);
    } else if (d->kind == STIGI_DECL_GIVEN) {
      check_given_decl(img, i, d, &arena, &lin_intern, diags, diag);
    }
  }

  intern_free(&lin_intern);
  arena_free(&arena);
  return STIGI_OK;
}
