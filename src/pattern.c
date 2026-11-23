#include "pattern.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  StigiImage *img;
  uint32_t uidx;
  StigiTypeDiags *diags;
  Diag *diag;
  uint32_t fresh_seq;
  Status err;
  const StigiDecl *cur_decl; /* the def or val whose body is elaborated: declared local types (S129) */
  StigiExpr *missing_body;   /* stands in for a missing case after not-exhaustive: typed like the match */
  bool arity_dropped;        /* this match dropped an alternative after pattern-arity: no not-exhaustive */
} ElabCtx;

typedef struct {
  StigiStr name;
  StigiExpr *rhs;
  StigiSpan span;
} PatBind;

typedef struct {
  StigiPat *pats;
  uint32_t ncols;
  PatBind *binds;
  uint32_t nbinds;
  StigiExpr *guard;
  StigiExpr *body;
  uint32_t arm_idx;
  StigiSpan span;
} PatRow;

static void *ea_alloc(ElabCtx *ctx, size_t n) {
  void *p = arena_alloc(&ctx->img->arena, n);
  if (p == NULL) {
    ctx->err = diag_set(ctx->diag, STIGI_ERR_OOM, -1, "pattern elaboration OOM");
  }
  return p;
}

static StigiStr ea_fmt_str(ElabCtx *ctx, const char *fmt, uint32_t id) {
  char tmp[48];
  int n = snprintf(tmp, sizeof tmp, fmt, id);
  if (n <= 0) return (StigiStr){"_", 1};
  size_t len = (size_t)n;
  char *buf = ea_alloc(ctx, len + 1U);
  if (buf == NULL) return (StigiStr){"_", 1};
  memcpy(buf, tmp, len + 1U);
  return (StigiStr){buf, (uint32_t)len};
}

static void elab_add_diag(ElabCtx *ctx, StigiTypeCode code, StigiSpan span,
                          const char *fmt, ...) {
  if (ctx->diags == NULL) return;
  if (ctx->diags->count == ctx->diags->cap) {
    uint32_t ncap = ctx->diags->cap == 0 ? 8U : ctx->diags->cap * 2U;
    if (ncap <= ctx->diags->cap) return;
    StigiTypeDiag *ni = realloc(ctx->diags->items, (size_t)ncap * sizeof *ni);
    if (ni == NULL) {
      ctx->err = diag_set(ctx->diag, STIGI_ERR_OOM, -1, "pattern diag OOM");
      return;
    }
    ctx->diags->items = ni;
    ctx->diags->cap = ncap;
  }
  StigiTypeDiag *item = &ctx->diags->items[ctx->diags->count++];
  item->code = code;
  item->span = span;
  va_list ap;
  va_start(ap, fmt);
  (void)vsnprintf(item->msg, sizeof item->msg, fmt, ap);
  va_end(ap);
}

static bool str_eq_ss(StigiStr a, StigiStr b) {
  return a.len == b.len && (a.len == 0 || memcmp(a.ptr, b.ptr, a.len) == 0);
}

static StigiStr case_short_name(StigiStr s) {
  for (uint32_t i = s.len; i > 0; i--) {
    if (s.ptr[i - 1] == '.') {
      return (StigiStr){s.ptr + i, s.len - i};
    }
  }
  return s;
}

static StigiExpr *make_local_expr(ElabCtx *ctx, StigiStr name, StigiSpan sp) {
  StigiExpr *e = ea_alloc(ctx, sizeof *e);
  if (e == NULL) return NULL;
  memset(e, 0, sizeof *e);
  e->kind = STIGI_EXPR_LOCAL;
  e->span = sp;
  e->u.ident.name = name;
  return e;
}

static StigiExpr *make_unit_expr(ElabCtx *ctx, StigiSpan sp) {
  StigiExpr *e = ea_alloc(ctx, sizeof *e);
  if (e == NULL) return NULL;
  memset(e, 0, sizeof *e);
  e->kind = STIGI_EXPR_LIT_UNIT;
  e->span = sp;
  return e;
}

static StigiExpr *make_val_var_stmt(ElabCtx *ctx, StigiStr name, StigiExpr *init, StigiSpan sp) {
  StigiExpr *e = ea_alloc(ctx, sizeof *e);
  if (e == NULL) return NULL;
  memset(e, 0, sizeof *e);
  e->kind = STIGI_EXPR_VAL;
  e->span = sp;
  e->u.val_stmt.pat.kind = STIGI_PAT_VAR;
  e->u.val_stmt.pat.span = sp;
  e->u.val_stmt.pat.name = name;
  e->u.val_stmt.init = init;
  return e;
}

static StigiExpr *make_if_expr(ElabCtx *ctx, StigiExpr *cond, StigiExpr *tb, StigiExpr *eb,
                               StigiSpan sp) {
  StigiExpr *e = ea_alloc(ctx, sizeof *e);
  if (e == NULL) return NULL;
  memset(e, 0, sizeof *e);
  e->kind = STIGI_EXPR_IF;
  e->span = sp;
  e->u.if_expr.cond = cond;
  e->u.if_expr.then_br = tb;
  e->u.if_expr.else_br = eb;
  return e;
}

static StigiExpr *make_eq_lit(ElabCtx *ctx, StigiExpr *lhs, const StigiPat *lit_pat, StigiSpan sp) {
  StigiExpr *rhs = ea_alloc(ctx, sizeof *rhs);
  if (rhs == NULL) return NULL;
  memset(rhs, 0, sizeof *rhs);
  rhs->span = lit_pat->span;
  if (lit_pat->kind == STIGI_PAT_CHAR) {
    rhs->kind = STIGI_EXPR_LIT_CHAR;
    rhs->u.lit_char.value = lit_pat->char_val;
    rhs->u.lit_char.raw = lit_pat->name;
  } else {
    rhs->kind = STIGI_EXPR_LIT_INT;
    rhs->u.lit_int.value = lit_pat->int_val;
    rhs->u.lit_int.overflow = lit_pat->int_overflow;
    if (lit_pat->int_neg && lit_pat->name.len > 1 && lit_pat->name.ptr[0] == '-') {
      rhs->u.lit_int.raw.ptr = lit_pat->name.ptr + 1;
      rhs->u.lit_int.raw.len = lit_pat->name.len - 1U;
      StigiExpr *neg = ea_alloc(ctx, sizeof *neg);
      if (neg == NULL) return NULL;
      memset(neg, 0, sizeof *neg);
      neg->kind = STIGI_EXPR_UNOP;
      neg->span = lit_pat->span;
      neg->u.unop.op = STIGI_OP_NEG;
      neg->u.unop.operand = rhs;
      rhs = neg;
    } else {
      rhs->u.lit_int.raw = lit_pat->name;
    }
  }
  StigiExpr *cmp = ea_alloc(ctx, sizeof *cmp);
  if (cmp == NULL) return NULL;
  memset(cmp, 0, sizeof *cmp);
  cmp->kind = STIGI_EXPR_BINOP;
  cmp->span = sp;
  cmp->u.binop.op = STIGI_OP_EQ;
  cmp->u.binop.lhs = lhs;
  cmp->u.binop.rhs = rhs;
  return cmp;
}

static StigiExpr *wrap_bindings(ElabCtx *ctx, const PatBind *binds, uint32_t nbinds,
                                StigiExpr *body) {
  if (nbinds == 0 || body == NULL) return body;
  uint32_t real = 0;
  for (uint32_t i = 0; i < nbinds; i++) {
    if (binds[i].name.len == 0 ||
        (binds[i].name.len == 1 && binds[i].name.ptr[0] == '_')) {
      continue;
    }
    if (binds[i].rhs != NULL && binds[i].rhs->kind == STIGI_EXPR_LOCAL &&
        str_eq_ss(binds[i].name, binds[i].rhs->u.ident.name)) {
      continue;
    }
    real++;
  }
  if (real == 0) return body;
  StigiExpr *blk = ea_alloc(ctx, sizeof *blk);
  if (blk == NULL) return body;
  memset(blk, 0, sizeof *blk);
  blk->kind = STIGI_EXPR_BLOCK;
  blk->span = body->span;
  uint32_t extra = (body->kind == STIGI_EXPR_BLOCK) ? body->u.block.count : 1U;
  blk->u.block.stmts = ea_alloc(ctx, (size_t)(real + extra) * sizeof *blk->u.block.stmts);
  if (blk->u.block.stmts == NULL) return body;
  uint32_t w = 0;
  for (uint32_t i = 0; i < nbinds; i++) {
    if (binds[i].name.len == 0 ||
        (binds[i].name.len == 1 && binds[i].name.ptr[0] == '_')) {
      continue;
    }
    if (binds[i].rhs != NULL && binds[i].rhs->kind == STIGI_EXPR_LOCAL &&
        str_eq_ss(binds[i].name, binds[i].rhs->u.ident.name)) {
      continue;
    }
    blk->u.block.stmts[w++] = make_val_var_stmt(ctx, binds[i].name, binds[i].rhs, binds[i].span);
  }
  if (body->kind == STIGI_EXPR_BLOCK) {
    for (uint32_t i = 0; i < body->u.block.count; i++) {
      blk->u.block.stmts[w++] = body->u.block.stmts[i];
    }
  } else {
    blk->u.block.stmts[w++] = body;
  }
  blk->u.block.count = w;
  return blk;
}

static bool pat_is_1level_simple(const StigiPat *p) {
  if (p == NULL) return true;
  switch (p->kind) {
    case STIGI_PAT_WILD:
    case STIGI_PAT_VAR:
    case STIGI_PAT_BOOL:
      return true;
    case STIGI_PAT_INT:
      return !p->int_neg;
    case STIGI_PAT_CHAR:
    case STIGI_PAT_TUPLE:
    case STIGI_PAT_OR:
    case STIGI_PAT_RANGE:
    case STIGI_PAT_STR:
      return false;
    case STIGI_PAT_CTOR:
      if (p->subpat_count == 0) return true;
      for (uint32_t i = 0; i < p->subpat_count; i++) {
        if (p->subpats[i].kind != STIGI_PAT_VAR && p->subpats[i].kind != STIGI_PAT_WILD) {
          return false;
        }
      }
      return true;
  }
  return false;
}

static bool pat_is_wild_or_var(const StigiPat *p) {
  return p->kind == STIGI_PAT_WILD || p->kind == STIGI_PAT_VAR;
}

static bool lit_pats_equal(const StigiPat *a, const StigiPat *b) {
  if (a->kind != b->kind) return false;
  if (a->kind == STIGI_PAT_CHAR) return a->char_val == b->char_val;
  if (a->kind == STIGI_PAT_INT) return a->int_neg == b->int_neg && a->int_val == b->int_val;
  if (a->kind == STIGI_PAT_BOOL) return a->bool_val == b->bool_val;
  return false;
}

static StigiExpr *elab_expr(ElabCtx *ctx, const StigiExpr *e);

/* Name of the closed enum a column ranges over: the enum of the first resolvable constructor pattern in it. */
static const char *column_enum_name(const ElabCtx *ctx, const PatRow *rows, uint32_t nrows, uint32_t col) {
  for (uint32_t r = 0; r < nrows; r++) {
    const StigiPat *p = &rows[r].pats[col];
    if (p->kind != STIGI_PAT_CTOR) continue;
    uint32_t case_idx = 0;
    bool ambig = false;
    int32_t eidx = stigi_image_resolve_enum_case(ctx->img, ctx->uidx, p->name.ptr, p->name.len,
                                                 &case_idx, &ambig);
    if (eidx >= 0 && !ambig) return ctx->img->units[eidx].name;
  }
  return NULL;
}

