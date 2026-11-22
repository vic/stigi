#include "types.h"

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
    case STIGI_TYPE_ASSIGN_VAL: return "assign-val";
    case STIGI_TYPE_NOT_EXHAUSTIVE: return "not-exhaustive";
    case STIGI_TYPE_WILDCARD_ARM: return "wildcard-arm";
    case STIGI_TYPE_NESTED_PATTERN: return "nested-pattern";
    case STIGI_TYPE_RECURSIVE_TYPE: return "recursive-type";
    case STIGI_TYPE_EXTERN_FORGES_FACTS: return "extern-forges-facts";
    case STIGI_TYPE_LITERAL_OUT_OF_RANGE: return "literal-out-of-range";
    case STIGI_TYPE_LENGTH_NOT_LINEAR: return "length-not-linear";
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

/* ---- Checked int64_t arithmetic (Fix #7) ---- */

static bool i64_add(int64_t a, int64_t b, int64_t *out) {
  if ((b > 0 && a > INT64_MAX - b) || (b < 0 && a < INT64_MIN - b)) return false;
  *out = a + b;
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
      if (r.nterms >= 8) return STIGI_ERR_OVERFLOW;
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
    return STIGI_ERR_INVALID; /* nonlinear: product of two variables */
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
  if (out_delta != NULL) *out_delta = a->c - b->c;
  return true;
}

Status stigi_linear_from_expr(const StigiExpr *e, InternPool *intern, StigiLinear *out) {
  if (e == NULL || intern == NULL || out == NULL) return STIGI_ERR_INVALID;
  switch (e->kind) {
    case STIGI_EXPR_LIT_INT:
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
      return STIGI_ERR_INVALID; /* nonlinear / or % */
    }
    default:
      return STIGI_ERR_INVALID;
  }
}

/* ---- Type helpers & layout ---- */

static bool str_eq_c(StigiStr s, const char *lit) {
  size_t n = strlen(lit);
  return s.len == n && memcmp(s.ptr, lit, n) == 0;
}

static bool is_int_ty(const StigiTy *t) {
  if (t == NULL) return false;
  return t->kind == STIGI_TY_U8 || t->kind == STIGI_TY_U32 || t->kind == STIGI_TY_U64 ||
         t->kind == STIGI_TY_I32 || t->kind == STIGI_TY_I64;
}

bool stigi_ty_equal(const StigiTy *a, const StigiTy *b) {
  if (a == b) return true;
  if (a == NULL || b == NULL) return false;
  if (a->kind == STIGI_TY_UNKNOWN || b->kind == STIGI_TY_UNKNOWN) return true;
  if (a->kind == STIGI_TY_NEVER || b->kind == STIGI_TY_NEVER) return true;
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

static uint64_t align_up_u64(uint64_t v, uint64_t align) {
  if (align <= 1) return v;
  uint64_t rem = v % align;
  return rem == 0 ? v : v + (align - rem);
}

static StigiTy *resolve_ast_type(const StigiImage *img, uint32_t from_idx,
                                 const StigiType *ast_ty, Arena *arena,
                                 InternPool *lin_intern, StigiTypeDiags *diags,
                                 Diag *diag, int depth);

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
      return (StigiLayout){8, 8};
    case STIGI_TY_ARRAY:
      return (StigiLayout){8, 8}; /* plain A * pointer (emitter.md 2) */
    case STIGI_TY_UNIT:
    case STIGI_TY_NEVER:
    case STIGI_TY_TYPE:
    case STIGI_TY_VAR:
    case STIGI_TY_TEST_RESULT:
    case STIGI_TY_UNKNOWN:
      return (StigiLayout){0, 1};
    case STIGI_TY_TYPEDEF:
      return ty_layout_rec(img, ty->elem, depth + 1);
    case STIGI_TY_ENUM: {
      if (img == NULL || ty->unit_idx >= img->unit_count) return (StigiLayout){1, 1};
      const StigiDecl *d = img->units[ty->unit_idx].ast->decl;
      if (d->kind != STIGI_DECL_ENUM) return (StigiLayout){1, 1};
      uint32_t case_count = d->u.enum_decl.case_count;
      uint64_t tag_size = (case_count <= 256U) ? 1ULL : (case_count <= 65536U ? 2ULL : 4ULL);
      uint64_t tag_align = tag_size;
      uint64_t union_align = 1;
      uint64_t union_size = 0;
      bool has_any_fields = false;
      Arena tmp;
      arena_init(&tmp);
      InternPool lin;
      intern_init(&lin);
      for (uint32_t c = 0; c < case_count; c++) {
        const StigiEnumCase *ec = &d->u.enum_decl.cases[c];
        if (ec->field_count == 0) continue;
        has_any_fields = true;
        StigiLayout *flays = arena_alloc(&tmp, (size_t)ec->field_count * sizeof *flays);
        uint32_t *order = arena_alloc(&tmp, (size_t)ec->field_count * sizeof *order);
        if (flays == NULL || order == NULL) continue;
        for (uint32_t f = 0; f < ec->field_count; f++) {
          StigiTy *fty = resolve_ast_type(img, ty->unit_idx, ec->fields[f].type,
                                          &tmp, &lin, NULL, NULL, 0);
          flays[f] = ty_layout_rec(img, fty, depth + 1);
          order[f] = f;
        }
        /* Stable insertion sort by decreasing alignment, then declaration order (memory.md 8) */
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
        if (c_align > union_align) union_align = c_align;
        if (c_size > union_size) union_size = c_size;
      }
      intern_free(&lin);
      arena_free(&tmp);
      if (!has_any_fields) {
        return (StigiLayout){tag_size, tag_align};
      }
      union_size = align_up_u64(union_size, union_align);
      uint64_t tag_off = align_up_u64(union_size, tag_align);
      uint64_t total_align = union_align > tag_align ? union_align : tag_align;
      uint64_t total_size = align_up_u64(tag_off + tag_size, total_align);
      return (StigiLayout){total_size, total_align};
    }
  }
  return z;
}