/* T3 / S121: a source row whose every column is `_` or a binder, without guard, is a catch-all; it is
 * `wildcard-arm` when any column ranges over a closed enum. Checked on the source rows, not on the
 * specialised sub-matrices, so `(Miss, _)` stays legal. */
static void report_wildcard_rows(ElabCtx *ctx, const PatRow *rows, uint32_t nrows, uint32_t ncols) {
  const char *enum_name = NULL;
  for (uint32_t c = 0; c < ncols && enum_name == NULL; c++) {
    enum_name = column_enum_name(ctx, rows, nrows, c);
  }
  if (enum_name == NULL) return;
  for (uint32_t r = 0; r < nrows; r++) {
    bool catch_all = rows[r].guard == NULL;
    for (uint32_t c = 0; c < ncols && catch_all; c++) catch_all = pat_is_wild_or_var(&rows[r].pats[c]);
    if (catch_all) {
      elab_add_diag(ctx, STIGI_TYPE_WILDCARD_ARM, rows[r].pats[0].span,
                    "wildcard '_' arm over closed enum '%s' is forbidden; name every case", enum_name);
    }
  }
}

/* ---- S129 pattern sugar: alternatives, ranges, text, `is` ---- */

static StigiExpr *make_lit_expr(ElabCtx *ctx, const StigiPat *lit_pat) {
  /* the literal of an int/char pattern as an expression: `x == L` with x the scrutinee, then take the rhs */
  StigiExpr *eq = make_eq_lit(ctx, NULL, lit_pat, lit_pat->span);
  return eq == NULL ? NULL : eq->u.binop.rhs;
}

static StigiExpr *make_binop_expr(ElabCtx *ctx, StigiOp op, StigiExpr *lhs, StigiExpr *rhs, StigiSpan sp) {
  StigiExpr *e = ea_alloc(ctx, sizeof *e);
  if (e == NULL) return NULL;
  memset(e, 0, sizeof *e);
  e->kind = STIGI_EXPR_BINOP;
  e->span = sp;
  e->u.binop.op = op;
  e->u.binop.lhs = lhs;
  e->u.binop.rhs = rhs;
  return e;
}

static StigiExpr *make_uint_expr(ElabCtx *ctx, uint64_t v, StigiSpan sp) {
  StigiExpr *e = ea_alloc(ctx, sizeof *e);
  if (e == NULL) return NULL;
  memset(e, 0, sizeof *e);
  e->kind = STIGI_EXPR_LIT_INT;
  e->span = sp;
  e->u.lit_int.value = v;
  char tmp[24];
  int n = snprintf(tmp, sizeof tmp, "%llu", (unsigned long long)v);
  char *raw = n > 0 ? ea_alloc(ctx, (size_t)n + 1U) : NULL;
  if (raw == NULL) return NULL;
  memcpy(raw, tmp, (size_t)n + 1U);
  e->u.lit_int.raw = (StigiStr){raw, (uint32_t)n};
  return e;
}

static StigiExpr *make_bool_expr(ElabCtx *ctx, bool v, StigiSpan sp) {
  StigiExpr *e = ea_alloc(ctx, sizeof *e);
  if (e == NULL) return NULL;
  memset(e, 0, sizeof *e);
  e->kind = STIGI_EXPR_LIT_BOOL;
  e->span = sp;
  e->u.lit_bool.value = v;
  return e;
}

/* Signed value of an int literal pattern, or the byte of a char pattern */
static int cmp_lit(const StigiPat *a, const StigiPat *b) {
  if (a->kind == STIGI_PAT_CHAR) return a->char_val < b->char_val ? -1 : a->char_val > b->char_val;
  if (a->int_neg != b->int_neg) return a->int_neg ? -1 : 1;
  int mag = a->int_val < b->int_val ? -1 : a->int_val > b->int_val;
  return a->int_neg ? -mag : mag;
}

static bool lit_is_zero(const StigiPat *p) {
  return p->kind == STIGI_PAT_CHAR ? p->char_val == 0 : (!p->int_neg && p->int_val == 0);
}

/* A local's type as written in the def or val being elaborated: a parameter, or a typed `val`/`var`. Names are
   unique in a body (names.md), so the first declaration found is the one. NULL when it states no type. */
static const StigiType *find_typed_local(const StigiExpr *e, StigiStr name, int depth);

static const StigiType *find_typed_local_arms(const StigiMatchArm *arms, uint32_t n, StigiStr name, int depth) {
  for (uint32_t i = 0; i < n; i++) {
    const StigiType *t = find_typed_local(arms[i].body, name, depth + 1);
    if (t != NULL) return t;
  }
  return NULL;
}

static const StigiType *find_typed_local(const StigiExpr *e, StigiStr name, int depth) {
  if (e == NULL || depth > 256) return NULL;
  switch (e->kind) {
    case STIGI_EXPR_VAL:
      if (e->u.val_stmt.type != NULL && e->u.val_stmt.pat.kind == STIGI_PAT_VAR &&
          str_eq_ss(e->u.val_stmt.pat.name, name)) {
        return e->u.val_stmt.type;
      }
      return find_typed_local(e->u.val_stmt.init, name, depth + 1);
    case STIGI_EXPR_VAR:
      if (e->u.var_stmt.type != NULL && str_eq_ss(e->u.var_stmt.name, name)) return e->u.var_stmt.type;
      return find_typed_local(e->u.var_stmt.init, name, depth + 1);
    case STIGI_EXPR_BLOCK:
      for (uint32_t i = 0; i < e->u.block.count; i++) {
        const StigiType *t = find_typed_local(e->u.block.stmts[i], name, depth + 1);
        if (t != NULL) return t;
      }
      return NULL;
    case STIGI_EXPR_IF: {
      const StigiType *t = find_typed_local(e->u.if_expr.then_br, name, depth + 1);
      return t != NULL ? t : find_typed_local(e->u.if_expr.else_br, name, depth + 1);
    }
    case STIGI_EXPR_WHILE:
      return find_typed_local(e->u.while_expr.body, name, depth + 1);
    case STIGI_EXPR_MATCH:
      return find_typed_local_arms(e->u.match_expr.arms, e->u.match_expr.arm_count, name, depth);
    case STIGI_EXPR_HANDLE:
      return find_typed_local(e->u.handle_stmt.body, name, depth + 1);
    case STIGI_EXPR_LOCAL:
    case STIGI_EXPR_GLOBAL:
    case STIGI_EXPR_LIT_INT:
    case STIGI_EXPR_LIT_CHAR:
    case STIGI_EXPR_LIT_STR:
    case STIGI_EXPR_LIT_BOOL:
    case STIGI_EXPR_LIT_UNIT:
    case STIGI_EXPR_CALL:
    case STIGI_EXPR_BINOP:
    case STIGI_EXPR_UNOP:
    case STIGI_EXPR_ASSIGN:
    case STIGI_EXPR_INDEX_READ:
    case STIGI_EXPR_INDEX_WRITE:
    case STIGI_EXPR_ASSERT:
    case STIGI_EXPR_DENY:
    case STIGI_EXPR_LOCAL_DECL:
    case STIGI_EXPR_IS:
      return NULL;
  }
  return NULL;
}

static const StigiType *declared_type_of(const ElabCtx *ctx, const StigiExpr *scrut) {
  if (scrut == NULL || scrut->kind != STIGI_EXPR_LOCAL || ctx->cur_decl == NULL) return NULL;
  StigiStr name = scrut->u.ident.name;
  const StigiDecl *d = ctx->cur_decl;
  if (d->kind == STIGI_DECL_DEF || d->kind == STIGI_DECL_LEMMA_DEF) {
    const StigiDefHead *h = &d->u.def_decl.head;
    for (uint32_t i = 0; i < h->param_count; i++) {
      if (str_eq_ss(h->params[i].name, name)) return h->params[i].type;
    }
    return find_typed_local(d->u.def_decl.body, name, 0);
  }
  if (d->kind == STIGI_DECL_VAL || d->kind == STIGI_DECL_TEST) return find_typed_local(d->u.val_decl.body, name, 0);
  return NULL;
}

/* The largest value of a declared unsigned integer type; 0 when the type is not one */
static uint64_t unsigned_type_max(const StigiType *t) {
  if (t == NULL || t->type_arg_count != 0 || t->len_arg_count != 0) return 0;
  static const struct { const char *name; uint64_t max; } ut[] = {
      {"U8", 255U}, {"U32", 4294967295U}, {"U64", UINT64_MAX}};
  for (size_t i = 0; i < sizeof ut / sizeof ut[0]; i++) {
    if (t->name.len == strlen(ut[i].name) && memcmp(t->name.ptr, ut[i].name, t->name.len) == 0) return ut[i].max;
  }
  return 0;
}

/* `lo..hi` on x: `x >= lo && x <= hi` (literal on the right, as the type check reads it), without a half the type
   of x already decides (ruling on S129 ranges): a byte or a declared unsigned x never needs `x >= 0`, nor
   `x <= max` at its type's maximum; lo == hi is `x == lo`. NULL when the range covers the whole type. */
static StigiExpr *make_range_test(ElabCtx *ctx, const StigiPat *r, StigiExpr *x) {
  const StigiPat *lo = &r->subpats[0];
  const StigiPat *hi = &r->subpats[1];
  if (cmp_lit(lo, hi) == 0) return make_eq_lit(ctx, x, lo, r->span);
  uint64_t umax = lo->kind == STIGI_PAT_CHAR ? 255U : unsigned_type_max(declared_type_of(ctx, x));
  bool is_unsigned = umax != 0;
  uint64_t hi_val = hi->kind == STIGI_PAT_CHAR ? hi->char_val : hi->int_val;
  bool need_lo = !(is_unsigned && lit_is_zero(lo));
  bool need_hi = !(is_unsigned && !hi->int_neg && hi_val == umax);
  StigiExpr *lo_t = need_lo ? make_binop_expr(ctx, STIGI_OP_GE, x, make_lit_expr(ctx, lo), r->span) : NULL;
  StigiExpr *hi_t = need_hi ? make_binop_expr(ctx, STIGI_OP_LE, x, make_lit_expr(ctx, hi), r->span) : NULL;
  if (lo_t != NULL && hi_t != NULL) return make_binop_expr(ctx, STIGI_OP_AND, lo_t, hi_t, r->span);
  return lo_t != NULL ? lo_t : hi_t;
}

/* How a text pattern meets its scrutinee (ruling on S129 text): the scrutinee must be a local declared
   Array[U8](n); a literal n equal to the text length needs no length test, a different one never matches. */
typedef enum { TEXT_BAD_SCRUTINEE, TEXT_NEVER, TEXT_SAME_LENGTH, TEXT_TEST_LENGTH } TextPlan;

static StigiExpr *text_length_expr(const ElabCtx *ctx, StigiExpr *s) {
  const StigiType *t = declared_type_of(ctx, s);
  if (t == NULL || t->len_arg_count != 1 || t->type_arg_count != 1 ||
      !(t->name.len == 5 && memcmp(t->name.ptr, "Array", 5) == 0)) {
    return NULL;
  }
  return t->len_args[0];
}

static TextPlan text_plan(ElabCtx *ctx, const StigiPat *tp, StigiExpr *s) {
  StigiExpr *n = text_length_expr(ctx, s);
  if (n == NULL) {
    elab_add_diag(ctx, STIGI_TYPE_MISMATCH, tp->span, "text pattern needs a named Array[U8](n) scrutinee");
    return TEXT_BAD_SCRUTINEE;
  }
  if (n->kind != STIGI_EXPR_LIT_INT) return TEXT_TEST_LENGTH;
  return n->u.lit_int.value == tp->int_val ? TEXT_SAME_LENGTH : TEXT_NEVER;
}

/* "text" on s: Array[U8](n): the length test when asked, then every byte; every failed test goes to `no` */
static StigiExpr *make_text_test(ElabCtx *ctx, const StigiPat *tp, StigiExpr *s, bool test_length, StigiExpr *yes,
                                 StigiExpr *no) {
  StigiExpr *n = text_length_expr(ctx, s);
  uint64_t len = tp->int_val;
  StigiExpr *body = yes;
  for (uint64_t i = len; i > 0; i--) {
    /* `s(i)`: an array read is a call on the array, as the parser builds it */
    StigiExpr *rd = ea_alloc(ctx, sizeof *rd);
    StigiExpr **args = ea_alloc(ctx, sizeof *args);
    if (rd == NULL || args == NULL) return no;
    memset(rd, 0, sizeof *rd);
    rd->kind = STIGI_EXPR_CALL;
    rd->span = tp->span;
    rd->u.call.callee = s;
    args[0] = make_uint_expr(ctx, i - 1U, tp->span);
    rd->u.call.args = args;
    rd->u.call.arg_count = 1;
    rd->u.call.has_parens = true;
    StigiExpr *lit = make_uint_expr(ctx, tp->str_bytes[i - 1U], tp->span);
    StigiExpr *cmp = make_binop_expr(ctx, STIGI_OP_EQ, rd, lit, tp->span);
    body = make_if_expr(ctx, cmp, body, no, tp->span);
  }
  if (!test_length || n == NULL) return body;
  StigiExpr *len_cmp = make_binop_expr(ctx, STIGI_OP_EQ, n, make_uint_expr(ctx, len, tp->span), tp->span);
  return make_if_expr(ctx, len_cmp, body, no, tp->span);
}

/* Binder names a pattern introduces, appended to *out (heap, grows) */
static void pat_binders(const StigiPat *p, StigiStr **out, uint32_t *n, uint32_t *cap) {
  if (p->kind == STIGI_PAT_VAR || (p->kind == STIGI_PAT_CTOR && p->subpat_count == 0)) {
    uint32_t add = p->kind == STIGI_PAT_VAR ? 1U : p->field_count;
    for (uint32_t i = 0; i < add; i++) {
      StigiStr nm = p->kind == STIGI_PAT_VAR ? p->name : p->fields[i];
      if (nm.len == 1 && nm.ptr[0] == '_') continue;
      if (*n == *cap) {
        uint32_t nc = *cap == 0 ? 8U : *cap * 2U;
        StigiStr *g = realloc(*out, (size_t)nc * sizeof *g);
        if (g == NULL) return;
        *out = g;
        *cap = nc;
      }
      (*out)[(*n)++] = nm;
    }
    return;
  }
  if (p->kind == STIGI_PAT_OR) {
    if (p->subpat_count > 0) pat_binders(&p->subpats[0], out, n, cap);
    return;
  }
  for (uint32_t i = 0; i < p->subpat_count; i++) pat_binders(&p->subpats[i], out, n, cap);
}

static int cmp_str_q(const void *a, const void *b) {
  const StigiStr *x = a, *y = b;
  uint32_t m = x->len < y->len ? x->len : y->len;
  int c = memcmp(x->ptr, y->ptr, m);
  return c != 0 ? c : (x->len > y->len) - (x->len < y->len);
}

/* Whether two patterns bind the same names (any order) */
static bool same_binders(const StigiPat *a, const StigiPat *b) {
  StigiStr *ba = NULL, *bb = NULL;
  uint32_t na = 0, ca = 0, nb = 0, cb = 0;
  pat_binders(a, &ba, &na, &ca);
  pat_binders(b, &bb, &nb, &cb);
  if (na > 1) qsort(ba, na, sizeof *ba, cmp_str_q);
  if (nb > 1) qsort(bb, nb, sizeof *bb, cmp_str_q);
  bool same = na == nb;
  for (uint32_t k = 0; same && k < na; k++) same = str_eq_ss(ba[k], bb[k]);
  free(ba);
  free(bb);
  return same;
}

/* Checks inside one pattern: alternatives bind the same names (pattern-arity), ranges are ordered (mismatch) */
static void check_pat_sugar(ElabCtx *ctx, const StigiPat *p) {
  if (p->kind == STIGI_PAT_RANGE && cmp_lit(&p->subpats[0], &p->subpats[1]) > 0) {
    elab_add_diag(ctx, STIGI_TYPE_MISMATCH, p->span, "range low end is above its high end");
  }
  if (p->kind == STIGI_PAT_OR) {
    for (uint32_t i = 1; i < p->subpat_count; i++) {
      if (!same_binders(&p->subpats[0], &p->subpats[i])) {
        elab_add_diag(ctx, STIGI_TYPE_PATTERN_ARITY, p->subpats[i].span,
                      "every alternative of a pattern must bind the same names");
      }
    }
  }
  if (p->kind != STIGI_PAT_RANGE) {
    for (uint32_t i = 0; i < p->subpat_count; i++) check_pat_sugar(ctx, &p->subpats[i]);
  }
}

static bool pat_has_or(const StigiPat *p) {
  if (p->kind == STIGI_PAT_OR) return true;
  for (uint32_t i = 0; i < p->subpat_count; i++) {
    if (pat_has_or(&p->subpats[i])) return true;
  }
  return false;
}

/* Every OR-free form of p, in source order: alternatives multiply out through constructors and tuples
   (Maranget 2008: an or-pattern is a set of rows). Arena-allocated. */
static StigiPat *pat_expand(ElabCtx *ctx, const StigiPat *p, uint32_t *out_n) {
  *out_n = 0;
  if (p->kind == STIGI_PAT_OR) {
    StigiPat *acc = NULL;
    uint32_t total = 0;
    for (uint32_t i = 0; i < p->subpat_count; i++) {
      /* an alternative with other binders was reported (pattern-arity) and adds no row */
      if (i > 0 && !same_binders(&p->subpats[0], &p->subpats[i])) {
        ctx->arity_dropped = true;
        continue;
      }
      uint32_t k = 0;
      StigiPat *ex = pat_expand(ctx, &p->subpats[i], &k);
      if (ex == NULL) return NULL;
      StigiPat *grown = ea_alloc(ctx, (size_t)(total + k) * sizeof *grown);
      if (grown == NULL) return NULL;
      if (total > 0) memcpy(grown, acc, (size_t)total * sizeof *grown);
      memcpy(grown + total, ex, (size_t)k * sizeof *grown);
      acc = grown;
      total += k;
    }
    *out_n = total;
    return acc;
  }
  if ((p->kind == STIGI_PAT_CTOR || p->kind == STIGI_PAT_TUPLE) && p->subpat_count > 0 && pat_has_or(p)) {
    uint32_t ns = p->subpat_count;
    StigiPat **opts = malloc((size_t)ns * sizeof *opts);
    uint32_t *cnt = malloc((size_t)ns * sizeof *cnt);
    if (opts == NULL || cnt == NULL) {
      free(opts);
      free(cnt);
      ctx->err = diag_set(ctx->diag, STIGI_ERR_OOM, -1, "pattern OOM");
      return NULL;
    }
    uint32_t total = 1;
    for (uint32_t i = 0; i < ns; i++) {
      opts[i] = pat_expand(ctx, &p->subpats[i], &cnt[i]);
      total *= cnt[i];
    }
    StigiPat *res = ea_alloc(ctx, (size_t)total * sizeof *res);
    for (uint32_t r = 0; res != NULL && r < total; r++) {
      res[r] = *p;
      res[r].subpats = ea_alloc(ctx, (size_t)ns * sizeof *res[r].subpats);
      if (res[r].subpats == NULL) break;
      uint32_t rem = r;
      bool flat = true;
      for (uint32_t i = ns; i > 0; i--) {
        res[r].subpats[i - 1U] = opts[i - 1U][rem % cnt[i - 1U]];
        rem /= cnt[i - 1U];
        flat = flat && pat_is_wild_or_var(&res[r].subpats[i - 1U]);
      }
      res[r].fields = NULL;
      res[r].field_count = 0;
      if (flat && p->kind == STIGI_PAT_CTOR) {
        /* keep the flat field list a written `C(a, b)` has (parse.c), so the 1-level IR is the same */
        res[r].fields = ea_alloc(ctx, (size_t)ns * sizeof *res[r].fields);
        if (res[r].fields != NULL) {
          for (uint32_t i = 0; i < ns; i++) res[r].fields[i] = res[r].subpats[i].name;
          res[r].field_count = ns;
        }
      }
    }
    free(opts);
    free(cnt);
    *out_n = res != NULL ? total : 0;
    return res;
  }
  StigiPat *one = ea_alloc(ctx, sizeof *one);
  if (one == NULL) return NULL;
  *one = *p;
  *out_n = 1;
  return one;
}

/* ---- Maranget (2007/2008) Pattern Matrix Decision-Tree Compiler ---- */