StigiLayout stigi_ty_layout(const StigiImage *img, const StigiTy *ty) {
  return ty_layout_rec(img, ty, 0);
}

static void frame_walk_expr(const StigiImage *img, uint32_t uidx, const StigiExpr *e,
                            Arena *tmp, InternPool *lin, uint64_t *bytes) {
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
      frame_walk_expr(img, uidx, e->u.val_stmt.init, tmp, lin, bytes);
      break;
    case STIGI_EXPR_VAR:
      if (e->u.var_stmt.type != NULL) {
        StigiTy *ty = resolve_ast_type(img, uidx, e->u.var_stmt.type, tmp, lin, NULL, NULL, 0);
        StigiLayout l = stigi_ty_layout(img, ty);
        *bytes = align_up_u64(*bytes, l.align) + l.size;
      } else {
        *bytes = align_up_u64(*bytes, 8) + 8;
      }
      frame_walk_expr(img, uidx, e->u.var_stmt.init, tmp, lin, bytes);
      break;
    case STIGI_EXPR_BLOCK:
      for (uint32_t i = 0; i < e->u.block.count; i++) {
        frame_walk_expr(img, uidx, e->u.block.stmts[i], tmp, lin, bytes);
      }
      break;
    case STIGI_EXPR_IF:
      frame_walk_expr(img, uidx, e->u.if_expr.then_br, tmp, lin, bytes);
      frame_walk_expr(img, uidx, e->u.if_expr.else_br, tmp, lin, bytes);
      break;
    case STIGI_EXPR_WHILE:
      frame_walk_expr(img, uidx, e->u.while_expr.body, tmp, lin, bytes);
      break;
    case STIGI_EXPR_MATCH:
      for (uint32_t i = 0; i < e->u.match_expr.arm_count; i++) {
        frame_walk_expr(img, uidx, e->u.match_expr.arms[i].body, tmp, lin, bytes);
      }
      break;
    case STIGI_EXPR_HANDLE:
      frame_walk_expr(img, uidx, e->u.handle_stmt.body, tmp, lin, bytes);
      break;
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
    frame_walk_expr(img, unit_idx, d->u.val_decl.body, &tmp, &lin, &bytes);
    intern_free(&lin);
    arena_free(&tmp);
    *out_bytes = align_up_u64(bytes, 8);
    return STIGI_OK;
  }
  if (d->kind != STIGI_DECL_DEF && d->kind != STIGI_DECL_LEMMA_DEF) {
    *out_bytes = 0;
    return STIGI_OK;
  }
  Arena tmp;
  arena_init(&tmp);
  InternPool lin;
  intern_init(&lin);
  uint64_t bytes = 0;
  for (uint32_t i = 0; i < d->u.def_decl.head.param_count; i++) {
    StigiTy *ty = resolve_ast_type(img, unit_idx, d->u.def_decl.head.params[i].type,
                                   &tmp, &lin, NULL, NULL, 0);
    StigiLayout l = stigi_ty_layout(img, ty);
    bytes = align_up_u64(bytes, l.align) + l.size;
  }
  frame_walk_expr(img, unit_idx, d->u.def_decl.body, &tmp, &lin, &bytes);
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
} DepVec;

static void dep_vec_push(DepVec *v, uint32_t x) {
  if (v->count == v->cap) {
    uint32_t ncap = v->cap == 0 ? 16U : v->cap * 2U;
    uint32_t *ni = realloc(v->items, (size_t)ncap * sizeof *ni);
    if (ni == NULL) return;
    v->items = ni;
    v->cap = ncap;
  }
  v->items[v->count++] = x;
}