static StigiExpr *compile_matrix(ElabCtx *ctx, StigiExpr *const *scruts, uint32_t ncols,
                                 const PatRow *rows, uint32_t nrows,
                                 StigiExpr *fallback, bool *arm_reached,
                                 StigiSpan match_span, const char *witness_ctx, int depth) {
  if (ctx->err != STIGI_OK) return make_unit_expr(ctx, match_span);
  if (depth > 64) {
    elab_add_diag(ctx, STIGI_TYPE_MISMATCH, match_span, "pattern decision tree exceeded max depth");
    return make_unit_expr(ctx, match_span);
  }

  /* 1. Empty matrix -> fallback or non-exhaustive with concrete witness */
  if (nrows == 0) {
    if (fallback != NULL) return fallback;
    if (!ctx->arity_dropped) {
      elab_add_diag(ctx, STIGI_TYPE_NOT_EXHAUSTIVE, match_span,
                    "non-exhaustive match: missing case '%s'",
                    witness_ctx != NULL && witness_ctx[0] != '\0' ? witness_ctx : "_");
    }
    /* the match is rejected; the stand-in has the match's type so no second code follows */
    return ctx->missing_body != NULL ? ctx->missing_body : make_unit_expr(ctx, match_span);
  }

  /* 2. Check if row 0 is all wildcards/variables */
  bool row0_default = true;
  for (uint32_t c = 0; c < ncols; c++) {
    if (!pat_is_wild_or_var(&rows[0].pats[c])) {
      row0_default = false;
      break;
    }
  }
  if (row0_default) {
    if (arm_reached != NULL) arm_reached[rows[0].arm_idx] = true;
    uint32_t total_b = rows[0].nbinds + ncols;
    PatBind *all_b = total_b > 0 ? malloc((size_t)total_b * sizeof *all_b) : NULL;
    if (total_b > 0 && all_b == NULL) {
      ctx->err = diag_set(ctx->diag, STIGI_ERR_OOM, -1, "pattern bind OOM");
      return make_unit_expr(ctx, match_span);
    }
    uint32_t bc = 0;
    for (uint32_t i = 0; i < rows[0].nbinds; i++) all_b[bc++] = rows[0].binds[i];
    for (uint32_t c = 0; c < ncols; c++) {
      if (rows[0].pats[c].kind == STIGI_PAT_VAR) {
        all_b[bc].name = rows[0].pats[c].name;
        all_b[bc].rhs = scruts[c];
        all_b[bc].span = rows[0].pats[c].span;
        bc++;
      }
    }
    StigiExpr *leaf = rows[0].body;
    if (rows[0].guard != NULL) {
      StigiExpr *else_br = compile_matrix(ctx, scruts, ncols, rows + 1, nrows - 1,
                                          fallback, arm_reached, match_span, witness_ctx,
                                          depth + 1);
      leaf = make_if_expr(ctx, rows[0].guard, rows[0].body, else_br, rows[0].span);
    }
    StigiExpr *res = wrap_bindings(ctx, all_b, bc, leaf);
    free(all_b);
    return res;
  }

  /* 3. Column selection (Maranget 2008: first column with a non-wildcard in row 0, else first active col) */
  uint32_t col = 0;
  bool found_col = false;
  for (uint32_t c = 0; c < ncols; c++) {
    if (!pat_is_wild_or_var(&rows[0].pats[c])) {
      col = c;
      found_col = true;
      break;
    }
  }
  if (!found_col) {
    for (uint32_t c = 0; c < ncols && !found_col; c++) {
      for (uint32_t r = 0; r < nrows; r++) {
        if (!pat_is_wild_or_var(&rows[r].pats[c])) {
          col = c;
          found_col = true;
          break;
        }
      }
    }
  }

  /* Determine head constructor kind in column `col` */
  StigiPatKind head_kind = STIGI_PAT_WILD;
  const StigiPat *first_non_wild = NULL;
  for (uint32_t r = 0; r < nrows; r++) {
    if (!pat_is_wild_or_var(&rows[r].pats[col])) {
      head_kind = rows[r].pats[col].kind;
      first_non_wild = &rows[r].pats[col];
      break;
    }
  }

  /* Case A: Tuple pattern in column (unpack inline) */
  if (head_kind == STIGI_PAT_TUPLE && first_non_wild != NULL) {
    elab_add_diag(ctx, STIGI_TYPE_PATTERN_ARITY, first_non_wild->span,
                  "unexpected tuple pattern for non-tuple scrutinee");
    return make_unit_expr(ctx, match_span);
  }

  /* Case B: Enum constructors */
  if (head_kind == STIGI_PAT_CTOR && first_non_wild != NULL) {
    uint32_t dummy_cidx = 0;
    bool ambig = false;
    int32_t eidx = stigi_image_resolve_enum_case(ctx->img, ctx->uidx,
                                                 first_non_wild->name.ptr,
                                                 first_non_wild->name.len,
                                                 &dummy_cidx, &ambig);
    if (eidx < 0 || ambig) {
      /* Builtin allocation enums (ArrayFillResult / ArrayGrowResult / CellMakeResult) or unknown */
      const char *b_cases[2] = {NULL, NULL};
      uint32_t b_arities[2] = {0, 0};
      uint32_t b_cnt = 0;
      StigiStr sn = case_short_name(first_non_wild->name);
      if ((sn.len == 6 && memcmp(sn.ptr, "Filled", 6) == 0) ||
          (sn.len == 10 && memcmp(sn.ptr, "FillFailed", 10) == 0)) {
        b_cases[0] = "Filled"; b_arities[0] = 1;
        b_cases[1] = "FillFailed"; b_arities[1] = 0;
        b_cnt = 2;
      } else if ((sn.len == 5 && memcmp(sn.ptr, "Grown", 5) == 0) ||
                 (sn.len == 10 && memcmp(sn.ptr, "GrowFailed", 10) == 0)) {
        b_cases[0] = "Grown"; b_arities[0] = 1;
        b_cases[1] = "GrowFailed"; b_arities[1] = 1;
        b_cnt = 2;
      } else if ((sn.len == 4 && memcmp(sn.ptr, "Made", 4) == 0) ||
                 (sn.len == 10 && memcmp(sn.ptr, "MakeFailed", 10) == 0)) {
        b_cases[0] = "Made"; b_arities[0] = 1;
        b_cases[1] = "MakeFailed"; b_arities[1] = 0;
        b_cnt = 2;
      }
      if (b_cnt == 0) {
        elab_add_diag(ctx, STIGI_TYPE_MISMATCH, first_non_wild->span,
                      "cannot resolve enum constructor '%.*s'",
                      (int)first_non_wild->name.len, first_non_wild->name.ptr);
        return make_unit_expr(ctx, match_span);
      }

      StigiMatchArm *out_arms = ea_alloc(ctx, (size_t)b_cnt * sizeof *out_arms);
      if (out_arms == NULL) return make_unit_expr(ctx, match_span);
      memset(out_arms, 0, (size_t)b_cnt * sizeof *out_arms);
      for (uint32_t c = 0; c < b_cnt; c++) {
        uint32_t arity = b_arities[c];
        StigiStr cname = {b_cases[c], (uint32_t)strlen(b_cases[c])};
        StigiStr f0 = arity > 0 ? ea_fmt_str(ctx, "patField%u", ctx->fresh_seq++) : (StigiStr){"_", 1};
        uint32_t next_ncols = ncols - 1U + arity;
        StigiExpr **next_scruts = next_ncols > 0 ? malloc((size_t)next_ncols * sizeof *next_scruts) : NULL;
        PatRow *sub_rows = malloc((size_t)nrows * sizeof *sub_rows);
        if ((next_ncols > 0 && next_scruts == NULL) || sub_rows == NULL) {
          free(next_scruts); free(sub_rows);
          ctx->err = diag_set(ctx->diag, STIGI_ERR_OOM, -1, "pattern OOM");
          return make_unit_expr(ctx, match_span);
        }
        uint32_t sc_w = 0;
        for (uint32_t j = 0; j < ncols; j++) {
          if (j == col) {
            if (arity == 1) next_scruts[sc_w++] = make_local_expr(ctx, f0, match_span);
          } else {
            next_scruts[sc_w++] = scruts[j];
          }
        }
        uint32_t sub_n = 0;
        bool f0_used = false;
        for (uint32_t r = 0; r < nrows; r++) {
          const StigiPat *rp = &rows[r].pats[col];
          bool match_c = false;
          bool is_def = pat_is_wild_or_var(rp);
          if (!is_def && rp->kind == STIGI_PAT_CTOR &&
              str_eq_ss(case_short_name(rp->name), cname)) {
            match_c = true;
            uint32_t got_a = rp->subpat_count > 0 ? rp->subpat_count : rp->field_count;
            if (got_a != arity) {
              elab_add_diag(ctx, STIGI_TYPE_PATTERN_ARITY, rp->span,
                            "case '%s' expects %u fields, got %u", b_cases[c], arity, got_a);
            }
          }
          if (!match_c && !is_def) continue;
          PatRow nr = rows[r];
          nr.ncols = next_ncols;
          nr.pats = next_ncols > 0 ? malloc((size_t)next_ncols * sizeof *nr.pats) : NULL;
          nr.binds = (rows[r].nbinds + 1U) > 0
                         ? malloc((size_t)(rows[r].nbinds + 1U) * sizeof *nr.binds)
                         : NULL;
          nr.nbinds = rows[r].nbinds;
          if (rows[r].nbinds > 0 && nr.binds != NULL) {
            memcpy(nr.binds, rows[r].binds, (size_t)rows[r].nbinds * sizeof *nr.binds);
          }
          if (is_def && rp->kind == STIGI_PAT_VAR && nr.binds != NULL) {
            nr.binds[nr.nbinds++] = (PatBind){rp->name, scruts[col], rp->span};
          }
          uint32_t pw = 0;
          for (uint32_t j = 0; j < ncols; j++) {
            if (j == col) {
              if (arity == 1) {
                if (match_c && rp->subpat_count == 1) {
                  nr.pats[pw] = rp->subpats[0];
                  if (rp->subpats[0].kind != STIGI_PAT_WILD) f0_used = true;
                } else {
                  memset(&nr.pats[pw], 0, sizeof nr.pats[pw]);
                  nr.pats[pw].kind = STIGI_PAT_WILD;
                  nr.pats[pw].name = (StigiStr){"_", 1};
                }
                pw++;
              }
            } else {
              nr.pats[pw++] = rows[r].pats[j];
            }
          }
          sub_rows[sub_n++] = nr;
        }
        char wbuf[128];
        (void)snprintf(wbuf, sizeof wbuf, "%s%s", b_cases[c], arity > 0 ? "(_)" : "");
        StigiExpr *arm_body = compile_matrix(ctx, next_scruts, next_ncols, sub_rows, sub_n,
                                             fallback, arm_reached, match_span, wbuf, depth + 1);
        for (uint32_t r = 0; r < sub_n; r++) { free(sub_rows[r].pats); free(sub_rows[r].binds); }
        free(sub_rows);
        free(next_scruts);

        out_arms[c].span = match_span;
        out_arms[c].pat.kind = STIGI_PAT_CTOR;
        out_arms[c].pat.span = match_span;
        out_arms[c].pat.name = cname;
        out_arms[c].pat.has_parens = (arity > 0);
        if (arity == 1) {
          out_arms[c].pat.fields = ea_alloc(ctx, sizeof *out_arms[c].pat.fields);
          if (out_arms[c].pat.fields != NULL) {
            out_arms[c].pat.fields[0] = f0_used ? f0 : (StigiStr){"_", 1};
            out_arms[c].pat.field_count = 1;
          }
        }
        out_arms[c].body = arm_body;
      }
      StigiExpr *m = ea_alloc(ctx, sizeof *m);
      if (m == NULL) return make_unit_expr(ctx, match_span);
      memset(m, 0, sizeof *m);
      m->kind = STIGI_EXPR_MATCH;
      m->span = match_span;
      m->u.match_expr.scrutinee = scruts[col];
      m->u.match_expr.arms = out_arms;
      m->u.match_expr.arm_count = b_cnt;
      return m;
    }

    const StigiDecl *ed = ctx->img->units[eidx].ast->decl;
    uint32_t ncases = ed->u.enum_decl.case_count;

    StigiMatchArm *out_arms = ea_alloc(ctx, (size_t)ncases * sizeof *out_arms);
    if (out_arms == NULL) return make_unit_expr(ctx, match_span);
    memset(out_arms, 0, (size_t)ncases * sizeof *out_arms);

    for (uint32_t c = 0; c < ncases; c++) {
      const StigiEnumCase *ec = &ed->u.enum_decl.cases[c];
      uint32_t arity = ec->field_count;
      StigiStr *fnames = arity > 0 ? malloc((size_t)arity * sizeof *fnames) : NULL;
      bool *fused = arity > 0 ? calloc(arity, sizeof *fused) : NULL;
      if (arity > 0 && (fnames == NULL || fused == NULL)) {
        free(fnames); free(fused);
        ctx->err = diag_set(ctx->diag, STIGI_ERR_OOM, -1, "pattern field OOM");
        return make_unit_expr(ctx, match_span);
      }
      for (uint32_t f = 0; f < arity; f++) {
        fnames[f] = ea_fmt_str(ctx, "patField%u", ctx->fresh_seq++);
      }

      uint32_t next_ncols = ncols - 1U + arity;
      StigiExpr **next_scruts = next_ncols > 0 ? malloc((size_t)next_ncols * sizeof *next_scruts) : NULL;
      PatRow *sub_rows = malloc((size_t)nrows * sizeof *sub_rows);
      if ((next_ncols > 0 && next_scruts == NULL) || sub_rows == NULL) {
        free(fnames); free(fused); free(next_scruts); free(sub_rows);
        ctx->err = diag_set(ctx->diag, STIGI_ERR_OOM, -1, "pattern OOM");
        return make_unit_expr(ctx, match_span);
      }
      uint32_t sc_w = 0;
      for (uint32_t j = 0; j < ncols; j++) {
        if (j == col) {
          for (uint32_t f = 0; f < arity; f++) {
            next_scruts[sc_w++] = make_local_expr(ctx, fnames[f], match_span);
          }
        } else {
          next_scruts[sc_w++] = scruts[j];
        }
      }

      uint32_t sub_n = 0;
      for (uint32_t r = 0; r < nrows; r++) {
        const StigiPat *rp = &rows[r].pats[col];
        bool match_c = false;
        bool is_def = pat_is_wild_or_var(rp);
        if (!is_def && rp->kind == STIGI_PAT_CTOR &&
            str_eq_ss(case_short_name(rp->name), ec->name)) {
          match_c = true;
          uint32_t got_a = rp->subpat_count > 0 ? rp->subpat_count : rp->field_count;
          if (got_a != arity) {
            elab_add_diag(ctx, STIGI_TYPE_PATTERN_ARITY, rp->span,
                          "case '%.*s' expects %u fields, got %u",
                          (int)ec->name.len, ec->name.ptr, arity, got_a);
          }
        }
        if (!match_c && !is_def) continue;

        PatRow nr = rows[r];
        nr.ncols = next_ncols;
        nr.pats = next_ncols > 0 ? malloc((size_t)next_ncols * sizeof *nr.pats) : NULL;
        nr.binds = malloc((size_t)(rows[r].nbinds + 1U) * sizeof *nr.binds);
        nr.nbinds = rows[r].nbinds;
        if (rows[r].nbinds > 0 && nr.binds != NULL) {
          memcpy(nr.binds, rows[r].binds, (size_t)rows[r].nbinds * sizeof *nr.binds);
        }
        if (is_def && rp->kind == STIGI_PAT_VAR && nr.binds != NULL) {
          nr.binds[nr.nbinds++] = (PatBind){rp->name, scruts[col], rp->span};
        }
        uint32_t pw = 0;
        for (uint32_t j = 0; j < ncols; j++) {
          if (j == col) {
            for (uint32_t f = 0; f < arity; f++) {
              if (match_c && f < rp->subpat_count) {
                nr.pats[pw] = rp->subpats[f];
                if (rp->subpats[f].kind != STIGI_PAT_WILD) fused[f] = true;
              } else if (match_c && f < rp->field_count) {
                memset(&nr.pats[pw], 0, sizeof nr.pats[pw]);
                bool is_w = (rp->fields[f].len == 1 && rp->fields[f].ptr[0] == '_');
                nr.pats[pw].kind = is_w ? STIGI_PAT_WILD : STIGI_PAT_VAR;
                nr.pats[pw].name = rp->fields[f];
                nr.pats[pw].span = rp->span;
                if (!is_w) fused[f] = true;
              } else {
                memset(&nr.pats[pw], 0, sizeof nr.pats[pw]);
                nr.pats[pw].kind = STIGI_PAT_WILD;
                nr.pats[pw].name = (StigiStr){"_", 1};
              }
              pw++;
            }
          } else {
            nr.pats[pw++] = rows[r].pats[j];
          }
        }
        sub_rows[sub_n++] = nr;
      }

      char wbuf[192];
      if (witness_ctx != NULL && witness_ctx[0] != '\0') {
        (void)snprintf(wbuf, sizeof wbuf, "%s -> %.*s%s", witness_ctx,
                       (int)ec->name.len, ec->name.ptr, arity > 0 ? "(..)" : "");
      } else {
        (void)snprintf(wbuf, sizeof wbuf, "%.*s%s",
                       (int)ec->name.len, ec->name.ptr, arity > 0 ? "(..)" : "");
      }

      StigiExpr *arm_body = compile_matrix(ctx, next_scruts, next_ncols, sub_rows, sub_n,
                                           fallback, arm_reached, match_span, wbuf, depth + 1);
      for (uint32_t r = 0; r < sub_n; r++) { free(sub_rows[r].pats); free(sub_rows[r].binds); }
      free(sub_rows);
      free(next_scruts);

      /* Build canonical qualified Case name `EnumName.CaseName` to avoid any ambiguity */
      size_t qlen = ctx->img->units[eidx].name_len + 1U + ec->name.len;
      char *qname = ea_alloc(ctx, qlen + 1U);
      if (qname != NULL) {
        memcpy(qname, ctx->img->units[eidx].name, ctx->img->units[eidx].name_len);
        qname[ctx->img->units[eidx].name_len] = '.';
        memcpy(qname + ctx->img->units[eidx].name_len + 1U, ec->name.ptr, ec->name.len);
        qname[qlen] = '\0';
      }
      out_arms[c].span = match_span;
      out_arms[c].pat.kind = STIGI_PAT_CTOR;
      out_arms[c].pat.span = match_span;
      out_arms[c].pat.name = qname != NULL ? (StigiStr){qname, (uint32_t)qlen} : ec->name;
      out_arms[c].pat.has_parens = (arity > 0);
      if (arity > 0) {
        out_arms[c].pat.fields = ea_alloc(ctx, (size_t)arity * sizeof *out_arms[c].pat.fields);
        if (out_arms[c].pat.fields != NULL) {
          for (uint32_t f = 0; f < arity; f++) {
            out_arms[c].pat.fields[f] = fused[f] ? fnames[f] : (StigiStr){"_", 1};
          }
          out_arms[c].pat.field_count = arity;
        }
      }
      out_arms[c].body = arm_body;
      free(fnames);
      free(fused);
    }

    StigiExpr *m = ea_alloc(ctx, sizeof *m);
    if (m == NULL) return make_unit_expr(ctx, match_span);
    memset(m, 0, sizeof *m);
    m->kind = STIGI_EXPR_MATCH;
    m->span = match_span;
    m->u.match_expr.scrutinee = scruts[col];
    m->u.match_expr.arms = out_arms;
    m->u.match_expr.arm_count = ncases;
    return m;
  }

  /* Case C: Bool literals (`true` / `false`) -> `if scrut then true_br else false_br` */
  if (head_kind == STIGI_PAT_BOOL) {
    uint32_t next_ncols = ncols - 1U;
    StigiExpr **next_scruts = next_ncols > 0 ? malloc((size_t)next_ncols * sizeof *next_scruts) : NULL;
    if (next_ncols > 0 && next_scruts == NULL) {
      ctx->err = diag_set(ctx->diag, STIGI_ERR_OOM, -1, "pattern OOM");
      return make_unit_expr(ctx, match_span);
    }
    for (uint32_t j = 0, w = 0; j < ncols; j++) {
      if (j != col) next_scruts[w++] = scruts[j];
    }
    StigiExpr *branches[2] = {NULL, NULL};
    for (int bv = 1; bv >= 0; bv--) {
      bool want = (bv == 1);
      PatRow *sub_rows = malloc((size_t)nrows * sizeof *sub_rows);
      if (sub_rows == NULL) {
        free(next_scruts);
        ctx->err = diag_set(ctx->diag, STIGI_ERR_OOM, -1, "pattern OOM");
        return make_unit_expr(ctx, match_span);
      }
      uint32_t sub_n = 0;
      for (uint32_t r = 0; r < nrows; r++) {
        const StigiPat *rp = &rows[r].pats[col];
        bool is_def = pat_is_wild_or_var(rp);
        bool match_b = (!is_def && rp->kind == STIGI_PAT_BOOL && rp->bool_val == want);
        if (!match_b && !is_def) continue;
        PatRow nr = rows[r];
        nr.ncols = next_ncols;
        nr.pats = next_ncols > 0 ? malloc((size_t)next_ncols * sizeof *nr.pats) : NULL;
        nr.binds = malloc((size_t)(rows[r].nbinds + 1U) * sizeof *nr.binds);
        nr.nbinds = rows[r].nbinds;
        if (rows[r].nbinds > 0 && nr.binds != NULL) {
          memcpy(nr.binds, rows[r].binds, (size_t)rows[r].nbinds * sizeof *nr.binds);
        }
        if (is_def && rp->kind == STIGI_PAT_VAR && nr.binds != NULL) {
          nr.binds[nr.nbinds++] = (PatBind){rp->name, scruts[col], rp->span};
        }
        for (uint32_t j = 0, pw = 0; j < ncols; j++) {
          if (j != col) nr.pats[pw++] = rows[r].pats[j];
        }
        sub_rows[sub_n++] = nr;
      }
      branches[bv] = compile_matrix(ctx, next_scruts, next_ncols, sub_rows, sub_n,
                                    fallback, arm_reached, match_span,
                                    want ? "true" : "false", depth + 1);
      for (uint32_t r = 0; r < sub_n; r++) { free(sub_rows[r].pats); free(sub_rows[r].binds); }
      free(sub_rows);
    }
    free(next_scruts);
    return make_if_expr(ctx, scruts[col], branches[1], branches[0], match_span);
  }

  /* Case R (S129): a column with a range or a text pattern is compiled by backtracking on row 0 (Maranget 2008,
     sec. 4): test row 0's pattern; the true side keeps row 0 with that column matched and every later row with
     its own test, the false side drops row 0. */
  bool backtrack = false;
  for (uint32_t r = 0; r < nrows && !backtrack; r++) {
    backtrack = rows[r].pats[col].kind == STIGI_PAT_RANGE || rows[r].pats[col].kind == STIGI_PAT_STR;
  }
  if (backtrack && (head_kind == STIGI_PAT_INT || head_kind == STIGI_PAT_CHAR ||
                    head_kind == STIGI_PAT_RANGE || head_kind == STIGI_PAT_STR)) {
    const StigiPat *p0 = &rows[0].pats[col];
    PatRow *then_rows = malloc((size_t)nrows * sizeof *then_rows);
    StigiPat *row0_pats = malloc((size_t)ncols * sizeof *row0_pats);
    if (then_rows == NULL || row0_pats == NULL) {
      free(then_rows);
      free(row0_pats);
      ctx->err = diag_set(ctx->diag, STIGI_ERR_OOM, -1, "pattern OOM");
      return make_unit_expr(ctx, match_span);
    }
    memcpy(then_rows, rows, (size_t)nrows * sizeof *then_rows);
    memcpy(row0_pats, rows[0].pats, (size_t)ncols * sizeof *row0_pats);
    memset(&row0_pats[col], 0, sizeof row0_pats[col]);
    row0_pats[col].kind = STIGI_PAT_WILD;
    row0_pats[col].name = (StigiStr){"_", 1};
    then_rows[0].pats = row0_pats;
    /* Which sides exist: a range over the whole type has no false side; a text whose declared length differs from
       the scrutinee's literal length has no true side. Only existing sides are compiled (never-reached and
       not-exhaustive stay exact). */
    TextPlan tplan = p0->kind == STIGI_PAT_STR ? text_plan(ctx, p0, scruts[col]) : TEXT_TEST_LENGTH;
    StigiExpr *cond = NULL;
    if (p0->kind == STIGI_PAT_RANGE) cond = make_range_test(ctx, p0, scruts[col]);
    if (p0->kind == STIGI_PAT_INT || p0->kind == STIGI_PAT_CHAR) cond = make_eq_lit(ctx, scruts[col], p0, p0->span);
    bool has_yes = tplan != TEXT_NEVER && tplan != TEXT_BAD_SCRUTINEE;
    bool has_no = p0->kind == STIGI_PAT_STR || cond != NULL;
    StigiExpr *yes = has_yes ? compile_matrix(ctx, scruts, ncols, then_rows, nrows, fallback, arm_reached,
                                              match_span, witness_ctx, depth + 1)
                             : NULL;
    StigiExpr *no = has_no ? compile_matrix(ctx, scruts, ncols, rows + 1, nrows - 1U, fallback, arm_reached,
                                            match_span, "_", depth + 1)
                           : NULL;
    free(row0_pats);
    free(then_rows);
    if (p0->kind == STIGI_PAT_STR) {
      return has_yes ? make_text_test(ctx, p0, scruts[col], tplan == TEXT_TEST_LENGTH, yes, no) : no;
    }
    return cond == NULL ? yes : make_if_expr(ctx, cond, yes, no, p0->span);
  }

  /* Case D: Int / negative Int / Char literals -> clean `if x == L then ... else ...` chain */
  if (head_kind == STIGI_PAT_INT || head_kind == STIGI_PAT_CHAR) {
    uint32_t next_ncols = ncols - 1U;
    StigiExpr **next_scruts = next_ncols > 0 ? malloc((size_t)next_ncols * sizeof *next_scruts) : NULL;
    const StigiPat **lits = malloc((size_t)nrows * sizeof *lits);
    if ((next_ncols > 0 && next_scruts == NULL) || lits == NULL) {
      free(next_scruts); free(lits);
      ctx->err = diag_set(ctx->diag, STIGI_ERR_OOM, -1, "pattern OOM");
      return make_unit_expr(ctx, match_span);
    }
    for (uint32_t j = 0, w = 0; j < ncols; j++) {
      if (j != col) next_scruts[w++] = scruts[j];
    }
    uint32_t nlits = 0;
    for (uint32_t r = 0; r < nrows; r++) {
      const StigiPat *rp = &rows[r].pats[col];
      if (rp->kind != STIGI_PAT_INT && rp->kind != STIGI_PAT_CHAR) continue;
      bool dup = false;
      for (uint32_t k = 0; k < nlits; k++) {
        if (lit_pats_equal(lits[k], rp)) { dup = true; break; }
      }
      if (!dup) lits[nlits++] = rp;
    }

    /* Default matrix D(M) (rows with WILD/VAR at col) */
    PatRow *def_rows = malloc((size_t)nrows * sizeof *def_rows);
    uint32_t def_n = 0;
    if (def_rows == NULL) {
      free(next_scruts); free(lits);
      ctx->err = diag_set(ctx->diag, STIGI_ERR_OOM, -1, "pattern OOM");
      return make_unit_expr(ctx, match_span);
    }
    for (uint32_t r = 0; r < nrows; r++) {
      const StigiPat *rp = &rows[r].pats[col];
      if (!pat_is_wild_or_var(rp)) continue;
      PatRow nr = rows[r];
      nr.ncols = next_ncols;
      nr.pats = next_ncols > 0 ? malloc((size_t)next_ncols * sizeof *nr.pats) : NULL;
      nr.binds = malloc((size_t)(rows[r].nbinds + 1U) * sizeof *nr.binds);
      nr.nbinds = rows[r].nbinds;
      if (rows[r].nbinds > 0 && nr.binds != NULL) {
        memcpy(nr.binds, rows[r].binds, (size_t)rows[r].nbinds * sizeof *nr.binds);
      }
      if (rp->kind == STIGI_PAT_VAR && nr.binds != NULL) {
        nr.binds[nr.nbinds++] = (PatBind){rp->name, scruts[col], rp->span};
      }
      for (uint32_t j = 0, pw = 0; j < ncols; j++) {
        if (j != col) nr.pats[pw++] = rows[r].pats[j];
      }
      def_rows[def_n++] = nr;
    }
    StigiExpr *chain = compile_matrix(ctx, next_scruts, next_ncols, def_rows, def_n,
                                      fallback, arm_reached, match_span, "_", depth + 1);
    for (uint32_t r = 0; r < def_n; r++) { free(def_rows[r].pats); free(def_rows[r].binds); }
    free(def_rows);

    for (uint32_t k = nlits; k > 0; k--) {
      const StigiPat *lp = lits[k - 1];
      PatRow *sub_rows = malloc((size_t)nrows * sizeof *sub_rows);
      if (sub_rows == NULL) {
        free(next_scruts); free(lits);
        ctx->err = diag_set(ctx->diag, STIGI_ERR_OOM, -1, "pattern OOM");
        return make_unit_expr(ctx, match_span);
      }
      uint32_t sub_n = 0;
      for (uint32_t r = 0; r < nrows; r++) {
        const StigiPat *rp = &rows[r].pats[col];
        bool is_def = pat_is_wild_or_var(rp);
        bool match_l = (!is_def && lit_pats_equal(rp, lp));
        if (!match_l && !is_def) continue;
        PatRow nr = rows[r];
        nr.ncols = next_ncols;
        nr.pats = next_ncols > 0 ? malloc((size_t)next_ncols * sizeof *nr.pats) : NULL;
        nr.binds = malloc((size_t)(rows[r].nbinds + 1U) * sizeof *nr.binds);
        nr.nbinds = rows[r].nbinds;
        if (rows[r].nbinds > 0 && nr.binds != NULL) {
          memcpy(nr.binds, rows[r].binds, (size_t)rows[r].nbinds * sizeof *nr.binds);
        }
        if (is_def && rp->kind == STIGI_PAT_VAR && nr.binds != NULL) {
          nr.binds[nr.nbinds++] = (PatBind){rp->name, scruts[col], rp->span};
        }
        for (uint32_t j = 0, pw = 0; j < ncols; j++) {
          if (j != col) nr.pats[pw++] = rows[r].pats[j];
        }
        sub_rows[sub_n++] = nr;
      }
      StigiExpr *lit_br = compile_matrix(ctx, next_scruts, next_ncols, sub_rows, sub_n,
                                         chain, arm_reached, match_span, "_", depth + 1);
      for (uint32_t r = 0; r < sub_n; r++) { free(sub_rows[r].pats); free(sub_rows[r].binds); }
      free(sub_rows);
      StigiExpr *cond = make_eq_lit(ctx, scruts[col], lp, lp->span);
      chain = make_if_expr(ctx, cond, lit_br, chain, lp->span);
    }

    free(next_scruts);
    free(lits);
    return chain;
  }

  return make_unit_expr(ctx, match_span);
}

static StigiExpr *compile_match_core(ElabCtx *ctx, StigiExpr *const *raw_scruts, uint32_t ncols,
                                     const StigiMatchArm *arms, uint32_t narms, const uint32_t *src_arm,
                                     StigiSpan match_span);

/* S129: check the sugar of every arm, then give every alternative its own arm (one row each, same guard and body),
   so `case A | B => e` is exactly `case A => e` and `case B => e`; diagnostics name the arm as written. */
static StigiExpr *compile_match_expr(ElabCtx *ctx, StigiExpr *const *raw_scruts, uint32_t ncols,
                                     const StigiMatchArm *arms, uint32_t narms,
                                     StigiSpan match_span) {
  bool any_or = false;
  for (uint32_t i = 0; i < narms; i++) {
    check_pat_sugar(ctx, &arms[i].pat);
    any_or = any_or || pat_has_or(&arms[i].pat);
  }
  if (!any_or) return compile_match_core(ctx, raw_scruts, ncols, arms, narms, NULL, match_span);
  bool saved_dropped = ctx->arity_dropped; /* nested matches keep their own flag */
  ctx->arity_dropped = false;
  uint32_t total = 0, cap = narms * 2U + 1U;
  StigiMatchArm *xarms = malloc((size_t)cap * sizeof *xarms);
  uint32_t *src = malloc((size_t)cap * sizeof *src);
  bool ok = xarms != NULL && src != NULL;
  for (uint32_t i = 0; ok && i < narms; i++) {
    uint32_t k = 0;
    StigiPat *ex = pat_expand(ctx, &arms[i].pat, &k);
    ok = ex != NULL;
    if (ok && total + k > cap) {
      cap = (total + k) * 2U;
      StigiMatchArm *gx = realloc(xarms, (size_t)cap * sizeof *xarms);
      if (gx != NULL) xarms = gx;
      uint32_t *gs = realloc(src, (size_t)cap * sizeof *src);
      if (gs != NULL) src = gs;
      ok = gx != NULL && gs != NULL;
    }
    for (uint32_t j = 0; ok && j < k; j++) {
      xarms[total] = arms[i];
      xarms[total].pat = ex[j];
      src[total++] = i;
    }
  }
  StigiExpr *res = NULL;
  if (ok) {
    res = compile_match_core(ctx, raw_scruts, ncols, xarms, total, src, match_span);
    if (res != NULL && res->kind == STIGI_EXPR_MATCH && ctx->arity_dropped) res->u.match_expr.arity_rejected = true;
  } else if (ctx->err == STIGI_OK) {
    ctx->err = diag_set(ctx->diag, STIGI_ERR_OOM, -1, "pattern OOM");
  }
  free(xarms);
  free(src);
  ctx->arity_dropped = saved_dropped;
  return res != NULL ? res : make_unit_expr(ctx, match_span);
}