static void collect_type_deps(const StigiImage *img, uint32_t from_idx, const StigiType *ty,
                              DepVec *deps) {
  if (ty == NULL) return;
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

static bool dfs_recursive_type(const StigiImage *img, uint32_t u, uint8_t *color,
                               StigiTypeDiags *diags, Diag *diag) {
  color[u] = 1; /* visiting */
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

  bool found = false;
  for (uint32_t i = 0; i < deps.count; i++) {
    uint32_t v = deps.items[i];
    if (color[v] == 1) {
      (void)add_tdiag(diags, STIGI_TYPE_RECURSIVE_TYPE, d->span, diag,
                      "recursive type cycle involving '%s' and '%s'",
                      img->units[u].name, img->units[v].name);
      found = true;
      break;
    }
    if (color[v] == 0) {
      if (dfs_recursive_type(img, v, color, diags, diag)) {
        found = true;
        break;
      }
    }
  }
  free(deps.items);
  color[u] = 2; /* ALWAYS mark done on unwind (Fix #8) */
  return found;
}

/* ---- extern-forges-facts check (S110, Fix #9) ---- */

static bool type_carries_facts(const StigiImage *img, uint32_t from_idx,
                               const StigiType *ty, int depth) {
  if (ty == NULL || depth > 16) return false;
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

static const StigiType *unwrap_transparent_type(const StigiImage *img, uint32_t *from_idx,
                                                const StigiType *ty, int depth) {
  if (ty == NULL || depth > 16) return ty;
  if (ty->kind == STIGI_TYPE_ARRAY || ty->kind == STIGI_TYPE_CELL || ty->kind == STIGI_TYPE_PTR) {
    return ty;
  }
  int32_t tgt = stigi_image_resolve_name(img, *from_idx, ty->name.ptr, ty->name.len);
  if (tgt >= 0) {
    const StigiDecl *d = img->units[tgt].ast->decl;
    if (d->kind == STIGI_DECL_TYPE && !d->u.type_decl.has_facts) {
      *from_idx = (uint32_t)tgt;
      return unwrap_transparent_type(img, from_idx, d->u.type_decl.rhs, depth + 1);
    }
  }
  return ty;
}

static bool type_is_enum_or_carries_facts(const StigiImage *img, uint32_t from_idx,
                                          const StigiType *ty, int depth) {
  if (ty == NULL || depth > 16) return false;
  int32_t tgt = stigi_image_resolve_name(img, from_idx, ty->name.ptr, ty->name.len);
  if (tgt >= 0) {
    const StigiDecl *d = img->units[tgt].ast->decl;
    if (d->kind == STIGI_DECL_ENUM) return true;
    if (d->kind == STIGI_DECL_TYPE) {
      if (d->u.type_decl.has_facts) return true;
      return type_is_enum_or_carries_facts(img, (uint32_t)tgt, d->u.type_decl.rhs, depth + 1);
    }
  }
  return type_carries_facts(img, from_idx, ty, depth);
}

static void check_overwrites_group(const StigiImage *img, uint32_t uidx,
                                   const StigiDefHead *h, const StigiFactGroup *fg,
                                   StigiTypeDiags *diags, Diag *diag) {
  for (uint32_t i = 0; i < fg->count; i++) {
    const StigiFactItem *it = &fg->items[i];
    if (it->kind != STIGI_FACT_ATOM || it->arg_count < 1) continue;
    if (!str_eq_c(it->atom_name, "overwrites") &&
        !str_eq_c(it->atom_name, "Stigi.Facts.overwrites")) {
      continue;
    }
    const StigiExpr *arg0 = it->args[0];
    if (arg0->kind != STIGI_EXPR_LOCAL) continue;
    for (uint32_t p = 0; p < h->param_count; p++) {
      if (h->params[p].name.len == arg0->u.ident.name.len &&
          memcmp(h->params[p].name.ptr, arg0->u.ident.name.ptr, arg0->u.ident.name.len) == 0) {
        uint32_t owner_idx = uidx;
        const StigiType *pty = unwrap_transparent_type(img, &owner_idx, h->params[p].type, 0);
        if ((pty->kind == STIGI_TYPE_ARRAY || pty->kind == STIGI_TYPE_CELL) &&
            pty->type_arg_count > 0 &&
            type_is_enum_or_carries_facts(img, owner_idx, pty->type_args[0], 0)) {
          (void)add_tdiag(diags, STIGI_TYPE_EXTERN_FORGES_FACTS, it->span, diag,
                          "body-less extern '%s' overwrites buffer '%.*s' of enum/refined elements",
                          img->units[uidx].name, (int)h->params[p].name.len, h->params[p].name.ptr);
        }
      }
    }
  }
}

/* ---- AST Type -> StigiTy resolution ---- */

static StigiTy *alloc_ty(Arena *a, StigiTyKind kind) {
  StigiTy *t = arena_alloc(a, sizeof *t);
  if (t != NULL) {
    memset(t, 0, sizeof *t);
    t->kind = kind;
  }
  return t;
}

typedef struct TyCtx TyCtx;

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

static StigiTy *resolve_ast_type_ctx(const StigiImage *img, uint32_t from_idx,
                                     const StigiType *ast_ty, Arena *arena,
                                     InternPool *lin_intern, const TyCtx *tctx,
                                     StigiTypeDiags *diags, Diag *diag, int depth) {
  if (ast_ty == NULL || depth > 16) return alloc_ty(arena, STIGI_TY_UNKNOWN);
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

  if (ast_ty->kind == STIGI_TYPE_ARRAY || ast_ty->kind == STIGI_TYPE_PTR) {
    StigiTy *t = alloc_ty(arena, ast_ty->kind == STIGI_TYPE_ARRAY ? STIGI_TY_ARRAY : STIGI_TY_PTR);
    if (t == NULL) return NULL;
    t->elem = ast_ty->type_arg_count > 0
                  ? resolve_ast_type_ctx(img, from_idx, ast_ty->type_args[0], arena, lin_intern,
                                         tctx, diags, diag, depth + 1)
                  : alloc_ty(arena, STIGI_TY_UNKNOWN);
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
                  : alloc_ty(arena, STIGI_TY_UNKNOWN);
    return t;
  }

  int32_t tgt = stigi_image_resolve_name(img, from_idx, ast_ty->name.ptr, ast_ty->name.len);
  if (tgt >= 0) {
    const StigiDecl *d = img->units[tgt].ast->decl;
    if (d->kind == STIGI_DECL_ENUM || d->kind == STIGI_DECL_TYPE) {
      StigiTy *t = NULL;
      if (d->kind == STIGI_DECL_ENUM) {
        t = alloc_ty(arena, STIGI_TY_ENUM);
        if (t != NULL) t->unit_idx = (uint32_t)tgt;
      } else {
        StigiTy *rhs = resolve_ast_type_ctx(img, (uint32_t)tgt, d->u.type_decl.rhs, arena,
                                            lin_intern, tctx, diags, diag, depth + 1);
        if (!d->u.type_decl.has_facts && ast_ty->len_arg_count == 0 && ast_ty->type_arg_count == 0) {
          return rhs; /* transparent alias with no args */
        }
        t = alloc_ty(arena, STIGI_TY_TYPEDEF);
        if (t != NULL) {
          t->unit_idx = (uint32_t)tgt;
          t->elem = rhs;
        }
      }
      if (t != NULL && ast_ty->type_arg_count > 0) {
        t->type_args = arena_alloc(arena, (size_t)ast_ty->type_arg_count * sizeof *t->type_args);
        if (t->type_args != NULL) {
          t->type_arg_count = ast_ty->type_arg_count;
          for (uint32_t i = 0; i < ast_ty->type_arg_count; i++) {
            t->type_args[i] = resolve_ast_type_ctx(img, from_idx, ast_ty->type_args[i], arena,
                                                   lin_intern, tctx, diags, diag, depth + 1);
          }
        }
      }
      if (t != NULL && ast_ty->len_arg_count > 0) {
        t->len_args = arena_alloc(arena, (size_t)ast_ty->len_arg_count * sizeof *t->len_args);
        if (t->len_args != NULL) {
          t->len_arg_count = ast_ty->len_arg_count;
          for (uint32_t i = 0; i < ast_ty->len_arg_count; i++) {
            if (linear_from_expr_ctx(ast_ty->len_args[i], tctx, lin_intern,
                                     &t->len_args[i]) != STIGI_OK && diags != NULL) {
              (void)add_tdiag(diags, STIGI_TYPE_LENGTH_NOT_LINEAR, ast_ty->len_args[i]->span, diag,
                              "length term is not linear");
            }
          }
        }
      }
      return t;
    }
  }

  return alloc_ty(arena, STIGI_TY_UNKNOWN);
}

/* ---- Expression type checker ---- */

typedef struct {
  StigiStr name;
  StigiTy *ty;
  bool is_mut;
  uint32_t binder_id;
  bool has_const_val;
  int64_t const_val;
} TyBinding;

struct TyCtx {
  const StigiImage *img;
  uint32_t uidx;
  Arena *arena;
  InternPool *lin_intern;
  StigiTypeDiags *diags;
  Diag *diag;
  TyBinding locals[256];
  uint32_t local_count;
  uint32_t next_binder_id;
};

static const TyBinding *find_local(const TyCtx *ctx, StigiStr name) {
  if (ctx == NULL) return NULL;
  for (uint32_t i = ctx->local_count; i > 0; i--) {
    if (ctx->locals[i - 1].name.len == name.len &&
        memcmp(ctx->locals[i - 1].name.ptr, name.ptr, name.len) == 0) {
      return &ctx->locals[i - 1];
    }
  }
  return NULL;
}

static void push_ty_local_val(TyCtx *ctx, StigiStr name, StigiTy *ty, bool is_mut,
                              bool has_const, int64_t const_val) {
  if (ctx->local_count < 256 && name.len > 0) {
    TyBinding *b = &ctx->locals[ctx->local_count++];
    b->name = name;
    b->ty = ty;
    b->is_mut = is_mut;
    b->binder_id = ++ctx->next_binder_id;
    b->has_const_val = has_const;
    b->const_val = const_val;
  }
}

static void push_ty_local(TyCtx *ctx, StigiStr name, StigiTy *ty, bool is_mut) {
  push_ty_local_val(ctx, name, ty, is_mut, false, 0);
}

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
        if (b->is_mut) return STIGI_ERR_INVALID; /* Fix #10: length term never names a var */
        if (b->has_const_val) {
          *out = stigi_linear_const(b->const_val);
          return STIGI_OK;
        }
        *out = stigi_linear_var(b->binder_id);
        return STIGI_OK;
      }
      /* Fold top-level immutable val if integer literal */
      if (ctx != NULL && ctx->img != NULL) {
        int32_t tgt = stigi_image_resolve_name(ctx->img, ctx->uidx,
                                               e->u.ident.name.ptr, e->u.ident.name.len);
        if (tgt >= 0) {
          const StigiDecl *d = ctx->img->units[tgt].ast->decl;
          if (d->kind == STIGI_DECL_VAL && d->u.val_decl.body != NULL &&
              d->u.val_decl.body->kind == STIGI_EXPR_LIT_INT &&
              !d->u.val_decl.body->u.lit_int.overflow &&
              d->u.val_decl.body->u.lit_int.value <= (uint64_t)INT64_MAX) {
            *out = stigi_linear_const((int64_t)d->u.val_decl.body->u.lit_int.value);
            return STIGI_OK;
          }
        }
      }
      uint32_t id = 0;
      Diag d;
      diag_clear(&d);
      Status st = intern_str(intern, e->u.ident.name.ptr, e->u.ident.name.len, &id, &d);
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

static void check_int_lit_range(TyCtx *ctx, uint64_t val, bool is_neg,
                                const StigiTy *expected, StigiSpan sp) {
  if (expected == NULL) return;
  bool oob = false;
  switch (expected->kind) {
    case STIGI_TY_U8: oob = is_neg || val > 255ULL; break;
    case STIGI_TY_U32: oob = is_neg || val > 4294967295ULL; break;
    case STIGI_TY_U64: oob = is_neg; break;
    case STIGI_TY_I32:
      oob = is_neg ? (val > 2147483648ULL) : (val > 2147483647ULL);
      break;
    case STIGI_TY_I64:
      oob = is_neg ? (val > 9223372036854775808ULL) : (val > 9223372036854775807ULL);
      break;
    default:
      break;
  }
  if (oob) {
    (void)add_tdiag(ctx->diags, STIGI_TYPE_LITERAL_OUT_OF_RANGE, sp, ctx->diag,
                    "integer literal out of range");
  }
}

static StigiTy *check_expr(TyCtx *ctx, const StigiExpr *e, const StigiTy *expected);

static StigiTy *check_expr(TyCtx *ctx, const StigiExpr *e, const StigiTy *expected) {
  if (e == NULL) return alloc_ty(ctx->arena, STIGI_TY_UNIT);
  switch (e->kind) {
    case STIGI_EXPR_LIT_INT:
      if (e->u.lit_int.overflow) {
        (void)add_tdiag(ctx->diags, STIGI_TYPE_LITERAL_OUT_OF_RANGE, e->span, ctx->diag,
                        "integer literal exceeds U64 range");
      }
      if (expected != NULL && is_int_ty(expected)) {
        check_int_lit_range(ctx, e->u.lit_int.value, false, expected, e->span);
        return alloc_ty(ctx->arena, expected->kind);
      }
      if (expected != NULL && expected->kind == STIGI_TY_TYPEDEF && is_int_ty(expected->elem)) {
        check_int_lit_range(ctx, e->u.lit_int.value, false, expected->elem, e->span);
        return alloc_ty(ctx->arena, expected->elem->kind);
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
      if (b != NULL) return b->ty;
      int32_t tgt = stigi_image_resolve_name(ctx->img, ctx->uidx,
                                             e->u.ident.name.ptr, e->u.ident.name.len);
      if (tgt >= 0) {
        const StigiDecl *d = ctx->img->units[tgt].ast->decl;
        if (d->kind == STIGI_DECL_VAL) {
          return resolve_ast_type(ctx->img, (uint32_t)tgt, d->u.val_decl.type,
                                  ctx->arena, ctx->lin_intern, ctx->diags, ctx->diag, 0);
        }
      }
      return alloc_ty(ctx->arena, STIGI_TY_UNKNOWN);
    }
    case STIGI_EXPR_UNOP: {
      if (e->u.unop.op == STIGI_OP_NEG && e->u.unop.operand->kind == STIGI_EXPR_LIT_INT) {
        if (expected != NULL && is_int_ty(expected)) {
          check_int_lit_range(ctx, e->u.unop.operand->u.lit_int.value, true, expected, e->span);
          return alloc_ty(ctx->arena, expected->kind);
        }
        return alloc_ty(ctx->arena, STIGI_TY_I64);
      }
      StigiTy *opnd = check_expr(ctx, e->u.unop.operand, expected);
      if (e->u.unop.op == STIGI_OP_NOT) {
        if (opnd->kind != STIGI_TY_BOOL && opnd->kind != STIGI_TY_UNKNOWN) {
          (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, e->span, ctx->diag,
                          "'!' requires Bool operand");
        }
        return alloc_ty(ctx->arena, STIGI_TY_BOOL);
      }
      return opnd;
    }
    case STIGI_EXPR_BINOP: {
      StigiOp op = e->u.binop.op;
      if (op == STIGI_OP_AND || op == STIGI_OP_OR) {
        StigiTy *bty = alloc_ty(ctx->arena, STIGI_TY_BOOL);
        StigiTy *lt = check_expr(ctx, e->u.binop.lhs, bty);
        StigiTy *rt = check_expr(ctx, e->u.binop.rhs, bty);
        if (!stigi_ty_equal(lt, bty) || !stigi_ty_equal(rt, bty)) {
          (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, e->span, ctx->diag,
                          "logical operator requires Bool operands");
        }
        return bty;
      }
      const StigiTy *hint = (op >= STIGI_OP_ADD && op <= STIGI_OP_MOD) ? expected : NULL;
      StigiTy *lt = check_expr(ctx, e->u.binop.lhs, hint);
      StigiTy *rt = check_expr(ctx, e->u.binop.rhs, lt->kind != STIGI_TY_UNKNOWN ? lt : hint);
      if (!stigi_ty_equal(lt, rt)) {
        (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, e->span, ctx->diag,
                        "binary operator operand type mismatch");
      }
      if (op >= STIGI_OP_LT && op <= STIGI_OP_NE) {
        return alloc_ty(ctx->arena, STIGI_TY_BOOL);
      }
      return lt;
    }
    case STIGI_EXPR_IF: {
      StigiTy *bty = alloc_ty(ctx->arena, STIGI_TY_BOOL);
      StigiTy *ct = check_expr(ctx, e->u.if_expr.cond, bty);
      if (!stigi_ty_equal(ct, bty)) {
        (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, e->u.if_expr.cond->span, ctx->diag,
                        "'if' condition must be Bool");
      }
      StigiTy *tt = check_expr(ctx, e->u.if_expr.then_br, expected);
      StigiTy *et = check_expr(ctx, e->u.if_expr.else_br, expected != NULL ? expected : tt);
      if (!stigi_ty_equal(tt, et)) {
        (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, e->span, ctx->diag,
                        "'if' branch types do not match");
      }
      return tt;
    }
    case STIGI_EXPR_WHILE: {
      StigiTy *bty = alloc_ty(ctx->arena, STIGI_TY_BOOL);
      StigiTy *ct = check_expr(ctx, e->u.while_expr.cond, bty);
      if (!stigi_ty_equal(ct, bty)) {
        (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, e->u.while_expr.cond->span, ctx->diag,
                        "'while' condition must be Bool");
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
      if (ann != NULL && !stigi_ty_equal(ann, it)) {
        (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, e->span, ctx->diag,
                        "val initializer type does not match annotation");
      }
      if (e->u.val_stmt.pat.kind == STIGI_PAT_VAR) {
        bool is_c = (e->u.val_stmt.init != NULL &&
                     e->u.val_stmt.init->kind == STIGI_EXPR_LIT_INT &&
                     !e->u.val_stmt.init->u.lit_int.overflow &&
                     e->u.val_stmt.init->u.lit_int.value <= (uint64_t)INT64_MAX);
        int64_t cval = is_c ? (int64_t)e->u.val_stmt.init->u.lit_int.value : 0;
        push_ty_local_val(ctx, e->u.val_stmt.pat.name, ann != NULL ? ann : it, false, is_c, cval);
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
      if (ann != NULL && !stigi_ty_equal(ann, it)) {
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
      if (b != NULL && !stigi_ty_equal(b->ty, vt)) {
        (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, e->span, ctx->diag,
                        "assignment type mismatch");
      }
      return alloc_ty(ctx->arena, STIGI_TY_UNIT);
    }
    case STIGI_EXPR_CALL: {
      /* Check if callee is a local array/cell (element read) */
      if (e->u.call.callee->kind == STIGI_EXPR_LOCAL) {
        const TyBinding *b = find_local(ctx, e->u.call.callee->u.ident.name);
        if (b != NULL && b->ty != NULL &&
            (b->ty->kind == STIGI_TY_ARRAY || b->ty->kind == STIGI_TY_CELL)) {
          for (uint32_t i = 0; i < e->u.call.arg_count; i++) {
            (void)check_expr(ctx, e->u.call.args[i], alloc_ty(ctx->arena, STIGI_TY_U64));
          }
          return b->ty->elem != NULL ? b->ty->elem : alloc_ty(ctx->arena, STIGI_TY_UNKNOWN);
        }
      }
      /* Check if callee is a top-level def or extern def */
      if (e->u.call.callee->kind == STIGI_EXPR_LOCAL ||
          e->u.call.callee->kind == STIGI_EXPR_GLOBAL) {
        StigiStr cn = e->u.call.callee->u.ident.name;
        if (str_eq_c(cn, "Stigi.Test.expect")) {
          if (e->u.call.arg_count == 3) {
            StigiTy *t0 = check_expr(ctx, e->u.call.args[0], NULL);
            StigiTy *t1 = check_expr(ctx, e->u.call.args[1], t0);
            if (!stigi_ty_equal(t0, t1)) {
              (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, e->span, ctx->diag,
                              "Stigi.Test.expect operands must have same type");
            }
          }
          return alloc_ty(ctx->arena, STIGI_TY_TEST_RESULT);
        }
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
            if (e->u.call.arg_count != h->param_count) {
              (void)add_tdiag(ctx->diags, STIGI_TYPE_ARITY, e->span, ctx->diag,
                              "call to '%s' expects %u args, got %u",
                              ctx->img->units[tgt].name, h->param_count, e->u.call.arg_count);
            } else {
              /* Bind callee params for dependent length substitution (e.g. (3, Array[U8](3))) */
              TyCtx sub_ctx;
              memset(&sub_ctx, 0, sizeof sub_ctx);
              sub_ctx.img = ctx->img;
              sub_ctx.uidx = (uint32_t)tgt;
              sub_ctx.arena = ctx->arena;
              sub_ctx.lin_intern = ctx->lin_intern;
              for (uint32_t i = 0; i < h->param_count; i++) {
                bool is_c = (e->u.call.args[i]->kind == STIGI_EXPR_LIT_INT &&
                             !e->u.call.args[i]->u.lit_int.overflow &&
                             e->u.call.args[i]->u.lit_int.value <= (uint64_t)INT64_MAX);
                int64_t cv = is_c ? (int64_t)e->u.call.args[i]->u.lit_int.value : 0;
                push_ty_local_val(&sub_ctx, h->params[i].name, NULL, false, is_c, cv);
              }
              for (uint32_t i = 0; i < h->param_count; i++) {
                StigiTy *pty = resolve_ast_type_ctx(ctx->img, (uint32_t)tgt, h->params[i].type,
                                                    ctx->arena, ctx->lin_intern, &sub_ctx,
                                                    ctx->diags, ctx->diag, 0);
                StigiTy *aty = check_expr(ctx, e->u.call.args[i], pty);
                if (!stigi_ty_equal(pty, aty)) {
                  (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, e->u.call.args[i]->span,
                                  ctx->diag, "argument %u type mismatch in call to '%s'",
                                  i + 1, ctx->img->units[tgt].name);
                }
              }
            }
            return h->ret_type != NULL
                       ? resolve_ast_type(ctx->img, (uint32_t)tgt, h->ret_type,
                                          ctx->arena, ctx->lin_intern, ctx->diags, ctx->diag, 0)
                       : alloc_ty(ctx->arena, STIGI_TY_UNIT);
          }
        }
      }
      for (uint32_t i = 0; i < e->u.call.arg_count; i++) {
        (void)check_expr(ctx, e->u.call.args[i], NULL);
      }
      return alloc_ty(ctx->arena, STIGI_TY_UNKNOWN);
    }
    case STIGI_EXPR_MATCH: {
      StigiTy *sc = check_expr(ctx, e->u.match_expr.scrutinee, NULL);
      bool has_wild = false;
      bool covered[64];
      memset(covered, 0, sizeof covered);
      const StigiDecl *enum_d = NULL;
      if (sc->kind == STIGI_TY_ENUM && sc->unit_idx < ctx->img->unit_count) {
        enum_d = ctx->img->units[sc->unit_idx].ast->decl;
      }
      StigiTy *res_ty = NULL;
      for (uint32_t i = 0; i < e->u.match_expr.arm_count; i++) {
        const StigiPat *pat = &e->u.match_expr.arms[i].pat;
        uint32_t saved = ctx->local_count;
        if (pat->kind == STIGI_PAT_WILD) {
          has_wild = true;
          if (enum_d != NULL) {
            (void)add_tdiag(ctx->diags, STIGI_TYPE_WILDCARD_ARM, pat->span, ctx->diag,
                            "wildcard '_' arm is not allowed in enum match");
          }
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
            for (uint32_t c = 0; c < enum_d->u.enum_decl.case_count; c++) {
              const StigiEnumCase *ec = &enum_d->u.enum_decl.cases[c];
              if (ec->name.len == pat->name.len &&
                  memcmp(ec->name.ptr, pat->name.ptr, pat->name.len) == 0) {
                if (c < 64) covered[c] = true;
                for (uint32_t f = 0; f < pat->field_count && f < ec->field_count; f++) {
                  StigiTy *fty = resolve_ast_type(ctx->img, sc->unit_idx, ec->fields[f].type,
                                                  ctx->arena, ctx->lin_intern, ctx->diags,
                                                  ctx->diag, 0);
                  push_ty_local(ctx, pat->fields[f], fty, false);
                }
              }
            }
          }
        }
        StigiTy *at = check_expr(ctx, e->u.match_expr.arms[i].body, expected);
        ctx->local_count = saved;
        if (res_ty == NULL) res_ty = at;
        else if (!stigi_ty_equal(res_ty, at)) {
          (void)add_tdiag(ctx->diags, STIGI_TYPE_MISMATCH, e->u.match_expr.arms[i].span,
                          ctx->diag, "match arm type mismatch");
        }
      }
      if (enum_d != NULL) {
        for (uint32_t c = 0; c < enum_d->u.enum_decl.case_count && c < 64; c++) {
          if (!covered[c]) {
            (void)add_tdiag(ctx->diags, STIGI_TYPE_NOT_EXHAUSTIVE, e->span, ctx->diag,
                            "match on '%s' is not exhaustive (missing case '%.*s')",
                            ctx->img->units[sc->unit_idx].name,
                            (int)enum_d->u.enum_decl.cases[c].name.len,
                            enum_d->u.enum_decl.cases[c].name.ptr);
          }
        }
      } else if (is_int_ty(sc) && !has_wild) {
        (void)add_tdiag(ctx->diags, STIGI_TYPE_NOT_EXHAUSTIVE, e->span, ctx->diag,
                        "integer match must end with 'case _'");
      }
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
    case STIGI_EXPR_HANDLE:
      return e->u.handle_stmt.body != NULL
                 ? check_expr(ctx, e->u.handle_stmt.body, expected)
                 : alloc_ty(ctx->arena, STIGI_TY_UNIT);
    case STIGI_EXPR_INDEX_WRITE:
      (void)check_expr(ctx, e->u.index_expr.target, NULL);
      (void)check_expr(ctx, e->u.index_expr.index, alloc_ty(ctx->arena, STIGI_TY_U64));
      (void)check_expr(ctx, e->u.index_expr.value, NULL);
      return alloc_ty(ctx->arena, STIGI_TY_UNIT);
    default:
      return alloc_ty(ctx->arena, STIGI_TY_UNKNOWN);
  }
}

Status stigi_typecheck_image(const StigiImage *img, StigiTypeDiags *diags, Diag *diag) {
  if (img == NULL) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_typecheck_image: invalid arguments");
  }

  /* 1. Check recursive types (S43/S69) across all enum and type units */
  if (img->unit_count > 0) {
    uint8_t *color = calloc(img->unit_count, sizeof *color);
    if (color == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "out of memory");
    for (uint32_t i = 0; i < img->unit_count; i++) {
      const StigiDecl *d = img->units[i].ast->decl;
      if ((d->kind == STIGI_DECL_ENUM || d->kind == STIGI_DECL_TYPE) && color[i] == 0) {
        (void)dfs_recursive_type(img, i, color, diags, diag);
      }
    }
    free(color);
  }

  /* 2. Check extern-forges-facts (S110) and type check each unit */
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

    if (d->kind == STIGI_DECL_EXTERN && !d->u.extern_decl.is_val &&
        d->u.extern_decl.body == NULL) {
      const StigiDefHead *h = &d->u.extern_decl.head;
      if (type_carries_facts(img, i, h->ret_type, 0)) {
        (void)add_tdiag(diags, STIGI_TYPE_EXTERN_FORGES_FACTS, d->span, diag,
                        "body-less extern '%s' returns a type carrying facts", img->units[i].name);
      }
      for (uint32_t g = 0; g < h->pre_group_count; g++) {
        if (h->pre_groups[g].is_fact) {
          check_overwrites_group(img, i, h, &h->pre_groups[g].facts, diags, diag);
        }
      }
      for (uint32_t g = 0; g < h->post_val_fact_count; g++) {
        check_overwrites_group(img, i, h, &h->post_val_facts[g], diags, diag);
      }
      for (uint32_t g = 0; g < h->exit_fact_count; g++) {
        check_overwrites_group(img, i, h, &h->exit_facts[g], diags, diag);
      }
      /* Also validate parameter/return types (including linear lengths) */
      for (uint32_t p = 0; p < h->param_count; p++) {
        (void)resolve_ast_type(img, i, h->params[p].type, &arena, &lin_intern, diags, diag, 0);
      }
      (void)resolve_ast_type(img, i, h->ret_type, &arena, &lin_intern, diags, diag, 0);
      continue;
    }

    if (d->kind == STIGI_DECL_DEF || d->kind == STIGI_DECL_LEMMA_DEF) {
      TyCtx ctx;
      memset(&ctx, 0, sizeof ctx);
      ctx.img = img;
      ctx.uidx = i;
      ctx.arena = &arena;
      ctx.lin_intern = &lin_intern;
      ctx.diags = diags;
      ctx.diag = diag;

      const StigiDefHead *h = &d->u.def_decl.head;
      for (uint32_t p = 0; p < h->param_count; p++) {
        StigiTy *pty = resolve_ast_type_ctx(img, i, h->params[p].type, &arena, &lin_intern,
                                            &ctx, diags, diag, 0);
        push_ty_local(&ctx, h->params[p].name, pty, false);
      }
      StigiTy *ret_ty = h->ret_type != NULL
                            ? resolve_ast_type_ctx(img, i, h->ret_type, &arena, &lin_intern,
                                                   &ctx, diags, diag, 0)
                            : alloc_ty(&arena, STIGI_TY_UNIT);
      StigiTy *body_ty = check_expr(&ctx, d->u.def_decl.body, ret_ty);
      if (d->kind == STIGI_DECL_DEF && !stigi_ty_equal(ret_ty, body_ty)) {
        (void)add_tdiag(diags, STIGI_TYPE_MISMATCH, d->span, diag,
                        "body of '%s' does not match declared return type", img->units[i].name);
      }
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
      StigiTy *body_ty = check_expr(&ctx, d->u.val_decl.body, decl_ty);
      if ((d->kind == STIGI_DECL_VAL || d->kind == STIGI_DECL_TEST) &&
          !stigi_ty_equal(decl_ty, body_ty)) {
        (void)add_tdiag(diags, STIGI_TYPE_MISMATCH, d->span, diag,
                        "initializer of val '%s' does not match declared type",
                        img->units[i].name);
      }
    } else if (d->kind == STIGI_DECL_TYPE) {
      (void)resolve_ast_type(img, i, d->u.type_decl.rhs, &arena, &lin_intern, diags, diag, 0);
    }
  }

  intern_free(&lin_intern);
  arena_free(&arena);
  return STIGI_OK;
}