static StigiExpr *compile_match_core(ElabCtx *ctx, StigiExpr *const *raw_scruts, uint32_t ncols,
                                     const StigiMatchArm *arms, uint32_t narms, const uint32_t *src_arm,
                                     StigiSpan match_span) {
  /* Fast path: simple 1-level match without guards/chars/neg-ints/tuples stays 1-level directly */
  if (ncols == 1) {
    bool simple = true;
    for (uint32_t i = 0; i < narms; i++) {
      if (arms[i].guard != NULL || !pat_is_1level_simple(&arms[i].pat)) {
        simple = false;
        break;
      }
    }
    if (simple) {
      /* Still report never-reached on duplicate or subsumed 1-level arms */
      bool seen_catchall = false;
      bool seen_true = false;
      bool seen_false = false;
      for (uint32_t i = 0; i < narms; i++) {
        bool dead = seen_catchall;
        const StigiPat *pi = &arms[i].pat;
        if (!dead && pi->kind == STIGI_PAT_BOOL) {
          if (pi->bool_val ? seen_true : seen_false) dead = true;
          if (pi->bool_val) seen_true = true; else seen_false = true;
          if (seen_true && seen_false) seen_catchall = true;
        } else if (!dead && pi->kind == STIGI_PAT_INT) {
          for (uint32_t j = 0; j < i; j++) {
            if (arms[j].pat.kind == STIGI_PAT_INT &&
                arms[j].pat.int_val == pi->int_val && arms[j].pat.int_neg == pi->int_neg) {
              dead = true;
              break;
            }
          }
        } else if (!dead && pi->kind == STIGI_PAT_CTOR) {
          for (uint32_t j = 0; j < i; j++) {
            if (arms[j].pat.kind == STIGI_PAT_CTOR &&
                str_eq_ss(case_short_name(arms[j].pat.name), case_short_name(pi->name))) {
              dead = true;
              break;
            }
          }
        } else if (!dead && pat_is_wild_or_var(pi)) {
          seen_catchall = true;
        }
        if (dead) {
          elab_add_diag(ctx, STIGI_TYPE_NEVER_REACHED, arms[i].span,
                        "unreachable match arm %u is subsumed by earlier patterns",
                        (src_arm != NULL ? src_arm[i] : i) + 1);
        }
      }
      StigiExpr *m = ea_alloc(ctx, sizeof *m);
      if (m == NULL) return make_unit_expr(ctx, match_span);
      memset(m, 0, sizeof *m);
      m->kind = STIGI_EXPR_MATCH;
      m->span = match_span;
      m->u.match_expr.scrutinee = elab_expr(ctx, raw_scruts[0]);
      m->u.match_expr.arm_count = narms;
      if (narms > 0) {
        m->u.match_expr.arms = ea_alloc(ctx, (size_t)narms * sizeof *m->u.match_expr.arms);
        if (m->u.match_expr.arms == NULL) return make_unit_expr(ctx, match_span);
        for (uint32_t i = 0; i < narms; i++) {
          m->u.match_expr.arms[i] = arms[i];
          m->u.match_expr.arms[i].body = elab_expr(ctx, arms[i].body);
        }
      }
      return m;
    }
  }

  /* General Maranget path: hoist any non-local scrutinee ONCE into a fresh `val` */
  StigiExpr **scruts = malloc((size_t)ncols * sizeof *scruts);
  PatBind *pre_binds = malloc((size_t)ncols * sizeof *pre_binds);
  PatRow *rows = narms > 0 ? malloc((size_t)narms * sizeof *rows) : NULL;
  bool *arm_reached = narms > 0 ? calloc(narms, sizeof *arm_reached) : NULL;
  if (scruts == NULL || pre_binds == NULL || (narms > 0 && (rows == NULL || arm_reached == NULL))) {
    free(scruts); free(pre_binds); free(rows); free(arm_reached);
    ctx->err = diag_set(ctx->diag, STIGI_ERR_OOM, -1, "pattern OOM");
    return make_unit_expr(ctx, match_span);
  }
  uint32_t npre = 0;
  for (uint32_t c = 0; c < ncols; c++) {
    StigiExpr *es = elab_expr(ctx, raw_scruts[c]);
    if (es != NULL && es->kind == STIGI_EXPR_LOCAL) {
      scruts[c] = es;
    } else {
      StigiStr sn = ea_fmt_str(ctx, "patScrut%u", ctx->fresh_seq++);
      pre_binds[npre++] = (PatBind){sn, es, es != NULL ? es->span : match_span};
      scruts[c] = make_local_expr(ctx, sn, es != NULL ? es->span : match_span);
    }
  }

  for (uint32_t i = 0; i < narms; i++) {
    rows[i].ncols = ncols;
    rows[i].pats = malloc((size_t)ncols * sizeof *rows[i].pats);
    rows[i].binds = NULL;
    rows[i].nbinds = 0;
    rows[i].guard = arms[i].guard != NULL ? elab_expr(ctx, arms[i].guard) : NULL;
    rows[i].body = elab_expr(ctx, arms[i].body);
    rows[i].arm_idx = i;
    rows[i].span = arms[i].span;
    if (rows[i].pats == NULL) {
      for (uint32_t k = 0; k < i; k++) free(rows[k].pats);
      free(scruts); free(pre_binds); free(rows); free(arm_reached);
      ctx->err = diag_set(ctx->diag, STIGI_ERR_OOM, -1, "pattern OOM");
      return make_unit_expr(ctx, match_span);
    }
    if (ncols == 1) {
      rows[i].pats[0] = arms[i].pat;
    } else if (arms[i].pat.kind == STIGI_PAT_TUPLE && arms[i].pat.subpat_count == ncols) {
      for (uint32_t c = 0; c < ncols; c++) rows[i].pats[c] = arms[i].pat.subpats[c];
    } else if (arms[i].pat.kind == STIGI_PAT_WILD) {
      for (uint32_t c = 0; c < ncols; c++) rows[i].pats[c] = arms[i].pat;
    } else {
      elab_add_diag(ctx, STIGI_TYPE_PATTERN_ARITY, arms[i].pat.span,
                    "multi-scrutinee match expects a %u-element tuple pattern", ncols);
      for (uint32_t c = 0; c < ncols; c++) {
        memset(&rows[i].pats[c], 0, sizeof rows[i].pats[c]);
        rows[i].pats[c].kind = STIGI_PAT_WILD;
        rows[i].pats[c].name = (StigiStr){"_", 1};
      }
    }
  }

  report_wildcard_rows(ctx, rows, narms, ncols);
  StigiExpr *saved_missing = ctx->missing_body;
  ctx->missing_body = narms > 0 ? rows[0].body : NULL;
  StigiExpr *tree = compile_matrix(ctx, scruts, ncols, rows, narms, NULL, arm_reached,
                                   match_span, "", 0);
  ctx->missing_body = saved_missing;

  for (uint32_t i = 0; i < narms; i++) {
    if (!arm_reached[i]) {
      elab_add_diag(ctx, STIGI_TYPE_NEVER_REACHED, arms[i].span,
                    "unreachable match arm %u is subsumed by earlier patterns",
                        (src_arm != NULL ? src_arm[i] : i) + 1);
    }
    free(rows[i].pats);
  }
  StigiExpr *wrapped = wrap_bindings(ctx, pre_binds, npre, tree);
  free(scruts);
  free(pre_binds);
  free(rows);
  free(arm_reached);
  return wrapped;
}

static StigiExpr *elab_block_tail(ElabCtx *ctx, StigiExpr *const *stmts, uint32_t start,
                                  uint32_t count, StigiSpan blk_span) {
  if (start >= count) return make_unit_expr(ctx, blk_span);
  for (uint32_t i = start; i < count; i++) {
    const StigiExpr *s = stmts[i];
    if (s != NULL && s->kind == STIGI_EXPR_VAL && s->u.val_stmt.else_arm_count > 0) {
      /* S123: CPS desugar `val Pat = init else case ...` wrapping remainder of block */
      StigiExpr *cont = elab_block_tail(ctx, stmts, i + 1, count, blk_span);
      uint32_t narms = 1U + s->u.val_stmt.else_arm_count;
      StigiMatchArm *arms = malloc((size_t)narms * sizeof *arms);
      if (arms == NULL) {
        ctx->err = diag_set(ctx->diag, STIGI_ERR_OOM, -1, "val-else OOM");
        return make_unit_expr(ctx, blk_span);
      }
      memset(&arms[0], 0, sizeof arms[0]);
      arms[0].span = s->span;
      arms[0].pat = s->u.val_stmt.pat;
      arms[0].body = cont;
      for (uint32_t k = 0; k < s->u.val_stmt.else_arm_count; k++) {
        arms[1U + k] = s->u.val_stmt.else_arms[k];
      }
      StigiExpr *init_e = s->u.val_stmt.init;
      StigiExpr *m = compile_match_expr(ctx, &init_e, 1, arms, narms, s->span);
      free(arms);
      if (i == start) return m;
      uint32_t pre_n = i - start;
      StigiExpr *blk = ea_alloc(ctx, sizeof *blk);
      if (blk == NULL) return m;
      memset(blk, 0, sizeof *blk);
      blk->kind = STIGI_EXPR_BLOCK;
      blk->span = stmts[start]->span;
      blk->u.block.stmts = ea_alloc(ctx, (size_t)(pre_n + 1U) * sizeof *blk->u.block.stmts);
      if (blk->u.block.stmts == NULL) return m;
      for (uint32_t k = 0; k < pre_n; k++) {
        blk->u.block.stmts[k] = elab_expr(ctx, stmts[start + k]);
      }
      blk->u.block.stmts[pre_n] = m;
      blk->u.block.count = pre_n + 1U;
      return blk;
    }
  }
  uint32_t n = count - start;
  if (n == 1) return elab_expr(ctx, stmts[start]);
  StigiExpr *blk = ea_alloc(ctx, sizeof *blk);
  if (blk == NULL) return make_unit_expr(ctx, blk_span);
  memset(blk, 0, sizeof *blk);
  blk->kind = STIGI_EXPR_BLOCK;
  blk->span = stmts[start]->span;
  blk->u.block.stmts = ea_alloc(ctx, (size_t)n * sizeof *blk->u.block.stmts);
  if (blk->u.block.stmts == NULL) return make_unit_expr(ctx, blk_span);
  for (uint32_t k = 0; k < n; k++) {
    blk->u.block.stmts[k] = elab_expr(ctx, stmts[start + k]);
  }
  blk->u.block.count = n;
  return blk;
}

/* S129: `x is P` is `x match case P => true` plus `=> false` arms for what P leaves out. Over a closed enum each
   case P does not cover in full gets its own `C(_, ..) => false` arm (no `_` over an enum, T3); otherwise one `_`
   arm. P's binders end with the match: they are not in scope after `is`. */
static StigiExpr *elab_is(ElabCtx *ctx, const StigiExpr *e) {
  const StigiPat *p = &e->u.is_expr.pat;
  uint32_t nalt = 0;
  StigiPat *alts = pat_expand(ctx, p, &nalt);
  if (alts == NULL) return make_unit_expr(ctx, e->span);
  const StigiDecl *ed = NULL;
  const char *ename = NULL;
  for (uint32_t i = 0; i < nalt && ed == NULL; i++) {
    if (alts[i].kind != STIGI_PAT_CTOR) continue;
    uint32_t cidx = 0;
    bool ambig = false;
    int32_t eidx = stigi_image_resolve_enum_case(ctx->img, ctx->uidx, alts[i].name.ptr, alts[i].name.len, &cidx,
                                                 &ambig);
    if (eidx >= 0 && !ambig) {
      ed = ctx->img->units[eidx].ast->decl;
      ename = ctx->img->units[eidx].name;
    }
  }
  uint32_t extra = ed != NULL ? ed->u.enum_decl.case_count : 1U;
  StigiMatchArm *arms = ea_alloc(ctx, (size_t)(1U + extra) * sizeof *arms);
  if (arms == NULL) return make_unit_expr(ctx, e->span);
  memset(arms, 0, (size_t)(1U + extra) * sizeof *arms);
  arms[0].span = e->span;
  arms[0].pat = *p;
  arms[0].body = make_bool_expr(ctx, true, e->span);
  uint32_t n = 1;
  if (ed == NULL) {
    arms[n].span = e->span;
    arms[n].pat.kind = STIGI_PAT_WILD;
    arms[n].pat.span = e->span;
    arms[n].pat.name = (StigiStr){"_", 1};
    arms[n++].body = make_bool_expr(ctx, false, e->span);
  } else {
    for (uint32_t c = 0; c < ed->u.enum_decl.case_count; c++) {
      const StigiEnumCase *ec = &ed->u.enum_decl.cases[c];
      bool full = false;
      for (uint32_t i = 0; i < nalt && !full; i++) {
        if (alts[i].kind != STIGI_PAT_CTOR || !str_eq_ss(case_short_name(alts[i].name), ec->name)) continue;
        full = true;
        for (uint32_t s = 0; s < alts[i].subpat_count && full; s++) full = pat_is_wild_or_var(&alts[i].subpats[s]);
      }
      if (full) continue;
      size_t qlen = strlen(ename) + 1U + ec->name.len;
      char *qname = ea_alloc(ctx, qlen + 1U);
      if (qname == NULL) return make_unit_expr(ctx, e->span);
      (void)snprintf(qname, qlen + 1U, "%s.%.*s", ename, (int)ec->name.len, ec->name.ptr);
      StigiMatchArm *a = &arms[n++];
      a->span = e->span;
      a->pat.kind = STIGI_PAT_CTOR;
      a->pat.span = e->span;
      a->pat.name = (StigiStr){qname, (uint32_t)qlen};
      a->pat.has_parens = ec->field_count > 0;
      if (ec->field_count > 0) {
        a->pat.fields = ea_alloc(ctx, (size_t)ec->field_count * sizeof *a->pat.fields);
        if (a->pat.fields == NULL) return make_unit_expr(ctx, e->span);
        for (uint32_t f = 0; f < ec->field_count; f++) a->pat.fields[f] = (StigiStr){"_", 1};
        a->pat.field_count = ec->field_count;
      }
      a->body = make_bool_expr(ctx, false, e->span);
    }
  }
  StigiExpr *subject = e->u.is_expr.subject;
  return compile_match_expr(ctx, &subject, 1, arms, n, e->span);
}

static StigiExpr *elab_expr(ElabCtx *ctx, const StigiExpr *e) {
  if (e == NULL || ctx->err != STIGI_OK) return NULL;
  switch (e->kind) {
    case STIGI_EXPR_LOCAL:
    case STIGI_EXPR_GLOBAL:
    case STIGI_EXPR_LIT_INT:
    case STIGI_EXPR_LIT_CHAR:
    case STIGI_EXPR_LIT_STR:
    case STIGI_EXPR_LIT_BOOL:
    case STIGI_EXPR_LIT_UNIT:
    case STIGI_EXPR_DENY:
    case STIGI_EXPR_LOCAL_DECL:
      return (StigiExpr *)e;

    case STIGI_EXPR_BINOP:
      if (e->u.binop.op == STIGI_OP_PIPE) {
        /* S123: first-argument functional pipe `lhs |> f(a, b)` -> `f(lhs, a, b)` */
        StigiExpr *lhs = elab_expr(ctx, e->u.binop.lhs);
        StigiExpr *rhs = elab_expr(ctx, e->u.binop.rhs);
        StigiExpr *call = ea_alloc(ctx, sizeof *call);
        if (call == NULL) return lhs;
        memset(call, 0, sizeof *call);
        call->kind = STIGI_EXPR_CALL;
        call->span = e->span;
        call->u.call.has_parens = true;
        if (rhs != NULL && rhs->kind == STIGI_EXPR_CALL) {
          call->u.call.callee = rhs->u.call.callee;
          call->u.call.type_args = rhs->u.call.type_args;
          call->u.call.type_arg_count = rhs->u.call.type_arg_count;
          call->u.call.cap_args = rhs->u.call.cap_args;
          call->u.call.cap_arg_count = rhs->u.call.cap_arg_count;
          uint32_t nac = 1U + rhs->u.call.arg_count;
          call->u.call.args = ea_alloc(ctx, (size_t)nac * sizeof *call->u.call.args);
          if (call->u.call.args == NULL) return lhs;
          call->u.call.args[0] = lhs;
          for (uint32_t i = 0; i < rhs->u.call.arg_count; i++) {
            call->u.call.args[1U + i] = rhs->u.call.args[i];
          }
          call->u.call.arg_count = nac;
        } else {
          call->u.call.callee = rhs;
          call->u.call.args = ea_alloc(ctx, sizeof *call->u.call.args);
          if (call->u.call.args == NULL) return lhs;
          call->u.call.args[0] = lhs;
          call->u.call.arg_count = 1U;
        }
        return call;
      } else {
        StigiExpr *ne = ea_alloc(ctx, sizeof *ne);
        if (ne == NULL) return (StigiExpr *)e;
        *ne = *e;
        ne->u.binop.lhs = elab_expr(ctx, e->u.binop.lhs);
        ne->u.binop.rhs = elab_expr(ctx, e->u.binop.rhs);
        return ne;
      }

    case STIGI_EXPR_UNOP: {
      StigiExpr *ne = ea_alloc(ctx, sizeof *ne);
      if (ne == NULL) return (StigiExpr *)e;
      *ne = *e;
      ne->u.unop.operand = elab_expr(ctx, e->u.unop.operand);
      return ne;
    }

    case STIGI_EXPR_CALL: {
      StigiExpr *ne = ea_alloc(ctx, sizeof *ne);
      if (ne == NULL) return (StigiExpr *)e;
      *ne = *e;
      ne->u.call.callee = elab_expr(ctx, e->u.call.callee);
      if (e->u.call.arg_count > 0) {
        ne->u.call.args = ea_alloc(ctx, (size_t)e->u.call.arg_count * sizeof *ne->u.call.args);
        if (ne->u.call.args == NULL) return (StigiExpr *)e;
        for (uint32_t i = 0; i < e->u.call.arg_count; i++) {
          ne->u.call.args[i] = elab_expr(ctx, e->u.call.args[i]);
        }
      }
      return ne;
    }

    case STIGI_EXPR_IF: {
      StigiExpr *ne = ea_alloc(ctx, sizeof *ne);
      if (ne == NULL) return (StigiExpr *)e;
      *ne = *e;
      ne->u.if_expr.cond = elab_expr(ctx, e->u.if_expr.cond);
      ne->u.if_expr.then_br = elab_expr(ctx, e->u.if_expr.then_br);
      ne->u.if_expr.else_br = elab_expr(ctx, e->u.if_expr.else_br);
      return ne;
    }

    case STIGI_EXPR_WHILE: {
      StigiExpr *ne = ea_alloc(ctx, sizeof *ne);
      if (ne == NULL) return (StigiExpr *)e;
      *ne = *e;
      ne->u.while_expr.cond = elab_expr(ctx, e->u.while_expr.cond);
      ne->u.while_expr.decreases = elab_expr(ctx, e->u.while_expr.decreases);
      ne->u.while_expr.body = elab_expr(ctx, e->u.while_expr.body);
      return ne;
    }

    case STIGI_EXPR_MATCH: {
      if (e->u.match_expr.scrutinee_count >= 2 && e->u.match_expr.scrutinees != NULL) {
        return compile_match_expr(ctx, e->u.match_expr.scrutinees, e->u.match_expr.scrutinee_count,
                                  e->u.match_expr.arms, e->u.match_expr.arm_count, e->span);
      }
      StigiExpr *s0 = e->u.match_expr.scrutinee;
      return compile_match_expr(ctx, &s0, 1, e->u.match_expr.arms, e->u.match_expr.arm_count,
                                e->span);
    }

    case STIGI_EXPR_BLOCK:
      return elab_block_tail(ctx, e->u.block.stmts, 0, e->u.block.count, e->span);

    case STIGI_EXPR_VAL: {
      StigiExpr *ne = ea_alloc(ctx, sizeof *ne);
      if (ne == NULL) return (StigiExpr *)e;
      *ne = *e;
      ne->u.val_stmt.init = elab_expr(ctx, e->u.val_stmt.init);
      return ne;
    }

    case STIGI_EXPR_VAR: {
      StigiExpr *ne = ea_alloc(ctx, sizeof *ne);
      if (ne == NULL) return (StigiExpr *)e;
      *ne = *e;
      ne->u.var_stmt.init = elab_expr(ctx, e->u.var_stmt.init);
      return ne;
    }

    case STIGI_EXPR_ASSIGN: {
      StigiExpr *ne = ea_alloc(ctx, sizeof *ne);
      if (ne == NULL) return (StigiExpr *)e;
      *ne = *e;
      ne->u.assign_stmt.value = elab_expr(ctx, e->u.assign_stmt.value);
      return ne;
    }

    case STIGI_EXPR_INDEX_READ:
    case STIGI_EXPR_INDEX_WRITE: {
      StigiExpr *ne = ea_alloc(ctx, sizeof *ne);
      if (ne == NULL) return (StigiExpr *)e;
      *ne = *e;
      ne->u.index_expr.target = elab_expr(ctx, e->u.index_expr.target);
      ne->u.index_expr.index = elab_expr(ctx, e->u.index_expr.index);
      ne->u.index_expr.value = elab_expr(ctx, e->u.index_expr.value);
      return ne;
    }

    case STIGI_EXPR_HANDLE: {
      StigiExpr *ne = ea_alloc(ctx, sizeof *ne);
      if (ne == NULL) return (StigiExpr *)e;
      *ne = *e;
      if (e->u.handle_stmt.arg_count > 0) {
        ne->u.handle_stmt.args = ea_alloc(ctx, (size_t)e->u.handle_stmt.arg_count *
                                               sizeof *ne->u.handle_stmt.args);
        if (ne->u.handle_stmt.args == NULL) return (StigiExpr *)e;
        for (uint32_t i = 0; i < e->u.handle_stmt.arg_count; i++) {
          ne->u.handle_stmt.args[i] = elab_expr(ctx, e->u.handle_stmt.args[i]);
        }
      }
      ne->u.handle_stmt.body = elab_expr(ctx, e->u.handle_stmt.body);
      return ne;
    }

    case STIGI_EXPR_ASSERT: {
      StigiExpr *ne = ea_alloc(ctx, sizeof *ne);
      if (ne == NULL) return (StigiExpr *)e;
      *ne = *e;
      ne->u.assert_expr.expr = elab_expr(ctx, e->u.assert_expr.expr);
      return ne;
    }

    case STIGI_EXPR_IS:
      return elab_is(ctx, e);
  }
  return (StigiExpr *)e;
}

static void elab_def_decl(ElabCtx *ctx, StigiDecl *d) {
  ctx->cur_decl = d;
  if (d->u.def_decl.clause_count > 0) {
    /* S122: Erlang-style multi-head def lowers directly through Maranget over params */
    const StigiDefHead *h = &d->u.def_decl.head;
    if (h->param_count == 0) {
      elab_add_diag(ctx, STIGI_TYPE_PATTERN_ARITY, d->span,
                    "multi-head def '%.*s' requires at least 1 value parameter",
                    (int)d->name.len, d->name.ptr);
      d->u.def_decl.elab_body = make_unit_expr(ctx, d->span);
      return;
    }
    StigiExpr **pscruts = malloc((size_t)h->param_count * sizeof *pscruts);
    if (pscruts == NULL) {
      ctx->err = diag_set(ctx->diag, STIGI_ERR_OOM, -1, "multi-head OOM");
      return;
    }
    for (uint32_t p = 0; p < h->param_count; p++) {
      pscruts[p] = make_local_expr(ctx, h->params[p].name, h->params[p].span);
    }
    d->u.def_decl.elab_body = compile_match_expr(ctx, pscruts, h->param_count,
                                                 d->u.def_decl.clauses,
                                                 d->u.def_decl.clause_count, d->span);
    free(pscruts);
  } else if (d->u.def_decl.body != NULL) {
    d->u.def_decl.elab_body = elab_expr(ctx, d->u.def_decl.body);
  }
}

Status stigi_elaborate_image(StigiImage *img, StigiTypeDiags *diags, Diag *diag) {
  if (img == NULL) return STIGI_OK;
  for (uint32_t i = 0; i < img->unit_count; i++) {
    StigiDecl *d = img->units[i].ast->decl;
    ElabCtx ctx;
    memset(&ctx, 0, sizeof ctx);
    ctx.img = img;
    ctx.uidx = i;
    ctx.diags = diags;
    ctx.diag = diag;
    ctx.err = STIGI_OK;

    if (d->kind == STIGI_DECL_DEF || d->kind == STIGI_DECL_LEMMA_DEF) {
      elab_def_decl(&ctx, d);
    } else if (d->kind == STIGI_DECL_VAL || d->kind == STIGI_DECL_TEST) {
      if (d->u.val_decl.body != NULL) {
        ctx.cur_decl = d;
        d->u.val_decl.elab_body = elab_expr(&ctx, d->u.val_decl.body);
      }
    } else if (d->kind == STIGI_DECL_HANDLER) {
      for (uint32_t m = 0; m < d->u.handler_decl.def_count && ctx.err == STIGI_OK; m++) {
        StigiDecl *md = d->u.handler_decl.defs[m];
        if (md->kind == STIGI_DECL_DEF) {
          elab_def_decl(&ctx, md);
        }
      }
    } else if (d->kind == STIGI_DECL_GIVEN) {
      for (uint32_t m = 0; m < d->u.given_decl.def_count && ctx.err == STIGI_OK; m++) {
        StigiDecl *md = d->u.given_decl.defs[m];
        if (md->kind == STIGI_DECL_DEF) {
          elab_def_decl(&ctx, md);
        }
      }
    }
    if (ctx.err != STIGI_OK) return ctx.err;
  }
  return STIGI_OK;
}
