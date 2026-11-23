#include "extract.h"
#include "eval.h"
#include "kanren.h"
#include "types.h"

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- Portable C11 exact 128-bit integer helpers ---- */

static StigiFactCol col_sym(const char *s) {
  StigiFactCol c;
  c.is_num = false;
  c.num = tupu_i128_from_i64(0);
  c.sym = s != NULL ? s : "";
  return c;
}

static StigiFactCol col_i64(int64_t v) {
  StigiFactCol c;
  c.is_num = true;
  c.num = tupu_i128_from_i64(v);
  c.sym = NULL;
  return c;
}

static StigiFactCol col_u64(uint64_t v) {
  StigiFactCol c;
  c.is_num = true;
  c.num = tupu_i128_from_u64(v);
  c.sym = NULL;
  return c;
}

static StigiFactCol col_i128(TupuI128 v) {
  StigiFactCol c;
  c.is_num = true;
  c.num = v;
  c.sym = NULL;
  return c;
}

static TupuI128 i128_neg_u64(uint64_t mag) {
  TupuI128 r;
  if (mag == 0ULL) {
    r.lo = 0ULL;
    r.hi = 0;
    return r;
  }
  r.lo = 0ULL - mag;
  r.hi = -1;
  return r;
}

static int i128_cmp(TupuI128 a, TupuI128 b) {
  if (a.hi != b.hi) return a.hi < b.hi ? -1 : 1;
  if (a.lo != b.lo) return a.lo < b.lo ? -1 : 1;
  return 0;
}

size_t stigi_i128_format(TupuI128 v, char out[48]) {
  bool neg = v.hi < 0;
  uint64_t ulo = v.lo;
  uint64_t uhi = 0;
  memcpy(&uhi, &v.hi, sizeof uhi);
  if (neg) {
    ulo = ~ulo + 1ULL;
    uhi = ~uhi + (ulo == 0ULL ? 1ULL : 0ULL);
  }
  uint32_t w[4] = {
      (uint32_t)(uhi >> 32),
      (uint32_t)(uhi & 0xFFFFFFFFULL),
      (uint32_t)(ulo >> 32),
      (uint32_t)(ulo & 0xFFFFFFFFULL),
  };
  char rev[44];
  size_t rlen = 0;
  if (w[0] == 0 && w[1] == 0 && w[2] == 0 && w[3] == 0) {
    rev[rlen++] = '0';
  } else {
    while (w[0] != 0 || w[1] != 0 || w[2] != 0 || w[3] != 0) {
      uint64_t rem = 0;
      for (int k = 0; k < 4; k++) {
        uint64_t cur = (rem << 32) | (uint64_t)w[k];
        w[k] = (uint32_t)(cur / 10ULL);
        rem = cur % 10ULL;
      }
      rev[rlen++] = (char)('0' + (int)rem);
    }
  }
  size_t pos = 0;
  if (neg) out[pos++] = '-';
  for (size_t i = rlen; i > 0; i--) out[pos++] = rev[i - 1];
  out[pos] = '\0';
  return pos;
}

/* ---- FactSet storage ---- */

void stigi_facts_init(StigiFactSet *fs) {
  if (fs == NULL) return;
  memset(fs, 0, sizeof *fs);
  arena_init(&fs->arena);
}

void stigi_facts_free(StigiFactSet *fs) {
  if (fs == NULL) return;
  free(fs->rows);
  arena_free(&fs->arena);
  memset(fs, 0, sizeof *fs);
}

static const char *fs_strdup(StigiFactSet *fs, const char *s, size_t n) {
  char *p = arena_alloc(&fs->arena, n + 1U);
  if (p == NULL) return NULL;
  if (n > 0) memcpy(p, s, n);
  p[n] = '\0';
  return p;
}

/* Two-pass exact arena formatter (zero fixed-buffer truncation) */
static const char *fs_fmt(StigiFactSet *fs, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int need = vsnprintf(NULL, 0, fmt, ap);
  va_end(ap);
  if (need < 0) return NULL;
  size_t len = (size_t)need;
  char *p = arena_alloc(&fs->arena, len + 1U);
  if (p == NULL) return NULL;
  va_start(ap, fmt);
  (void)vsnprintf(p, len + 1U, fmt, ap);
  va_end(ap);
  return p;
}

static Status fs_add(StigiFactSet *fs, const char *rel, int ncols,
                     const StigiFactCol *cols, Diag *diag) {
  if (fs->count == fs->cap) {
    uint32_t ncap = fs->cap == 0 ? 64U : fs->cap * 2U;
    if (ncap <= fs->cap) return diag_set(diag, STIGI_ERR_OVERFLOW, -1, "fact set overflow");
    StigiFactRow *nr = realloc(fs->rows, (size_t)ncap * sizeof *nr);
    if (nr == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "fact set OOM");
    fs->rows = nr;
    fs->cap = ncap;
  }
  StigiFactRow *r = &fs->rows[fs->count++];
  r->rel = rel;
  r->ncols = ncols;
  for (int i = 0; i < ncols && i < STIGI_MAX_FACT_COLS; i++) {
    r->cols[i] = cols[i];
  }
  return STIGI_OK;
}

Status stigi_facts_add_sym(StigiFactSet *fs, const char *rel, int ncols,
                           const char *const *syms, Diag *diag) {
  if (fs == NULL || rel == NULL || ncols < 0 || ncols > STIGI_MAX_FACT_COLS ||
      (ncols > 0 && syms == NULL)) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_facts_add_sym: invalid arguments");
  }
  const char *rdup = fs_strdup(fs, rel, strlen(rel));
  if (rdup == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
  StigiFactCol cols[STIGI_MAX_FACT_COLS];
  for (int i = 0; i < ncols; i++) {
    const char *s = syms[i] != NULL ? syms[i] : "";
    const char *sdup = fs_strdup(fs, s, strlen(s));
    if (sdup == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
    cols[i] = col_sym(sdup);
  }
  return fs_add(fs, rdup, ncols, cols, diag);
}

Status stigi_facts_append(StigiFactSet *dst, const StigiFactSet *src, Diag *diag) {
  if (dst == NULL || src == NULL) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_facts_append: invalid arguments");
  }
  for (uint32_t i = 0; i < src->count; i++) {
    const StigiFactRow *r = &src->rows[i];
    const char *rel = fs_strdup(dst, r->rel, strlen(r->rel));
    if (rel == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
    StigiFactCol cols[STIGI_MAX_FACT_COLS];
    for (int c = 0; c < r->ncols; c++) {
      cols[c] = r->cols[c];
      if (!cols[c].is_num) {
        cols[c].sym = fs_strdup(dst, r->cols[c].sym, strlen(r->cols[c].sym));
        if (cols[c].sym == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
      }
    }
    Status st = fs_add(dst, rel, r->ncols, cols, diag);
    if (st != STIGI_OK) return st;
  }
  return STIGI_OK;
}

static Status emit1(StigiFactSet *fs, const char *rel, StigiFactCol c0, Diag *d) {
  StigiFactCol cols[1] = {c0};
  return fs_add(fs, rel, 1, cols, d);
}
static Status emit2(StigiFactSet *fs, const char *rel, StigiFactCol c0, StigiFactCol c1, Diag *d) {
  StigiFactCol cols[2] = {c0, c1};
  return fs_add(fs, rel, 2, cols, d);
}
static Status emit3(StigiFactSet *fs, const char *rel, StigiFactCol c0, StigiFactCol c1,
                    StigiFactCol c2, Diag *d) {
  StigiFactCol cols[3] = {c0, c1, c2};
  return fs_add(fs, rel, 3, cols, d);
}
static Status emit4(StigiFactSet *fs, const char *rel, StigiFactCol c0, StigiFactCol c1,
                    StigiFactCol c2, StigiFactCol c3, Diag *d) {
  StigiFactCol cols[4] = {c0, c1, c2, c3};
  return fs_add(fs, rel, 4, cols, d);
}
static Status emit5(StigiFactSet *fs, const char *rel, StigiFactCol c0, StigiFactCol c1,
                    StigiFactCol c2, StigiFactCol c3, StigiFactCol c4, Diag *d) {
  StigiFactCol cols[5] = {c0, c1, c2, c3, c4};
  return fs_add(fs, rel, 5, cols, d);
}
static Status emit6(StigiFactSet *fs, const char *rel, StigiFactCol c0, StigiFactCol c1,
                    StigiFactCol c2, StigiFactCol c3, StigiFactCol c4, StigiFactCol c5, Diag *d) {
  StigiFactCol cols[6] = {c0, c1, c2, c3, c4, c5};
  return fs_add(fs, rel, 6, cols, d);
}

/* ---- Kernel tables (exact 128-bit ranges, S37 requiresRow, S72 stack profile, S74 taintSource) ---- */

static Status emit_kernel_tables(StigiFactSet *fs, Diag *diag) {
  Status st = emit3(fs, "typeRange", col_sym("U8"), col_i64(0), col_i64(255), diag);
  if (st != STIGI_OK) return st;
  st = emit2(fs, "typeWidth", col_sym("U8"), col_i64(8), diag);
  if (st != STIGI_OK) return st;

  st = emit3(fs, "typeRange", col_sym("I8"), col_i64(-128), col_i64(127), diag);
  if (st != STIGI_OK) return st;
  st = emit2(fs, "typeWidth", col_sym("I8"), col_i64(8), diag);
  if (st != STIGI_OK) return st;

  st = emit3(fs, "typeRange", col_sym("U32"), col_i64(0), col_u64(4294967295ULL), diag);
  if (st != STIGI_OK) return st;
  st = emit2(fs, "typeWidth", col_sym("U32"), col_i64(32), diag);
  if (st != STIGI_OK) return st;

  st = emit3(fs, "typeRange", col_sym("U64"), col_i64(0), col_u64(UINT64_MAX), diag);
  if (st != STIGI_OK) return st;
  st = emit2(fs, "typeWidth", col_sym("U64"), col_i64(64), diag);
  if (st != STIGI_OK) return st;

  st = emit3(fs, "typeRange", col_sym("I32"), col_i64(-2147483648LL), col_i64(2147483647LL), diag);
  if (st != STIGI_OK) return st;
  st = emit2(fs, "typeWidth", col_sym("I32"), col_i64(32), diag);
  if (st != STIGI_OK) return st;

  st = emit3(fs, "typeRange", col_sym("I64"), col_i64(INT64_MIN), col_i64(INT64_MAX), diag);
  if (st != STIGI_OK) return st;
  st = emit2(fs, "typeWidth", col_sym("I64"), col_i64(64), diag);
  if (st != STIGI_OK) return st;

  /* Target profile stack budget (1 MiB) and signal reserve (64 KiB), memory.md 7 / runtime.md 1 */
  st = emit1(fs, "stackBudget", col_i64(1048576LL), diag);
  if (st != STIGI_OK) return st;
  st = emit1(fs, "signalReserve", col_i64(65536LL), diag);
  if (st != STIGI_OK) return st;

  st = emit1(fs, "taintSource", col_sym("Stigi.File"), diag);
  if (st != STIGI_OK) return st;
  st = emit1(fs, "taintSource", col_sym("Stigi.Socket"), diag);
  if (st != STIGI_OK) return st;
  st = emit1(fs, "taintSource", col_sym("Stigi.Clock"), diag);
  if (st != STIGI_OK) return st;

  /* requiresRow table (S37, S74) */
  static const char *const k_req[][2] = {
      {"IndexRead", "IndexRead"}, {"IndexRead", "Operand"}, {"IndexRead", "Result"},
      {"IndexWrite", "IndexWrite"}, {"IndexWrite", "ElementWrite"}, {"IndexWrite", "Overwrite"},
      {"IndexWrite", "WriteVia"}, {"IndexWrite", "Operand"},
      {"Add", "AddOp"}, {"Add", "Operand"}, {"Add", "Result"},
      {"Sub", "SubtractOp"}, {"Sub", "Operand"}, {"Sub", "Result"},
      {"Mul", "MultiplyOp"}, {"Mul", "Operand"}, {"Mul", "Result"},
      {"Div", "QuotientOp"}, {"Div", "Operand"}, {"Div", "Result"},
      {"Mod", "RemainderOp"}, {"Mod", "Operand"}, {"Mod", "Result"},
      {"Shift", "ShiftOp"}, {"Shift", "Operand"}, {"Shift", "Result"},
      {"And", "AndOp"}, {"And", "Operand"}, {"And", "Result"},
      {"WrapOp", "Operand"}, {"WrapOp", "Result"},
      {"Narrow", "Operand"}, {"Narrow", "Result"},
      {"Fill", "FillOp"}, {"Fill", "Result"},
      {"Grow", "Operand"}, {"Grow", "Result"},
      {"Expect", "Operand"}, {"Expect", "Result"},
      {"Ctor", "Result"},
      {"CapCall", "OperationCall"}, {"CapCall", "Result"},
      {"Cond", "Condition"}, {"Cond", "TrueEdge"}, {"Cond", "FalseEdge"},
      {"CondBool", "TrueEdge"}, {"CondBool", "FalseEdge"},
      {"Call", "CallOf"}, {"Call", "Operand"}, {"Call", "ArgVia"}, {"Call", "Result"},
      {"Call0", "CallOf"}, {"Call0", "Result"},
      {"LoopHead", "BackEdge"},
      {"Assert", "Assert"}, {"Val", "Val"}, {"Var", "Var"}, {"Assign", "Assign"},
      {"Lit", "Lit"}, {"Ref", "Ref"}, {"Match", "Match"}, {"MatchArm", "MatchArm"},
      {"Handle", "Handle"}, {"UnOp", "UnOp"}, {"Deny", "Deny"}, {"Block", "Block"}};
  for (size_t i = 0; i < sizeof(k_req) / sizeof(k_req[0]); i++) {
    st = emit2(fs, "requiresRow", col_sym(k_req[i][0]), col_sym(k_req[i][1]), diag);
    if (st != STIGI_OK) return st;
  }
  return STIGI_OK;
}

/* ---- Extraction context ---- */

typedef enum {
  TRY_NONE = 0,
  TRY_FILL,
  TRY_GROW
} ExtTryKind;

typedef struct {
  const char *sym;
  const char *ty;
  bool is_array;
  bool is_static;
  const char *len_sym;
  const char *elem_ty;
  ExtTryKind try_kind;
  const char *try_region;
  const char *try_cap;
  const char *try_elem_ty;
  const char *try_old_obj;
  const char *try_old_len;
} ExtVal;

typedef struct {
  StigiStr name;
  const char *val_sym;
  const char *len_sym;
  const char *type_sym;
  const char *elem_ty;
  bool is_array;
  bool is_static;
  bool is_param;
  bool param_mutated;
  bool param_consumed;
  bool has_inv;
  StigiFactGroup inv;
  const StigiDecl *refined_decl;
  uint32_t refined_uidx;
} ExtVar;

typedef struct {
  StigiStr binder;
  const char *prov_sym;
  const char *unit_sym;
  const char *first_pt;
  bool from_handle;
  bool closed;
} ExtCap;

typedef struct {
  const char *denial_sym;
} ExtDenial;

typedef struct {
  const char *ty;
  TupuI128 num;
  const char *sym;
} ExtLitCache;

typedef struct {
  const StigiImage *img;
  uint32_t uidx;
  const char *unit_sym;
  const char *inst_sym;
  StigiFactSet *fs;
  Diag *diag;
  Status err;

  uint32_t next_op;
  uint32_t next_pt;
  uint32_t next_sc;
  uint32_t next_val;
  uint32_t next_atom;
  const char *cur_pt;
  const char *cur_sc;
  const char *entry_pt;
  bool in_top_block;

  /* Active def decreases measure (if any) */
  const char *def_dec_site;
  const char *def_dec_val;
  int32_t def_dec_param_idx;

  ExtVar *vars;
  uint32_t var_count;
  uint32_t var_cap;

  ExtCap *caps;
  uint32_t cap_count;
  uint32_t cap_cap;

  ExtDenial *denials;
  uint32_t denial_count;
  uint32_t denial_cap;

  const StigiParam *rigid_tparams;
  uint32_t rigid_tparam_count;
  StigiCapParam *local_assumps;
  uint32_t local_assump_count;

  ExtLitCache *lits;
  uint32_t lit_count;
  uint32_t lit_cap;
} ExtCtx;

static void ext_ctx_free(ExtCtx *ctx) {
  free(ctx->vars);
  free(ctx->caps);
  free(ctx->denials);
  free(ctx->local_assumps);
  free(ctx->lits);
  ctx->vars = NULL;
  ctx->caps = NULL;
  ctx->denials = NULL;
  ctx->local_assumps = NULL;
  ctx->lits = NULL;
}

static bool str_eq_c(StigiStr s, const char *lit) {
  size_t n = strlen(lit);
  return s.len == n && memcmp(s.ptr, lit, n) == 0;
}

static bool is_int_type_name(const char *t) {
  if (t == NULL) return false;
  return strcmp(t, "U8") == 0 || strcmp(t, "I8") == 0 || strcmp(t, "U32") == 0 ||
         strcmp(t, "I32") == 0 || strcmp(t, "U64") == 0 || strcmp(t, "I64") == 0;
}

static const char *ctx_strdup(ExtCtx *ctx, const char *s, size_t n) {
  if (ctx->err != STIGI_OK) return "";
  const char *r = fs_strdup(ctx->fs, s, n);
  if (r == NULL) {
    ctx->err = diag_set(ctx->diag, STIGI_ERR_OOM, -1, "extract OOM");
    return "";
  }
  return r;
}

static const char *ctx_fmt(ExtCtx *ctx, const char *fmt, ...) {
  if (ctx->err != STIGI_OK) return "";
  va_list ap;
  va_start(ap, fmt);
  int need = vsnprintf(NULL, 0, fmt, ap);
  va_end(ap);
  if (need < 0) {
    ctx->err = diag_set(ctx->diag, STIGI_ERR_INVALID, -1, "extract format failed");
    return "";
  }
  size_t len = (size_t)need;
  char *p = arena_alloc(&ctx->fs->arena, len + 1U);
  if (p == NULL) {
    ctx->err = diag_set(ctx->diag, STIGI_ERR_OOM, -1, "extract OOM");
    return "";
  }
  va_start(ap, fmt);
  (void)vsnprintf(p, len + 1U, fmt, ap);
  va_end(ap);
  return p;
}

static void ctx_emit1(ExtCtx *ctx, const char *rel, StigiFactCol c0) {
  if (ctx->err != STIGI_OK) return;
  ctx->err = emit1(ctx->fs, rel, c0, ctx->diag);
}
static void ctx_emit2(ExtCtx *ctx, const char *rel, StigiFactCol c0, StigiFactCol c1) {
  if (ctx->err != STIGI_OK) return;
  ctx->err = emit2(ctx->fs, rel, c0, c1, ctx->diag);
}
static void ctx_emit3(ExtCtx *ctx, const char *rel, StigiFactCol c0, StigiFactCol c1,
                      StigiFactCol c2) {
  if (ctx->err != STIGI_OK) return;
  ctx->err = emit3(ctx->fs, rel, c0, c1, c2, ctx->diag);
}
static void ctx_emit4(ExtCtx *ctx, const char *rel, StigiFactCol c0, StigiFactCol c1,
                      StigiFactCol c2, StigiFactCol c3) {
  if (ctx->err != STIGI_OK) return;
  ctx->err = emit4(ctx->fs, rel, c0, c1, c2, c3, ctx->diag);
}
static void ctx_emit5(ExtCtx *ctx, const char *rel, StigiFactCol c0, StigiFactCol c1,
                      StigiFactCol c2, StigiFactCol c3, StigiFactCol c4) {
  if (ctx->err != STIGI_OK) return;
  ctx->err = emit5(ctx->fs, rel, c0, c1, c2, c3, c4, ctx->diag);
}
static void ctx_emit6(ExtCtx *ctx, const char *rel, StigiFactCol c0, StigiFactCol c1,
                      StigiFactCol c2, StigiFactCol c3, StigiFactCol c4, StigiFactCol c5) {
  if (ctx->err != STIGI_OK) return;
  ctx->err = emit6(ctx->fs, rel, c0, c1, c2, c3, c4, c5, ctx->diag);
}

static ExtVar *ext_push_var(ExtCtx *ctx) {
  if (ctx->err != STIGI_OK) return NULL;
  if (ctx->var_count == ctx->var_cap) {
    uint32_t ncap = ctx->var_cap == 0 ? 32U : ctx->var_cap * 2U;
    ExtVar *nv = realloc(ctx->vars, (size_t)ncap * sizeof *nv);
    if (nv == NULL) {
      ctx->err = diag_set(ctx->diag, STIGI_ERR_OOM, -1, "extract vars OOM");
      return NULL;
    }
    ctx->vars = nv;
    ctx->var_cap = ncap;
  }
  ExtVar *v = &ctx->vars[ctx->var_count++];
  memset(v, 0, sizeof *v);
  return v;
}

static ExtCap *ext_push_cap(ExtCtx *ctx) {
  if (ctx->err != STIGI_OK) return NULL;
  if (ctx->cap_count == ctx->cap_cap) {
    uint32_t ncap = ctx->cap_cap == 0 ? 16U : ctx->cap_cap * 2U;
    ExtCap *nc = realloc(ctx->caps, (size_t)ncap * sizeof *nc);
    if (nc == NULL) {
      ctx->err = diag_set(ctx->diag, STIGI_ERR_OOM, -1, "extract caps OOM");
      return NULL;
    }
    ctx->caps = nc;
    ctx->cap_cap = ncap;
  }
  ExtCap *c = &ctx->caps[ctx->cap_count++];
  memset(c, 0, sizeof *c);
  return c;
}

static void ext_push_denial(ExtCtx *ctx, const char *dsym) {
  if (ctx->err != STIGI_OK) return;
  if (ctx->denial_count == ctx->denial_cap) {
    uint32_t ncap = ctx->denial_cap == 0 ? 8U : ctx->denial_cap * 2U;
    ExtDenial *nd = realloc(ctx->denials, (size_t)ncap * sizeof *nd);
    if (nd == NULL) {
      ctx->err = diag_set(ctx->diag, STIGI_ERR_OOM, -1, "extract denials OOM");
      return;
    }
    ctx->denials = nd;
    ctx->denial_cap = ncap;
  }
  ctx->denials[ctx->denial_count++].denial_sym = dsym;
}

static ExtVar *ext_find_var(ExtCtx *ctx, StigiStr name) {
  for (uint32_t i = ctx->var_count; i > 0; i--) {
    if (ctx->vars[i - 1].name.len == name.len &&
        memcmp(ctx->vars[i - 1].name.ptr, name.ptr, name.len) == 0) {
      return &ctx->vars[i - 1];
    }
  }
  return NULL;
}

static ExtCap *ext_find_cap_by_binder(ExtCtx *ctx, StigiStr binder) {
  if (binder.len == 0) return NULL;
  for (uint32_t i = ctx->cap_count; i > 0; i--) {
    if (ctx->caps[i - 1].binder.len == binder.len &&
        memcmp(ctx->caps[i - 1].binder.ptr, binder.ptr, binder.len) == 0) {
      return &ctx->caps[i - 1];
    }
  }
  return NULL;
}

static ExtCap *ext_find_cap_by_unit(ExtCtx *ctx, const char *unit_sym) {
  if (unit_sym == NULL) return NULL;
  for (uint32_t i = ctx->cap_count; i > 0; i--) {
    if (strcmp(ctx->caps[i - 1].unit_sym, unit_sym) == 0) {
      return &ctx->caps[i - 1];
    }
  }
  return NULL;
}

static const char *ext_new_op(ExtCtx *ctx) {
  return ctx_fmt(ctx, "%s:o%u", ctx->inst_sym, ctx->next_op++);
}

static const char *ext_new_atom(ExtCtx *ctx) {
  return ctx_fmt(ctx, "%s:a%u", ctx->inst_sym, ctx->next_atom++);
}

static StigiType *ext_make_named_ast_type(Arena *a, const char *name) {
  if (name == NULL) return NULL;
  StigiType *t = arena_alloc(a, sizeof *t);
  if (t == NULL) return NULL;
  memset(t, 0, sizeof *t);
  t->kind = STIGI_TYPE_NAMED;
  size_t len = strlen(name);
  char *cp = arena_alloc(a, len + 1U);
  if (cp == NULL) return NULL;
  memcpy(cp, name, len + 1U);
  t->name = (StigiStr){ cp, (uint32_t)len };
  return t;
}

static StigiType *ext_subst_ast_type(Arena *a, const StigiType *ty,
                                     const StigiParam *tparams, uint32_t tpc,
                                     const StigiType *const *targs, uint32_t tac) {
  if (ty == NULL) return NULL;
  if (ty->kind == STIGI_TYPE_NAMED && ty->type_arg_count == 0) {
    for (uint32_t i = 0; i < tpc && i < tac; i++) {
      if (targs[i] != NULL && ty->name.len == tparams[i].name.len &&
          memcmp(ty->name.ptr, tparams[i].name.ptr, ty->name.len) == 0) {
        return (StigiType *)targs[i];
      }
    }
    return (StigiType *)ty;
  }
  StigiType *nt = arena_alloc(a, sizeof *nt);
  if (nt == NULL) return (StigiType *)ty;
  *nt = *ty;
  if (ty->type_arg_count > 0) {
    nt->type_args = arena_alloc(a, (size_t)ty->type_arg_count * sizeof *nt->type_args);
    if (nt->type_args != NULL) {
      for (uint32_t i = 0; i < ty->type_arg_count; i++) {
        nt->type_args[i] = ext_subst_ast_type(a, ty->type_args[i], tparams, tpc, targs, tac);
      }
    }
  }
  return nt;
}

static void ext_emit_given_lookup(ExtCtx *ctx, const StigiType *goal_ty) {
  if (goal_ty == NULL) return;
  Arena ka;
  arena_init(&ka);
  StigiKanrenProof pr = stigi_kanren_solve(ctx->img, ctx->uidx, goal_ty,
                                           ctx->rigid_tparams, ctx->rigid_tparam_count,
                                           ctx->local_assumps, ctx->local_assump_count, &ka);
  const char *st_s = "Missing";
  switch (pr.status) {
    case STIGI_KANREN_FOUND:
    case STIGI_KANREN_LOCAL_EV: st_s = "Found"; break;
    case STIGI_KANREN_MISSING: st_s = "Missing"; break;
    case STIGI_KANREN_AMBIGUOUS: st_s = "Ambiguous"; break;
    case STIGI_KANREN_FUEL_EXCEEDED: st_s = "OutOfFuel"; break;
  }
  const char *ga = ext_new_atom(ctx);
  ctx_emit2(ctx, "givenLookup", col_sym(ga), col_sym(st_s));
  arena_free(&ka);
}

static const char *ext_new_pt_raw(ExtCtx *ctx) {
  const char *pt = ctx_fmt(ctx, "%s:p%u", ctx->inst_sym, ctx->next_pt++);
  for (uint32_t i = 0; i < ctx->denial_count; i++) {
    ctx_emit2(ctx, "denialRange", col_sym(ctx->denials[i].denial_sym), col_sym(pt));
  }
  return pt;
}

static const char *ext_new_pt(ExtCtx *ctx) {
  const char *pt = ext_new_pt_raw(ctx);
  if (ctx->cur_pt != NULL) {
    ctx_emit2(ctx, "next", col_sym(ctx->cur_pt), col_sym(pt));
  }
  ctx->cur_pt = pt;
  return pt;
}

static const char *ext_new_sc(ExtCtx *ctx, const char *parent_sc) {
  const char *sc = ctx_fmt(ctx, "%s:s%u", ctx->inst_sym, ctx->next_sc++);
  ctx_emit1(ctx, "scope", col_sym(sc));
  if (parent_sc != NULL) {
    ctx_emit2(ctx, "parent", col_sym(sc), col_sym(parent_sc));
  }
  return sc;
}

static ExtVal ext_val_zero(const char *sym, const char *ty) {
  ExtVal ev;
  memset(&ev, 0, sizeof ev);
  ev.sym = sym;
  ev.ty = ty;
  return ev;
}

static ExtVal ext_new_val(ExtCtx *ctx, const char *ty_sym) {
  const char *ty = (ty_sym != NULL && ty_sym[0] != '\0') ? ty_sym : "U64";
  const char *v = ctx_fmt(ctx, "%s:v%u", ctx->inst_sym, ctx->next_val++);
  ctx_emit1(ctx, "value", col_sym(v));
  ctx_emit2(ctx, "valueType", col_sym(v), col_sym(ty));
  if (strcmp(ty, "U64") == 0) {
    ctx_emit1(ctx, "u64", col_sym(v));
  }
  return ext_val_zero(v, ty);
}

/* Deduplicate pure integer literals per (type, exact 128-bit value) inside one def instance.
 * Eliminates duplicate lit(a, n) rows in bounds.dl without losing any precision. */
static ExtVal ext_lit_val(ExtCtx *ctx, const char *ty_sym, TupuI128 num) {
  const char *ty = (ty_sym != NULL && ty_sym[0] != '\0') ? ty_sym : "U64";
  for (uint32_t i = 0; i < ctx->lit_count; i++) {
    if (ctx->lits[i].num.lo == num.lo && ctx->lits[i].num.hi == num.hi &&
        strcmp(ctx->lits[i].ty, ty) == 0) {
      return ext_val_zero(ctx->lits[i].sym, ty);
    }
  }
  ExtVal v = ext_new_val(ctx, ty);
  ctx_emit2(ctx, "lit", col_sym(v.sym), col_i128(num));
  if (ctx->err == STIGI_OK) {
    if (ctx->lit_count == ctx->lit_cap) {
      uint32_t ncap = ctx->lit_cap == 0 ? 16U : ctx->lit_cap * 2U;
      ExtLitCache *nl = realloc(ctx->lits, (size_t)ncap * sizeof *nl);
      if (nl != NULL) {
        ctx->lits = nl;
        ctx->lit_cap = ncap;
      }
    }
    if (ctx->lit_count < ctx->lit_cap) {
      ctx->lits[ctx->lit_count].ty = ty;
      ctx->lits[ctx->lit_count].num = num;
      ctx->lits[ctx->lit_count].sym = v.sym;
      ctx->lit_count++;
    }
  }
  return v;
}

/* ---- String literal decoded length ---- */

static uint64_t decoded_str_len(StigiStr raw) {
  if (raw.len < 2) return 0;
  uint64_t len = 0;
  size_t i = 1;
  size_t end = raw.len - 1;
  while (i < end) {
    if (raw.ptr[i] == '\\' && i + 1 < end) {
      if (raw.ptr[i + 1] == 'x' && i + 3 < end) i += 4;
      else i += 2;
    } else {
      i++;
    }
    len++;
  }
  return len;
}

/* ---- Type inspection & transparent alias unwrapping ---- */

static const StigiType *unwrap_type_alias(const StigiImage *img, uint32_t from_idx,
                                          const StigiType *ty, int depth,
                                          uint32_t *out_alias_idx) {
  if (ty == NULL || depth > 16) return ty;
  if (ty->kind != STIGI_TYPE_NAMED) return ty;
  int32_t idx = stigi_image_resolve_name(img, from_idx, ty->name.ptr, ty->name.len);
  if (idx < 0) return ty;
  const StigiDecl *d = img->units[idx].ast->decl;
  if (d->kind != STIGI_DECL_TYPE || d->u.type_decl.rhs == NULL) return ty;
  if (out_alias_idx != NULL) *out_alias_idx = (uint32_t)idx;
  /* Simple zero-param alias can unwrap directly; parameterized alias handled at call site */
  if (d->u.type_decl.val_param_count == 0 && d->u.type_decl.type_param_count == 0) {
    return unwrap_type_alias(img, (uint32_t)idx, d->u.type_decl.rhs, depth + 1, out_alias_idx);
  }
  return ty;
}

static const StigiDecl *find_refined_type_decl(const StigiImage *img, uint32_t from_idx,
                                               const StigiType *ty, uint32_t *out_decl_uidx,
                                               int depth) {
  if (ty == NULL || depth > 16 || ty->kind != STIGI_TYPE_NAMED) return NULL;
  int32_t idx = stigi_image_resolve_name(img, from_idx, ty->name.ptr, ty->name.len);
  if (idx < 0) return NULL;
  const StigiDecl *d = img->units[idx].ast->decl;
  if (d->kind != STIGI_DECL_TYPE) return NULL;
  if (d->u.type_decl.has_facts) {
    if (out_decl_uidx != NULL) *out_decl_uidx = (uint32_t)idx;
    return d;
  }
  if (d->u.type_decl.rhs != NULL && d->u.type_decl.val_param_count == 0 &&
      d->u.type_decl.type_param_count == 0) {
    return find_refined_type_decl(img, (uint32_t)idx, d->u.type_decl.rhs, out_decl_uidx, depth + 1);
  }
  return NULL;
}

static bool type_is_handle_carrying(const StigiImage *img, uint32_t from_idx,
                                    const StigiType *ty, int depth) {
  if (ty == NULL || depth > 16) return false;
  if (ty->kind == STIGI_TYPE_ARRAY || ty->kind == STIGI_TYPE_CELL || ty->kind == STIGI_TYPE_PTR) {
    return true;
  }
  int32_t idx = stigi_image_resolve_name(img, from_idx, ty->name.ptr, ty->name.len);
  if (idx < 0) return false;
  const StigiDecl *d = img->units[idx].ast->decl;
  if (d->kind == STIGI_DECL_TYPE) {
    return type_is_handle_carrying(img, (uint32_t)idx, d->u.type_decl.rhs, depth + 1);
  }
  if (d->kind == STIGI_DECL_ENUM) {
    for (uint32_t c = 0; c < d->u.enum_decl.case_count; c++) {
      for (uint32_t f = 0; f < d->u.enum_decl.cases[c].field_count; f++) {
        if (type_is_handle_carrying(img, (uint32_t)idx,
                                    d->u.enum_decl.cases[c].fields[f].type, depth + 1)) {
          return true;
        }
      }
    }
  }
  return false;
}

static const char *type_name_sym(ExtCtx *ctx, const StigiType *ty) {
  if (ty == NULL) return "U64";
  const StigiType *u = unwrap_type_alias(ctx->img, ctx->uidx, ty, 0, NULL);
  if (u->kind == STIGI_TYPE_ARRAY) return "Array";
  if (u->kind == STIGI_TYPE_CELL) return "Cell";
  if (u->kind == STIGI_TYPE_PTR) return "Ptr";
  if (u->name.len > 0) return ctx_strdup(ctx, u->name.ptr, u->name.len);
  return "U64";
}

/* ---- Top-level val compile-time integer lookup ---- */

static bool eval_top_val_int(const StigiImage *img, uint32_t val_uidx,
                             TupuI128 *out_num, const char **out_ty) {
  const StigiDecl *d = img->units[val_uidx].ast->decl;
  if (d->kind != STIGI_DECL_VAL || d->u.val_decl.type == NULL) return false;
  const StigiType *vt = unwrap_type_alias(img, val_uidx, d->u.val_decl.type, 0, NULL);
  const char *ty = NULL;
  if (str_eq_c(vt->name, "U8")) ty = "U8";
  else if (str_eq_c(vt->name, "I8")) ty = "I8";
  else if (str_eq_c(vt->name, "U32")) ty = "U32";
  else if (str_eq_c(vt->name, "I32")) ty = "I32";
  else if (str_eq_c(vt->name, "U64")) ty = "U64";
  else if (str_eq_c(vt->name, "I64")) ty = "I64";
  else return false;

  Arena ea;
  arena_init(&ea);
  StigiEvalResult er = stigi_eval_unit(img, val_uidx, 4096U, &ea);
  bool ok = false;
  if (er.status == STIGI_OK && er.value.kind == STIGI_VAL_INT) {
    if (er.value.is_unsigned) {
      *out_num = tupu_i128_from_u64(er.value.u.u);
      ok = true;
    } else {
      *out_num = tupu_i128_from_i64(er.value.u.i);
      ok = true;
    }
  }
  arena_free(&ea);
  if (ok && out_ty != NULL) *out_ty = ty;
  return ok;
}

/* ---- Safety fact classification (S21) ---- */

static bool is_safety_atom(StigiStr name) {
  return str_eq_c(name, "below") || str_eq_c(name, "Stigi.Facts.below") ||
         str_eq_c(name, "lessOrEqual") || str_eq_c(name, "Stigi.Facts.lessOrEqual") ||
         str_eq_c(name, "at") || str_eq_c(name, "Stigi.Facts.at") ||
         str_eq_c(name, "outlives") || str_eq_c(name, "Stigi.Facts.outlives") ||
         str_eq_c(name, "decreases") || str_eq_c(name, "valid") ||
         str_eq_c(name, "overwrites") || str_eq_c(name, "Stigi.Facts.overwrites");
}

static bool fact_group_has_safety(const StigiFactGroup *fg) {
  for (uint32_t i = 0; i < fg->count; i++) {
    StigiFactKind k = fg->items[i].kind;
    if (k == STIGI_FACT_CMP || k == STIGI_FACT_DENY) return true;
    if (k == STIGI_FACT_ATOM && is_safety_atom(fg->items[i].atom_name)) return true;
  }
  return false;
}

/* ---- Forward declarations ---- */

static ExtVal ext_expr(ExtCtx *ctx, const StigiExpr *e, const char *hint_ty, bool in_tail);

static bool ext_lookup_lit_u64(const ExtCtx *ctx, const char *sym, uint64_t *out) {
  if (ctx == NULL || sym == NULL || out == NULL) return false;
  for (uint32_t i = 0; i < ctx->lit_count; i++) {
    if (ctx->lits[i].num.hi == 0 && strcmp(ctx->lits[i].sym, sym) == 0) {
      *out = ctx->lits[i].num.lo;
      return true;
    }
  }
  return false;
}

static const char *ext_resolve_fact_val(ExtCtx *ctx, const StigiExpr *e, const char *res_sym,
                                        const StigiParam *subst_params,
                                        const char *const *subst_vals, uint32_t subst_count) {
  if (e == NULL) return ext_new_val(ctx, "U64").sym;
  if (e->kind == STIGI_EXPR_LOCAL || e->kind == STIGI_EXPR_GLOBAL) {
    if ((str_eq_c(e->u.ident.name, "result") || str_eq_c(e->u.ident.name, "self")) &&
        res_sym != NULL) {
      return res_sym;
    }
    for (uint32_t i = 0; i < subst_count; i++) {
      if (subst_params[i].name.len == e->u.ident.name.len &&
          memcmp(subst_params[i].name.ptr, e->u.ident.name.ptr, e->u.ident.name.len) == 0) {
        return subst_vals[i];
      }
    }
    ExtVar *ev = ext_find_var(ctx, e->u.ident.name);
    if (ev != NULL) {
      if (ctx->entry_pt != NULL) {
        ctx_emit2(ctx, "binderRead", col_sym(ev->val_sym), col_sym(ctx->entry_pt));
      }
      return ev->val_sym;
    }
    /* Resolve top-level compile-time val constant (e.g. Stigi.Table.maxCap) */
    int32_t uidx = stigi_image_resolve_name(ctx->img, ctx->uidx, e->u.ident.name.ptr,
                                            e->u.ident.name.len);
    if (uidx >= 0) {
      TupuI128 num;
      const char *vty = "U64";
      if (eval_top_val_int(ctx->img, (uint32_t)uidx, &num, &vty)) {
        return ext_lit_val(ctx, vty, num).sym;
      }
    }
    return ctx_fmt(ctx, "%s:sym:%.*s", ctx->inst_sym, (int)e->u.ident.name.len,
                   e->u.ident.name.ptr);
  }
  if (e->kind == STIGI_EXPR_LIT_INT) {
    TupuI128 num = e->u.lit_int.overflow ? (TupuI128){0ULL, 1} : tupu_i128_from_u64(e->u.lit_int.value);
    return ext_lit_val(ctx, "U64", num).sym;
  }
  if (e->kind == STIGI_EXPR_UNOP && e->u.unop.op == STIGI_OP_NEG &&
      e->u.unop.operand != NULL && e->u.unop.operand->kind == STIGI_EXPR_LIT_INT) {
    TupuI128 num = i128_neg_u64(e->u.unop.operand->u.lit_int.value);
    return ext_lit_val(ctx, "I64", num).sym;
  }
  if (e->kind == STIGI_EXPR_BINOP &&
      (e->u.binop.op == STIGI_OP_ADD || e->u.binop.op == STIGI_OP_SUB ||
       e->u.binop.op == STIGI_OP_MUL)) {
    const char *lx = ext_resolve_fact_val(ctx, e->u.binop.lhs, res_sym, subst_params, subst_vals,
                                          subst_count);
    const char *ry = ext_resolve_fact_val(ctx, e->u.binop.rhs, res_sym, subst_params, subst_vals,
                                          subst_count);
    uint64_t lnum = 0, rnum = 0;
    if (ext_lookup_lit_u64(ctx, lx, &lnum) && ext_lookup_lit_u64(ctx, ry, &rnum)) {
      if (e->u.binop.op == STIGI_OP_ADD && lnum <= UINT64_MAX - rnum) {
        return ext_lit_val(ctx, "U64", tupu_i128_from_u64(lnum + rnum)).sym;
      }
      if (e->u.binop.op == STIGI_OP_SUB && lnum >= rnum) {
        return ext_lit_val(ctx, "U64", tupu_i128_from_u64(lnum - rnum)).sym;
      }
      if (e->u.binop.op == STIGI_OP_MUL && (rnum == 0 || lnum <= UINT64_MAX / rnum)) {
        return ext_lit_val(ctx, "U64", tupu_i128_from_u64(lnum * rnum)).sym;
      }
    }
    const char *op = ext_new_op(ctx);
    const char *pt = ext_new_pt(ctx);
    ExtVal res = ext_new_val(ctx, "U64");
    const char *kind_s = e->u.binop.op == STIGI_OP_ADD ? "Add"
                         : (e->u.binop.op == STIGI_OP_SUB ? "Sub" : "Mul");
    const char *rel_s = e->u.binop.op == STIGI_OP_ADD ? "addOp"
                        : (e->u.binop.op == STIGI_OP_SUB ? "subtractOp" : "multiplyOp");
    const char *row_s = e->u.binop.op == STIGI_OP_ADD ? "AddOp"
                        : (e->u.binop.op == STIGI_OP_SUB ? "SubtractOp" : "MultiplyOp");
    ctx_emit4(ctx, "operation", col_sym(op), col_sym(kind_s), col_sym(pt), col_sym(ctx->cur_sc));
    ctx_emit2(ctx, "operationOf", col_sym(op), col_sym(ctx->inst_sym));
    ctx_emit5(ctx, rel_s, col_sym(op), col_sym(res.sym), col_sym(lx), col_sym(ry),
              col_sym(ctx->cur_sc));
    ctx_emit3(ctx, "operand", col_sym(op), col_i64(0), col_sym(lx));
    ctx_emit3(ctx, "operand", col_sym(op), col_i64(1), col_sym(ry));
    ctx_emit2(ctx, "result", col_sym(op), col_sym(res.sym));
    ctx_emit2(ctx, "emitted", col_sym(op), col_sym(row_s));
    ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Operand"));
    ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Result"));
    return res.sym;
  }
  return ext_new_val(ctx, "U64").sym;
}

/* Extract length term symbol for an Array type, substituting optional parameter map */
static const char *ext_array_len_sym(ExtCtx *ctx, const StigiType *ty,
                                     const StigiParam *sp, const char *const *sv, uint32_t scnt,
                                     bool *out_is_array, const char **out_elem_ty) {
  *out_is_array = false;
  if (out_elem_ty != NULL) *out_elem_ty = "U64";
  if (ty == NULL) return NULL;
  const StigiType *u = unwrap_type_alias(ctx->img, ctx->uidx, ty, 0, NULL);
  if (u->kind == STIGI_TYPE_ARRAY) {
    *out_is_array = true;
    if (out_elem_ty != NULL && u->type_arg_count > 0) {
      *out_elem_ty = type_name_sym(ctx, u->type_args[0]);
    }
    if (u->len_arg_count > 0 && u->len_args[0] != NULL) {
      return ext_resolve_fact_val(ctx, u->len_args[0], NULL, sp, sv, scnt);
    }
    return ext_new_val(ctx, "U64").sym;
  }
  /* Parameterized type alias whose rhs is Array[T](len) */
  if (u->kind == STIGI_TYPE_NAMED) {
    int32_t tidx = stigi_image_resolve_name(ctx->img, ctx->uidx, u->name.ptr, u->name.len);
    if (tidx >= 0 && ctx->img->units[tidx].ast->decl->kind == STIGI_DECL_TYPE) {
      const StigiDecl *td = ctx->img->units[tidx].ast->decl;
      if (td->u.type_decl.rhs != NULL && td->u.type_decl.rhs->kind == STIGI_TYPE_ARRAY) {
        *out_is_array = true;
        const StigiType *ar = td->u.type_decl.rhs;
        if (out_elem_ty != NULL && ar->type_arg_count > 0) {
          *out_elem_ty = type_name_sym(ctx, ar->type_args[0]);
        }
        uint32_t nv = td->u.type_decl.val_param_count;
        if (nv > 0 && u->len_arg_count == nv) {
          const char **vals = malloc((size_t)nv * sizeof *vals);
          if (vals == NULL) {
            ctx->err = diag_set(ctx->diag, STIGI_ERR_OOM, -1, "OOM");
            return NULL;
          }
          for (uint32_t k = 0; k < nv; k++) {
            vals[k] = ext_resolve_fact_val(ctx, u->len_args[k], NULL, sp, sv, scnt);
          }
          const char *ls = ar->len_arg_count > 0
                               ? ext_resolve_fact_val(ctx, ar->len_args[0], NULL,
                                                      td->u.type_decl.val_params, vals, nv)
                               : ext_new_val(ctx, "U64").sym;
          free(vals);
          return ls;
        }
        return ext_new_val(ctx, "U64").sym;
      }
    }
  }
  return NULL;
}

static void ext_emit_denial(ExtCtx *ctx, const StigiFactItem *deny_it) {
  if (deny_it == NULL || deny_it->kind != STIGI_FACT_DENY) return;
  const char *dsym = ext_new_atom(ctx);
  for (uint32_t i = 0; i < deny_it->deny_atom_count; i++) {
    const StigiFactItem *sub = &deny_it->deny_atoms[i];
    if (sub->kind != STIGI_FACT_ATOM) continue;
    const char *ksym = ext_new_atom(ctx);
    ctx_emit2(ctx, "denialAtom", col_sym(dsym), col_sym(ksym));
    if ((str_eq_c(sub->atom_name, "at") || str_eq_c(sub->atom_name, "Stigi.Facts.at")) &&
        sub->arg_count == 2 &&
        (sub->args[0]->kind == STIGI_EXPR_LOCAL || sub->args[0]->kind == STIGI_EXPR_GLOBAL) &&
        (sub->args[1]->kind == STIGI_EXPR_LOCAL || sub->args[1]->kind == STIGI_EXPR_GLOBAL)) {
      ExtCap *cap = ext_find_cap_by_binder(ctx, sub->args[0]->u.ident.name);
      const char *cs = cap != NULL ? cap->prov_sym
                                   : ctx_strdup(ctx, sub->args[0]->u.ident.name.ptr,
                                                sub->args[0]->u.ident.name.len);
      const char *ss = ctx_strdup(ctx, sub->args[1]->u.ident.name.ptr,
                                  sub->args[1]->u.ident.name.len);
      ctx_emit4(ctx, "denyState", col_sym(dsym), col_sym(ksym), col_sym(cs), col_sym(ss));
      if (cap != NULL) {
        const char *sr = ext_new_atom(ctx);
        ctx_emit3(ctx, "stateRef", col_sym(sr), col_sym(cap->unit_sym), col_sym(ss));
      }
    } else if (sub->arg_count == 1 &&
               (sub->args[0]->kind == STIGI_EXPR_LOCAL ||
                sub->args[0]->kind == STIGI_EXPR_GLOBAL)) {
      ExtCap *cap = ext_find_cap_by_binder(ctx, sub->args[0]->u.ident.name);
      if (cap != NULL) {
        const char *op_s = ctx_fmt(ctx, "%s.%.*s", cap->unit_sym,
                                   (int)sub->atom_name.len, sub->atom_name.ptr);
        ctx_emit4(ctx, "denyOperation", col_sym(dsym), col_sym(ksym), col_sym(cap->prov_sym),
                  col_sym(op_s));
      }
    }
  }
  ext_push_denial(ctx, dsym);
}

static void ext_emit_credits_subst(ExtCtx *ctx, const StigiFactGroup *fg, const char *sc,
                                   const char *res_sym, const StigiParam *sp,
                                   const char *const *sv, uint32_t scnt) {
  for (uint32_t i = 0; i < fg->count; i++) {
    const StigiFactItem *it = &fg->items[i];
    if (it->kind == STIGI_FACT_DENY) {
      ext_emit_denial(ctx, it);
      continue;
    }
    if (it->kind == STIGI_FACT_CMP) {
      const char *vx = ext_resolve_fact_val(ctx, it->lhs, res_sym, sp, sv, scnt);
      const char *vy = ext_resolve_fact_val(ctx, it->rhs, res_sym, sp, sv, scnt);
      if (it->op == STIGI_OP_LT) {
        ctx_emit3(ctx, "baseBelow", col_sym(vx), col_sym(vy), col_sym(sc));
      } else if (it->op == STIGI_OP_LE) {
        ctx_emit3(ctx, "baseLeq", col_sym(vx), col_sym(vy), col_sym(sc));
      } else if (it->op == STIGI_OP_GT) {
        ctx_emit3(ctx, "baseBelow", col_sym(vy), col_sym(vx), col_sym(sc));
      } else if (it->op == STIGI_OP_GE) {
        ctx_emit3(ctx, "baseLeq", col_sym(vy), col_sym(vx), col_sym(sc));
      } else if (it->op == STIGI_OP_EQ) {
        ctx_emit3(ctx, "baseLeq", col_sym(vx), col_sym(vy), col_sym(sc));
        ctx_emit3(ctx, "baseLeq", col_sym(vy), col_sym(vx), col_sym(sc));
      }
      continue;
    }
    if (it->kind != STIGI_FACT_ATOM) continue;
    if (it->arg_count == 2) {
      if (str_eq_c(it->atom_name, "below") || str_eq_c(it->atom_name, "Stigi.Facts.below")) {
        const char *vx = ext_resolve_fact_val(ctx, it->args[0], res_sym, sp, sv, scnt);
        const char *vy = ext_resolve_fact_val(ctx, it->args[1], res_sym, sp, sv, scnt);
        ctx_emit3(ctx, "baseBelow", col_sym(vx), col_sym(vy), col_sym(sc));
      } else if (str_eq_c(it->atom_name, "lessOrEqual") ||
                 str_eq_c(it->atom_name, "Stigi.Facts.lessOrEqual")) {
        const char *vx = ext_resolve_fact_val(ctx, it->args[0], res_sym, sp, sv, scnt);
        const char *vy = ext_resolve_fact_val(ctx, it->args[1], res_sym, sp, sv, scnt);
        ctx_emit3(ctx, "baseLeq", col_sym(vx), col_sym(vy), col_sym(sc));
      } else if ((str_eq_c(it->atom_name, "at") || str_eq_c(it->atom_name, "Stigi.Facts.at")) &&
                 (it->args[0]->kind == STIGI_EXPR_LOCAL || it->args[0]->kind == STIGI_EXPR_GLOBAL) &&
                 (it->args[1]->kind == STIGI_EXPR_LOCAL || it->args[1]->kind == STIGI_EXPR_GLOBAL)) {
        ExtCap *cap = ext_find_cap_by_binder(ctx, it->args[0]->u.ident.name);
        if (cap != NULL) {
          const char *ss = ctx_strdup(ctx, it->args[1]->u.ident.name.ptr,
                                      it->args[1]->u.ident.name.len);
          const char *sr = ext_new_atom(ctx);
          ctx_emit3(ctx, "stateRef", col_sym(sr), col_sym(cap->unit_sym), col_sym(ss));
        }
      }
    }
    /* Check user relation arity */
    if (!is_safety_atom(it->atom_name)) {
      int32_t ridx = stigi_image_resolve_name(ctx->img, ctx->uidx, it->atom_name.ptr,
                                              it->atom_name.len);
      if (ridx >= 0 && ctx->img->units[ridx].ast->decl->kind == STIGI_DECL_RELATION) {
        const char *aid = ext_new_atom(ctx);
        const char *rname = ctx_strdup(ctx, ctx->img->units[ridx].name,
                                       ctx->img->units[ridx].name_len);
        ctx_emit3(ctx, "atomUse", col_sym(aid), col_sym(rname), col_i64((int64_t)it->arg_count));
      }
    }
  }
}

static void ext_emit_owed_subst(ExtCtx *ctx, const char *op, const StigiFactGroup *fg,
                                const char *sc, const char *code, const char *res_sym,
                                const StigiParam *sp, const char *const *sv, uint32_t scnt) {
  for (uint32_t i = 0; i < fg->count; i++) {
    const StigiFactItem *it = &fg->items[i];
    if (it->kind == STIGI_FACT_CMP) {
      const char *vx = ext_resolve_fact_val(ctx, it->lhs, res_sym, sp, sv, scnt);
      const char *vy = ext_resolve_fact_val(ctx, it->rhs, res_sym, sp, sv, scnt);
      if (it->op == STIGI_OP_LT) {
        ctx_emit5(ctx, "belowOwed", col_sym(op), col_sym(vx), col_sym(vy), col_sym(sc),
                  col_sym(code));
      } else if (it->op == STIGI_OP_LE) {
        ctx_emit5(ctx, "leqOwed", col_sym(op), col_sym(vx), col_sym(vy), col_sym(sc),
                  col_sym(code));
      } else if (it->op == STIGI_OP_GT) {
        ctx_emit5(ctx, "belowOwed", col_sym(op), col_sym(vy), col_sym(vx), col_sym(sc),
                  col_sym(code));
      } else if (it->op == STIGI_OP_GE) {
        ctx_emit5(ctx, "leqOwed", col_sym(op), col_sym(vy), col_sym(vx), col_sym(sc),
                  col_sym(code));
      } else if (it->op == STIGI_OP_EQ) {
        ctx_emit5(ctx, "leqOwed", col_sym(op), col_sym(vx), col_sym(vy), col_sym(sc),
                  col_sym(code));
        ctx_emit5(ctx, "leqOwed", col_sym(op), col_sym(vy), col_sym(vx), col_sym(sc),
                  col_sym(code));
      }
      continue;
    }
    if (it->kind != STIGI_FACT_ATOM || it->arg_count != 2) continue;
    if (str_eq_c(it->atom_name, "below") || str_eq_c(it->atom_name, "Stigi.Facts.below")) {
      const char *vx = ext_resolve_fact_val(ctx, it->args[0], res_sym, sp, sv, scnt);
      const char *vy = ext_resolve_fact_val(ctx, it->args[1], res_sym, sp, sv, scnt);
      ctx_emit5(ctx, "belowOwed", col_sym(op), col_sym(vx), col_sym(vy), col_sym(sc),
                col_sym(code));
    } else if (str_eq_c(it->atom_name, "lessOrEqual") ||
               str_eq_c(it->atom_name, "Stigi.Facts.lessOrEqual")) {
      const char *vx = ext_resolve_fact_val(ctx, it->args[0], res_sym, sp, sv, scnt);
      const char *vy = ext_resolve_fact_val(ctx, it->args[1], res_sym, sp, sv, scnt);
      ctx_emit5(ctx, "leqOwed", col_sym(op), col_sym(vx), col_sym(vy), col_sym(sc),
                col_sym(code));
    }
  }
}

static void ext_emit_refined_credits(ExtCtx *ctx, const StigiDecl *rt, uint32_t decl_uidx,
                                     const char *sc, const char *val_sym) {
  if (rt == NULL || rt->kind != STIGI_DECL_TYPE || !rt->u.type_decl.has_facts) return;
  uint32_t saved_uidx = ctx->uidx;
  ctx->uidx = decl_uidx;
  if (fact_group_has_safety(&rt->u.type_decl.facts)) {
    ctx_emit2(ctx, "statesFact", col_sym(ctx->unit_sym), col_sym("Refinement"));
  }
  ext_emit_credits_subst(ctx, &rt->u.type_decl.facts, sc, val_sym, NULL, NULL, 0);
  ctx->uidx = saved_uidx;
}

static void ext_emit_refined_owed(ExtCtx *ctx, const char *op, const StigiDecl *rt,
                                  uint32_t decl_uidx, const char *sc, const char *val_sym) {
  if (rt == NULL || rt->kind != STIGI_DECL_TYPE || !rt->u.type_decl.has_facts) return;
  uint32_t saved_uidx = ctx->uidx;
  ctx->uidx = decl_uidx;
  ext_emit_owed_subst(ctx, op, &rt->u.type_decl.facts, sc, "refinement-unproved",
                      val_sym, NULL, NULL, 0);
  ctx->uidx = saved_uidx;
}

static bool expr_is_pure_total_leaf(const StigiImage *img, uint32_t uidx, const StigiExpr *e,
                                    int depth) {
  if (e == NULL) return true;
  if (depth > 64) return false;
  switch (e->kind) {
    case STIGI_EXPR_LIT_INT:
    case STIGI_EXPR_LIT_CHAR:
    case STIGI_EXPR_LIT_STR:
    case STIGI_EXPR_LIT_BOOL:
    case STIGI_EXPR_LIT_UNIT:
    case STIGI_EXPR_LOCAL:
    case STIGI_EXPR_GLOBAL:
      return true;
    case STIGI_EXPR_UNOP:
      if (e->u.unop.op == STIGI_OP_NEG &&
          (e->u.unop.operand == NULL || e->u.unop.operand->kind != STIGI_EXPR_LIT_INT)) {
        return false;
      }
      return expr_is_pure_total_leaf(img, uidx, e->u.unop.operand, depth + 1);
    case STIGI_EXPR_BINOP: {
      StigiOp op = e->u.binop.op;
      if (op == STIGI_OP_ADD || op == STIGI_OP_SUB || op == STIGI_OP_MUL ||
          op == STIGI_OP_DIV || op == STIGI_OP_MOD) {
        return false;
      }
      return expr_is_pure_total_leaf(img, uidx, e->u.binop.lhs, depth + 1) &&
             expr_is_pure_total_leaf(img, uidx, e->u.binop.rhs, depth + 1);
    }
    case STIGI_EXPR_IF:
      return expr_is_pure_total_leaf(img, uidx, e->u.if_expr.cond, depth + 1) &&
             expr_is_pure_total_leaf(img, uidx, e->u.if_expr.then_br, depth + 1) &&
             expr_is_pure_total_leaf(img, uidx, e->u.if_expr.else_br, depth + 1);
    case STIGI_EXPR_BLOCK:
      for (uint32_t i = 0; i < e->u.block.count; i++) {
        if (!expr_is_pure_total_leaf(img, uidx, e->u.block.stmts[i], depth + 1)) return false;
      }
      return true;
    case STIGI_EXPR_VAL:
      if (e->u.val_stmt.type != NULL &&
          find_refined_type_decl(img, uidx, e->u.val_stmt.type, NULL, 0) != NULL) {
        return false;
      }
      return expr_is_pure_total_leaf(img, uidx, e->u.val_stmt.init, depth + 1);
    case STIGI_EXPR_MATCH:
      if (!expr_is_pure_total_leaf(img, uidx, e->u.match_expr.scrutinee, depth + 1)) return false;
      for (uint32_t i = 0; i < e->u.match_expr.arm_count; i++) {
        if (!expr_is_pure_total_leaf(img, uidx, e->u.match_expr.arms[i].body, depth + 1)) {
          return false;
        }
      }
      return true;
    case STIGI_EXPR_CALL: {
      for (uint32_t i = 0; i < e->u.call.arg_count; i++) {
        if (!expr_is_pure_total_leaf(img, uidx, e->u.call.args[i], depth + 1)) return false;
      }
      if (e->u.call.callee == NULL ||
          (e->u.call.callee->kind != STIGI_EXPR_LOCAL &&
           e->u.call.callee->kind != STIGI_EXPR_GLOBAL)) {
        return false;
      }
      StigiStr cn = e->u.call.callee->u.ident.name;
      if (str_eq_c(cn, "U64.ofU8") || str_eq_c(cn, "U64.ofU32") ||
          str_eq_c(cn, "U64.mix") || str_eq_c(cn, "U64.wrappingAdd") ||
          str_eq_c(cn, "U64.wrappingSubtract") || str_eq_c(cn, "U64.wrappingMultiply") ||
          str_eq_c(cn, "U64.bitwiseAnd") || str_eq_c(cn, "U64.bitwiseOr") ||
          str_eq_c(cn, "U64.bitwiseXor") || str_eq_c(cn, "U32.wrappingAdd") ||
          str_eq_c(cn, "U32.wrappingSubtract") || str_eq_c(cn, "U32.wrappingMultiply") ||
          str_eq_c(cn, "U32.bitwiseAnd") || str_eq_c(cn, "U32.bitwiseOr") ||
          str_eq_c(cn, "U32.bitwiseXor") || str_eq_c(cn, "U32.rotateRight")) {
        return true;
      }
      /* Enum constructor without case facts is pure & total */
      uint32_t cidx = 0;
      bool ambig = false;
      int32_t eidx = stigi_image_resolve_enum_case(img, uidx, cn.ptr, cn.len, &cidx, &ambig);
      if (eidx >= 0 && !ambig) {
        const StigiEnumCase *ec = &img->units[eidx].ast->decl->u.enum_decl.cases[cidx];
        if (ec->has_facts) return false;
        for (uint32_t f = 0; f < ec->field_count; f++) {
          if (find_refined_type_decl(img, (uint32_t)eidx, ec->fields[f].type, NULL, 0) != NULL) {
            return false;
          }
        }
        return true;
      }
      /* Pure trait operation call (Trait.op) is pure & total (S125) */
      {
        const char *dot = NULL;
        for (size_t i = 0; i < cn.len; i++) {
          if (cn.ptr[i] == '.') dot = cn.ptr + i;
        }
        if (dot != NULL) {
          int32_t tr = stigi_image_resolve_name(img, uidx, cn.ptr, (size_t)(dot - cn.ptr));
          if (tr >= 0 && img->units[tr].ast->decl->kind == STIGI_DECL_TRAIT) {
            return true;
          }
        }
      }
      /* Non-recursive call to a pure leaf def with no preconditions/capabilities */
      int32_t tgt = stigi_image_resolve_name(img, uidx, cn.ptr, cn.len);
      if (tgt >= 0 && (uint32_t)tgt != uidx &&
          img->units[tgt].ast->decl->kind == STIGI_DECL_DEF) {
        const StigiDefHead *ch = &img->units[tgt].ast->decl->u.def_decl.head;
        for (uint32_t g = 0; g < ch->pre_group_count; g++) {
          if (ch->pre_groups[g].is_fact || ch->pre_groups[g].cap_count > 0) return false;
        }
        if (ch->post_val_fact_count > 0) return false;
        return expr_is_pure_total_leaf(img, (uint32_t)tgt,
                                       stigi_def_body(img->units[tgt].ast->decl), depth + 1);
      }
      return false;
    }
    case STIGI_EXPR_WHILE:
    case STIGI_EXPR_VAR:
    case STIGI_EXPR_ASSIGN:
    case STIGI_EXPR_INDEX_READ:
    case STIGI_EXPR_INDEX_WRITE:
    case STIGI_EXPR_HANDLE:
    case STIGI_EXPR_ASSERT:
    case STIGI_EXPR_DENY:
    case STIGI_EXPR_LOCAL_DECL:
      return false;
  }
  return false;
}

static bool is_pure_total_leaf_def(const StigiImage *img, uint32_t uidx,
                                   const StigiDefHead *h, const StigiExpr *body) {
  if (img == NULL || uidx >= img->unit_count || h == NULL || body == NULL) return false;
  const StigiDecl *d = img->units[uidx].ast->decl;
  if (d->kind != STIGI_DECL_DEF) return false;
  if (strcmp(img->units[uidx].name, "main") == 0) return false;
  for (uint32_t g = 0; g < h->pre_group_count; g++) {
    if (!h->pre_groups[g].is_fact) {
      for (uint32_t c = 0; c < h->pre_groups[g].cap_count; c++) {
        const StigiType *ct = h->pre_groups[g].caps[c].cap_type;
        int32_t cidx = ct != NULL ? stigi_image_resolve_name(img, uidx, ct->name.ptr, ct->name.len) : -1;
        if (cidx < 0 || img->units[cidx].ast->decl->kind != STIGI_DECL_TRAIT) return false;
      }
    }
  }
  if (h->ret_type != NULL && find_refined_type_decl(img, uidx, h->ret_type, NULL, 0) != NULL) {
    return false;
  }
  return expr_is_pure_total_leaf(img, uidx, body, 0);
}

/* ---- Find decreases(m) inside a fact group ---- */

static const StigiExpr *find_decreases_in_fg(const StigiFactGroup *fg) {
  for (uint32_t i = 0; i < fg->count; i++) {
    const StigiFactItem *it = &fg->items[i];
    if (it->kind == STIGI_FACT_ATOM && str_eq_c(it->atom_name, "decreases") &&
        it->arg_count == 1 && it->args[0] != NULL) {
      return it->args[0];
    }
  }
  return NULL;
}

/* ---- Collect vars assigned inside an AST subtree (for while loop SSA) ---- */

static void collect_assigned_vars(ExtCtx *ctx, const StigiExpr *e, bool *assigned,
                                  uint32_t var_limit) {
  if (e == NULL) return;
  switch (e->kind) {
    case STIGI_EXPR_ASSIGN: {
      for (uint32_t i = var_limit; i > 0; i--) {
        if (ctx->vars[i - 1].name.len == e->u.assign_stmt.name.len &&
            memcmp(ctx->vars[i - 1].name.ptr, e->u.assign_stmt.name.ptr,
                   e->u.assign_stmt.name.len) == 0) {
          assigned[i - 1] = true;
          break;
        }
      }
      collect_assigned_vars(ctx, e->u.assign_stmt.value, assigned, var_limit);
      break;
    }
    case STIGI_EXPR_BLOCK:
      for (uint32_t i = 0; i < e->u.block.count; i++) {
        collect_assigned_vars(ctx, e->u.block.stmts[i], assigned, var_limit);
      }
      break;
    case STIGI_EXPR_IF:
      collect_assigned_vars(ctx, e->u.if_expr.cond, assigned, var_limit);
      collect_assigned_vars(ctx, e->u.if_expr.then_br, assigned, var_limit);
      collect_assigned_vars(ctx, e->u.if_expr.else_br, assigned, var_limit);
      break;
    case STIGI_EXPR_WHILE:
      collect_assigned_vars(ctx, e->u.while_expr.cond, assigned, var_limit);
      collect_assigned_vars(ctx, e->u.while_expr.body, assigned, var_limit);
      break;
    case STIGI_EXPR_MATCH:
      collect_assigned_vars(ctx, e->u.match_expr.scrutinee, assigned, var_limit);
      for (uint32_t i = 0; i < e->u.match_expr.arm_count; i++) {
        collect_assigned_vars(ctx, e->u.match_expr.arms[i].body, assigned, var_limit);
      }
      break;
    case STIGI_EXPR_VAL:
      collect_assigned_vars(ctx, e->u.val_stmt.init, assigned, var_limit);
      break;
    case STIGI_EXPR_VAR:
      collect_assigned_vars(ctx, e->u.var_stmt.init, assigned, var_limit);
      break;
    case STIGI_EXPR_CALL:
      collect_assigned_vars(ctx, e->u.call.callee, assigned, var_limit);
      for (uint32_t i = 0; i < e->u.call.arg_count; i++) {
        collect_assigned_vars(ctx, e->u.call.args[i], assigned, var_limit);
      }
      break;
    case STIGI_EXPR_BINOP:
      collect_assigned_vars(ctx, e->u.binop.lhs, assigned, var_limit);
      collect_assigned_vars(ctx, e->u.binop.rhs, assigned, var_limit);
      break;
    case STIGI_EXPR_UNOP:
      collect_assigned_vars(ctx, e->u.unop.operand, assigned, var_limit);
      break;
    case STIGI_EXPR_INDEX_READ:
    case STIGI_EXPR_INDEX_WRITE:
      collect_assigned_vars(ctx, e->u.index_expr.target, assigned, var_limit);
      collect_assigned_vars(ctx, e->u.index_expr.index, assigned, var_limit);
      collect_assigned_vars(ctx, e->u.index_expr.value, assigned, var_limit);
      break;
    case STIGI_EXPR_HANDLE:
      for (uint32_t i = 0; i < e->u.handle_stmt.arg_count; i++) {
        collect_assigned_vars(ctx, e->u.handle_stmt.args[i], assigned, var_limit);
      }
      collect_assigned_vars(ctx, e->u.handle_stmt.body, assigned, var_limit);
      break;
    case STIGI_EXPR_ASSERT:
      collect_assigned_vars(ctx, e->u.assert_expr.expr, assigned, var_limit);
      break;
    case STIGI_EXPR_LOCAL:
    case STIGI_EXPR_GLOBAL:
    case STIGI_EXPR_LIT_INT:
    case STIGI_EXPR_LIT_CHAR:
    case STIGI_EXPR_LIT_STR:
    case STIGI_EXPR_LIT_BOOL:
    case STIGI_EXPR_LIT_UNIT:
    case STIGI_EXPR_DENY:
    case STIGI_EXPR_LOCAL_DECL:
      break;
  }
}

/* ---- Short-circuit boolean condition branch extractor ---- */

static void ext_emit_cond_branch(ExtCtx *ctx, const StigiExpr *cond,
                                 const char *true_join_pt, const char *false_join_pt,
                                 const char **out_true_sc, const char **out_false_sc) {
  if (cond != NULL && cond->kind == STIGI_EXPR_BINOP && cond->u.binop.op == STIGI_OP_AND) {
    /* A && B: if A is false -> false_join_pt; if A is true -> evaluate B in tsA (or pre_sc) */
    const char *pre_sc = ctx->cur_sc;
    const char *a_true_pt = ext_new_pt_raw(ctx);
    const char *a_true_sc = NULL;
    const char *a_false_sc = NULL;
    ext_emit_cond_branch(ctx, cond->u.binop.lhs, a_true_pt, false_join_pt,
                         &a_true_sc, &a_false_sc);
    ctx->cur_sc = a_true_sc != NULL ? a_true_sc : pre_sc;
    ctx->cur_pt = a_true_pt;
    const char *b_true_sc = NULL;
    const char *b_false_sc = NULL;
    ext_emit_cond_branch(ctx, cond->u.binop.rhs, true_join_pt, false_join_pt,
                         &b_true_sc, &b_false_sc);
    if (out_true_sc != NULL) *out_true_sc = b_true_sc != NULL ? b_true_sc : pre_sc;
    /* False edge can come from A-false OR B-false, so false join inherits pre_sc */
    if (out_false_sc != NULL) *out_false_sc = pre_sc;
    return;
  }

  if (cond != NULL && cond->kind == STIGI_EXPR_BINOP && cond->u.binop.op == STIGI_OP_OR) {
    /* A || B: if A is true -> true_join_pt; if A is false -> evaluate B in fsA (or pre_sc) */
    const char *pre_sc = ctx->cur_sc;
    const char *a_false_pt = ext_new_pt_raw(ctx);
    const char *a_true_sc = NULL;
    const char *a_false_sc = NULL;
    ext_emit_cond_branch(ctx, cond->u.binop.lhs, true_join_pt, a_false_pt,
                         &a_true_sc, &a_false_sc);
    ctx->cur_sc = a_false_sc != NULL ? a_false_sc : pre_sc;
    ctx->cur_pt = a_false_pt;
    const char *b_true_sc = NULL;
    const char *b_false_sc = NULL;
    ext_emit_cond_branch(ctx, cond->u.binop.rhs, true_join_pt, false_join_pt,
                         &b_true_sc, &b_false_sc);
    if (out_true_sc != NULL) *out_true_sc = pre_sc;
    if (out_false_sc != NULL) *out_false_sc = b_false_sc != NULL ? b_false_sc : pre_sc;
    return;
  }

  if (cond != NULL && cond->kind == STIGI_EXPR_UNOP && cond->u.unop.op == STIGI_OP_NOT) {
    ext_emit_cond_branch(ctx, cond->u.unop.operand, false_join_pt, true_join_pt,
                         out_false_sc, out_true_sc);
    return;
  }

  const char *pre_sc = ctx->cur_sc;
  const char *ts = ext_new_sc(ctx, pre_sc);
  const char *fs_sc = ext_new_sc(ctx, pre_sc);

  bool is_cmp = (cond != NULL && cond->kind == STIGI_EXPR_BINOP &&
                 (cond->u.binop.op == STIGI_OP_LT || cond->u.binop.op == STIGI_OP_LE ||
                  cond->u.binop.op == STIGI_OP_GT || cond->u.binop.op == STIGI_OP_GE ||
                  cond->u.binop.op == STIGI_OP_EQ || cond->u.binop.op == STIGI_OP_NE));

  if (is_cmp) {
    StigiOp cop = cond->u.binop.op;
    ExtVal lx = ext_expr(ctx, cond->u.binop.lhs, NULL, false);
    ExtVal ry = ext_expr(ctx, cond->u.binop.rhs, lx.ty, false);
    const char *cond_pt = ext_new_pt(ctx);
    const char *cond_op = ext_new_op(ctx);
    const char *a = lx.sym;
    const char *b = ry.sym;
    const char *cmp_s = "Equal";
    if (cop == STIGI_OP_LT) {
      cmp_s = "Less";
      ctx_emit3(ctx, "baseBelow", col_sym(a), col_sym(b), col_sym(ts));
      ctx_emit3(ctx, "baseLeq", col_sym(b), col_sym(a), col_sym(fs_sc));
    } else if (cop == STIGI_OP_LE) {
      cmp_s = "LessOrEqual";
      ctx_emit3(ctx, "baseLeq", col_sym(a), col_sym(b), col_sym(ts));
      ctx_emit3(ctx, "baseBelow", col_sym(b), col_sym(a), col_sym(fs_sc));
    } else if (cop == STIGI_OP_GT) {
      cmp_s = "Less";
      a = ry.sym;
      b = lx.sym;
      ctx_emit3(ctx, "baseBelow", col_sym(a), col_sym(b), col_sym(ts));
      ctx_emit3(ctx, "baseLeq", col_sym(b), col_sym(a), col_sym(fs_sc));
    } else if (cop == STIGI_OP_GE) {
      cmp_s = "LessOrEqual";
      a = ry.sym;
      b = lx.sym;
      ctx_emit3(ctx, "baseLeq", col_sym(a), col_sym(b), col_sym(ts));
      ctx_emit3(ctx, "baseBelow", col_sym(b), col_sym(a), col_sym(fs_sc));
    } else if (cop == STIGI_OP_EQ) {
      cmp_s = "Equal";
      ctx_emit3(ctx, "baseLeq", col_sym(a), col_sym(b), col_sym(ts));
      ctx_emit3(ctx, "baseLeq", col_sym(b), col_sym(a), col_sym(ts));
    } else if (cop == STIGI_OP_NE) {
      cmp_s = "NotEqual";
      ctx_emit3(ctx, "baseLeq", col_sym(a), col_sym(b), col_sym(fs_sc));
      ctx_emit3(ctx, "baseLeq", col_sym(b), col_sym(a), col_sym(fs_sc));
    }
    ctx_emit4(ctx, "operation", col_sym(cond_op), col_sym("Cond"), col_sym(cond_pt),
              col_sym(pre_sc));
    ctx_emit2(ctx, "operationOf", col_sym(cond_op), col_sym(ctx->inst_sym));
    ctx_emit6(ctx, "condition", col_sym(cond_op), col_sym(a), col_sym(cmp_s),
              col_sym(b), col_sym(ts), col_sym(fs_sc));
    ctx_emit2(ctx, "emitted", col_sym(cond_op), col_sym("Condition"));
    ctx_emit2(ctx, "emitted", col_sym(cond_op), col_sym("TrueEdge"));
    ctx_emit2(ctx, "emitted", col_sym(cond_op), col_sym("FalseEdge"));
    ctx_emit2(ctx, "trueEdge", col_sym(cond_op), col_sym(true_join_pt));
    ctx_emit2(ctx, "falseEdge", col_sym(cond_op), col_sym(false_join_pt));
    ctx_emit2(ctx, "next", col_sym(cond_pt), col_sym(true_join_pt));
    ctx_emit2(ctx, "next", col_sym(cond_pt), col_sym(false_join_pt));
  } else {
    (void)ext_expr(ctx, cond, "Bool", false);
    const char *cond_pt = ext_new_pt(ctx);
    const char *cond_op = ext_new_op(ctx);
    ctx_emit4(ctx, "operation", col_sym(cond_op), col_sym("CondBool"), col_sym(cond_pt),
              col_sym(pre_sc));
    ctx_emit2(ctx, "operationOf", col_sym(cond_op), col_sym(ctx->inst_sym));
    ctx_emit2(ctx, "emitted", col_sym(cond_op), col_sym("TrueEdge"));
    ctx_emit2(ctx, "emitted", col_sym(cond_op), col_sym("FalseEdge"));
    ctx_emit2(ctx, "trueEdge", col_sym(cond_op), col_sym(true_join_pt));
    ctx_emit2(ctx, "falseEdge", col_sym(cond_op), col_sym(false_join_pt));
    ctx_emit2(ctx, "next", col_sym(cond_pt), col_sym(true_join_pt));
    ctx_emit2(ctx, "next", col_sym(cond_pt), col_sym(false_join_pt));
  }

  if (out_true_sc != NULL) *out_true_sc = ts;
  if (out_false_sc != NULL) *out_false_sc = fs_sc;
}

/* ---- Snapshot/restore helper for vars across branches ---- */

static const char **snapshot_var_vals(ExtCtx *ctx, uint32_t n) {
  if (n == 0) return NULL;
  const char **snap = malloc((size_t)n * sizeof *snap);
  if (snap == NULL) {
    ctx->err = diag_set(ctx->diag, STIGI_ERR_OOM, -1, "extract snapshot OOM");
    return NULL;
  }
  for (uint32_t i = 0; i < n; i++) snap[i] = ctx->vars[i].val_sym;
  return snap;
}

static void restore_var_vals(ExtCtx *ctx, const char *const *snap, uint32_t n) {
  if (snap == NULL) return;
  for (uint32_t i = 0; i < n && i < ctx->var_count; i++) {
    ctx->vars[i].val_sym = snap[i];
  }
}

/* ---- Helper: emit IndexRead on an array value ---- */

static ExtVal emit_index_read_op(ExtCtx *ctx, const char *arr_sym, const char *len_sym,
                                 const char *elem_ty, ExtVal iv, bool emit_binder_read) {
  const char *pt = ext_new_pt(ctx);
  const char *op = ext_new_op(ctx);
  const char *rty = (elem_ty != NULL && elem_ty[0] != '\0') ? elem_ty : "U64";
  ExtVal rv = ext_new_val(ctx, rty);
  const char *ls = len_sym != NULL ? len_sym : ext_new_val(ctx, "U64").sym;
  ctx_emit4(ctx, "operation", col_sym(op), col_sym("IndexRead"), col_sym(pt), col_sym(ctx->cur_sc));
  ctx_emit2(ctx, "operationOf", col_sym(op), col_sym(ctx->inst_sym));
  ctx_emit4(ctx, "indexRead", col_sym(op), col_sym(iv.sym), col_sym(ls), col_sym(ctx->cur_sc));
  ctx_emit3(ctx, "operand", col_sym(op), col_i64(0), col_sym(arr_sym));
  ctx_emit3(ctx, "operand", col_sym(op), col_i64(1), col_sym(iv.sym));
  ctx_emit2(ctx, "result", col_sym(op), col_sym(rv.sym));
  ctx_emit2(ctx, "emitted", col_sym(op), col_sym("IndexRead"));
  ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Operand"));
  ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Result"));
  if (emit_binder_read) {
    ctx_emit2(ctx, "binderRead", col_sym(arr_sym), col_sym(pt));
  }
  ctx_emit2(ctx, "use", col_sym(pt), col_sym(arr_sym));
  ctx_emit2(ctx, "contentRead", col_sym(pt), col_sym(arr_sym));
  ctx_emit3(ctx, "access", col_sym(arr_sym), col_sym(iv.sym), col_sym(ctx->cur_sc));
  return rv;
}

/* ---- Helper: check whether an expression is an unused pure statement ---- */

static bool is_stmt_or_effect(const ExtCtx *ctx, const StigiExpr *e) {
  if (e == NULL) return true;
  switch (e->kind) {
    case STIGI_EXPR_VAL:
    case STIGI_EXPR_VAR:
    case STIGI_EXPR_ASSIGN:
    case STIGI_EXPR_INDEX_WRITE:
    case STIGI_EXPR_WHILE:
    case STIGI_EXPR_HANDLE:
    case STIGI_EXPR_ASSERT:
    case STIGI_EXPR_DENY:
    case STIGI_EXPR_LOCAL_DECL:
    case STIGI_EXPR_LIT_UNIT:
    case STIGI_EXPR_IF:
    case STIGI_EXPR_MATCH:
    case STIGI_EXPR_BLOCK:
      return true;
    case STIGI_EXPR_CALL: {
      if (e->u.call.callee->kind == STIGI_EXPR_LOCAL ||
          e->u.call.callee->kind == STIGI_EXPR_GLOBAL) {
        StigiStr cn = e->u.call.callee->u.ident.name;
        /* Capability op call or Unit/lemma call or Stigi.Test.expect is effectful/consumed */
        const char *dot = NULL;
        for (size_t i = 0; i < cn.len; i++) {
          if (cn.ptr[i] == '.') dot = cn.ptr + i;
        }
        if (dot != NULL) {
          StigiStr pfx = {cn.ptr, (size_t)(dot - cn.ptr)};
          if (ext_find_cap_by_binder((ExtCtx *)ctx, pfx) != NULL) return true;
          int32_t ctgt = stigi_image_resolve_name(ctx->img, ctx->uidx, pfx.ptr, pfx.len);
          if (ctgt >= 0 && ctx->img->units[ctgt].ast->decl->kind == STIGI_DECL_CAPABILITY) {
            return true;
          }
        }
        int32_t tgt = stigi_image_resolve_name(ctx->img, ctx->uidx, cn.ptr, cn.len);
        if (tgt >= 0) {
          const StigiDecl *cd = ctx->img->units[tgt].ast->decl;
          if (cd->kind == STIGI_DECL_LEMMA_DEF || cd->kind == STIGI_DECL_EXTERN) return true;
          if (cd->kind == STIGI_DECL_DEF) {
            if (cd->u.def_decl.head.ret_type != NULL &&
                str_eq_c(cd->u.def_decl.head.ret_type->name, "Unit")) {
              return true;
            }
            for (uint32_t g = 0; g < cd->u.def_decl.head.pre_group_count; g++) {
              if (!cd->u.def_decl.head.pre_groups[g].is_fact &&
                  cd->u.def_decl.head.pre_groups[g].cap_count > 0) {
                return true;
              }
            }
          }
        }
        if (str_eq_c(cn, "Stigi.Test.expect") || str_eq_c(cn, "Stigi.NegTest.expect")) {
          return true;
        }
      }
      return false;
    }
    default:
      return false;
  }
}

/* ---- Main expression extractor ---- */

static ExtVal ext_expr(ExtCtx *ctx, const StigiExpr *e, const char *hint_ty, bool in_tail) {
  if (ctx->err != STIGI_OK || e == NULL) return ext_new_val(ctx, "Unit");

  /* Exhaustive switch over StigiExprKind with NO default arm (S37) */
  switch (e->kind) {
    case STIGI_EXPR_LOCAL:
    case STIGI_EXPR_GLOBAL: {
      ExtVar *ev = ext_find_var(ctx, e->u.ident.name);
      const char *pt = ext_new_pt(ctx);
      const char *op = ext_new_op(ctx);
      ctx_emit4(ctx, "operation", col_sym(op), col_sym("Ref"), col_sym(pt), col_sym(ctx->cur_sc));
      ctx_emit2(ctx, "operationOf", col_sym(op), col_sym(ctx->inst_sym));
      ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Ref"));
      if (ev != NULL) {
        ctx_emit2(ctx, "binderRead", col_sym(ev->val_sym), col_sym(pt));
        ctx_emit2(ctx, "use", col_sym(pt), col_sym(ev->val_sym));
        ExtVal out = ext_val_zero(ev->val_sym, ev->type_sym != NULL ? ev->type_sym : "U64");
        out.is_array = ev->is_array;
        out.is_static = ev->is_static;
        out.len_sym = ev->len_sym;
        out.elem_ty = ev->elem_ty;
        return out;
      }
      /* Top-level compile-time val constant */
      int32_t uidx = stigi_image_resolve_name(ctx->img, ctx->uidx, e->u.ident.name.ptr,
                                              e->u.ident.name.len);
      if (uidx >= 0) {
        TupuI128 num;
        const char *vty = "U64";
        if (eval_top_val_int(ctx->img, (uint32_t)uidx, &num, &vty)) {
          return ext_lit_val(ctx, vty, num);
        }
      }
      /* Nullary enum constructor (qualified or unqualified) */
      uint32_t case_idx = 0;
      bool ambig = false;
      int32_t enum_idx = stigi_image_resolve_enum_case(ctx->img, ctx->uidx, e->u.ident.name.ptr,
                                                       e->u.ident.name.len, &case_idx, &ambig);
      if (enum_idx >= 0 && !ambig) {
        const StigiDecl *ed = ctx->img->units[enum_idx].ast->decl;
        const StigiEnumCase *ec = &ed->u.enum_decl.cases[case_idx];
        const char *ety = ctx_strdup(ctx, ctx->img->units[enum_idx].name,
                                     ctx->img->units[enum_idx].name_len);
        const char *cname = ctx_strdup(ctx, ec->name.ptr, ec->name.len);
        ExtVal out = ext_new_val(ctx, ety);
        ctx_emit2(ctx, "builtBy", col_sym(out.sym), col_sym(cname));
        return out;
      }
      ExtVal out = ext_new_val(ctx, hint_ty != NULL ? hint_ty : "U64");
      if (e->u.ident.name.len > 0 && e->u.ident.name.ptr[0] >= 'A' &&
          e->u.ident.name.ptr[0] <= 'Z') {
        const char *cs = ctx_strdup(ctx, e->u.ident.name.ptr, e->u.ident.name.len);
        ctx_emit2(ctx, "builtBy", col_sym(out.sym), col_sym(cs));
      }
      return out;
    }

    case STIGI_EXPR_LIT_INT: {
      const char *pt = ext_new_pt(ctx);
      const char *op = ext_new_op(ctx);
      const char *ty = is_int_type_name(hint_ty) ? hint_ty : "U64";
      ctx_emit4(ctx, "operation", col_sym(op), col_sym("Lit"), col_sym(pt), col_sym(ctx->cur_sc));
      ctx_emit2(ctx, "operationOf", col_sym(op), col_sym(ctx->inst_sym));
      ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Lit"));
      TupuI128 num = e->u.lit_int.overflow ? (TupuI128){0ULL, 1}
                                           : tupu_i128_from_u64(e->u.lit_int.value);
      return ext_lit_val(ctx, ty, num);
    }

    case STIGI_EXPR_LIT_CHAR: {
      const char *pt = ext_new_pt(ctx);
      const char *op = ext_new_op(ctx);
      uint8_t ch = e->u.lit_char.value;
      ctx_emit4(ctx, "operation", col_sym(op), col_sym("Lit"), col_sym(pt), col_sym(ctx->cur_sc));
      ctx_emit2(ctx, "operationOf", col_sym(op), col_sym(ctx->inst_sym));
      ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Lit"));
      return ext_lit_val(ctx, "U8", tupu_i128_from_u64((uint64_t)ch));
    }

    case STIGI_EXPR_LIT_STR: {
      const char *pt = ext_new_pt(ctx);
      const char *op = ext_new_op(ctx);
      ExtVal v = ext_new_val(ctx, "Array");
      uint64_t slen = decoded_str_len(e->u.lit_str.raw);
      ExtVal lv = ext_lit_val(ctx, "U64", tupu_i128_from_u64(slen));
      ctx_emit1(ctx, "staticObject", col_sym(v.sym));
      ctx_emit4(ctx, "operation", col_sym(op), col_sym("Lit"), col_sym(pt), col_sym(ctx->cur_sc));
      ctx_emit2(ctx, "operationOf", col_sym(op), col_sym(ctx->inst_sym));
      ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Lit"));
      v.is_array = true;
      v.is_static = true;
      v.len_sym = lv.sym;
      v.elem_ty = "U8";
      return v;
    }

    case STIGI_EXPR_LIT_BOOL:
    case STIGI_EXPR_LIT_UNIT: {
      const char *pt = ext_new_pt(ctx);
      const char *op = ext_new_op(ctx);
      const char *ty = (e->kind == STIGI_EXPR_LIT_BOOL) ? "Bool" : "Unit";
      ExtVal v = ext_new_val(ctx, ty);
      ctx_emit4(ctx, "operation", col_sym(op), col_sym("Lit"), col_sym(pt), col_sym(ctx->cur_sc));
      ctx_emit2(ctx, "operationOf", col_sym(op), col_sym(ctx->inst_sym));
      ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Lit"));
      return v;
    }

    case STIGI_EXPR_CALL: {
      /* 1. Array element read: localArray(index) */
      if (e->u.call.callee->kind == STIGI_EXPR_LOCAL && e->u.call.arg_count == 1) {
        ExtVar *ev = ext_find_var(ctx, e->u.call.callee->u.ident.name);
        if (ev != NULL && ev->is_array) {
          ExtVal iv = ext_expr(ctx, e->u.call.args[0], "U64", false);
          return emit_index_read_op(ctx, ev->val_sym, ev->len_sym,
                                    ev->elem_ty != NULL ? ev->elem_ty : hint_ty, iv, true);
        }
      }

      /* 2. Capability operation call: binder.op(args...) or unnamed Cap.op(args...) (S125) */
      if (e->u.call.callee->kind == STIGI_EXPR_GLOBAL ||
          e->u.call.callee->kind == STIGI_EXPR_LOCAL) {
        StigiStr cn = e->u.call.callee->u.ident.name;
        const char *dot = NULL;
        for (size_t i = 0; i < cn.len; i++) {
          if (cn.ptr[i] == '.') dot = cn.ptr + i;
        }
        if (dot != NULL) {
          StigiStr bname = {cn.ptr, (size_t)(dot - cn.ptr)};
          StigiStr opname = {dot + 1, cn.len - bname.len - 1U};
          ExtCap *cap = ext_find_cap_by_binder(ctx, bname);
          if (cap == NULL) {
            int32_t ctgt = stigi_image_resolve_name(ctx->img, ctx->uidx, bname.ptr, bname.len);
            if (ctgt >= 0 && ctx->img->units[ctgt].ast->decl->kind == STIGI_DECL_CAPABILITY) {
              cap = ext_find_cap_by_unit(ctx, ctx->img->units[ctgt].name);
            }
          }
          if (cap != NULL) {
            uint32_t argc = e->u.call.arg_count;
            const char **arg_vals = argc > 0 ? malloc((size_t)argc * sizeof *arg_vals) : NULL;
            if (argc > 0 && arg_vals == NULL) {
              ctx->err = diag_set(ctx->diag, STIGI_ERR_OOM, -1, "OOM");
              return ext_new_val(ctx, "Unit");
            }
            for (uint32_t i = 0; i < argc; i++) {
              arg_vals[i] = ext_expr(ctx, e->u.call.args[i], NULL, false).sym;
            }
            const char *pt = ext_new_pt(ctx);
            const char *op = ext_new_op(ctx);
            const char *op_full = ctx_fmt(ctx, "%s.%.*s", cap->unit_sym,
                                          (int)opname.len, opname.ptr);
            const char *rty = hint_ty != NULL ? hint_ty : "U64";
            int32_t cap_uidx = stigi_image_find(ctx->img, cap->unit_sym, strlen(cap->unit_sym));
            const StigiDecl *op_decl = NULL;
            if (cap_uidx >= 0 && ctx->img->units[cap_uidx].ast->decl->kind == STIGI_DECL_CAPABILITY) {
              const StigiDecl *cd = ctx->img->units[cap_uidx].ast->decl;
              for (uint32_t m = 0; m < cd->u.cap_decl.member_count; m++) {
                const StigiDecl *md = cd->u.cap_decl.members[m];
                if (md->kind == STIGI_DECL_DEF && md->name.len == opname.len &&
                    memcmp(md->name.ptr, opname.ptr, opname.len) == 0) {
                  op_decl = md;
                  if (md->u.def_decl.head.ret_type != NULL) {
                    rty = type_name_sym(ctx, md->u.def_decl.head.ret_type);
                  }
                  break;
                }
              }
            }
            ExtVal rv = ext_new_val(ctx, rty);
            ctx_emit4(ctx, "operation", col_sym(op), col_sym("CapCall"), col_sym(pt),
                      col_sym(ctx->cur_sc));
            ctx_emit2(ctx, "operationOf", col_sym(op), col_sym(ctx->inst_sym));
            ctx_emit4(ctx, "operationCall", col_sym(op), col_sym(cap->prov_sym),
                      col_sym(op_full), col_sym(pt));
            ctx_emit2(ctx, "result", col_sym(op), col_sym(rv.sym));
            ctx_emit2(ctx, "emitted", col_sym(op), col_sym("OperationCall"));
            ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Result"));
            ctx_emit2(ctx, "needsCapability", col_sym(op), col_sym(cap->unit_sym));
            ctx_emit2(ctx, "provider", col_sym(op), col_sym(cap->prov_sym));
            for (uint32_t i = 0; i < argc; i++) {
              ctx_emit3(ctx, "operand", col_sym(op), col_i64((int64_t)i), col_sym(arg_vals[i]));
              ctx_emit3(ctx, "callArg", col_sym(op), col_i64((int64_t)i), col_sym(arg_vals[i]));
            }
            if (op_decl != NULL) {
              const StigiDefHead *ch = &op_decl->u.def_decl.head;
              uint32_t scnt = argc < ch->param_count ? argc : ch->param_count;
              for (uint32_t g = 0; g < ch->pre_group_count; g++) {
                if (ch->pre_groups[g].is_fact) {
                  ext_emit_owed_subst(ctx, op, &ch->pre_groups[g].facts, ctx->cur_sc,
                                      "precondition-unproved", rv.sym, ch->params, arg_vals, scnt);
                }
              }
              for (uint32_t g = 0; g < ch->post_val_fact_count; g++) {
                ext_emit_owed_subst(ctx, op, &ch->post_val_facts[g], ctx->cur_sc,
                                    "precondition-unproved", rv.sym, ch->params, arg_vals, scnt);
              }
              for (uint32_t g = 0; g < ch->exit_fact_count; g++) {
                ext_emit_credits_subst(ctx, &ch->exit_facts[g], ctx->cur_sc, rv.sym,
                                       ch->params, arg_vals, scnt);
              }
            }
            free(arg_vals);
            return rv;
          }

          /* 2b. Trait operation call: Trait.op(args...) or ev.op(args...) via uKanren (S125) */
          int32_t tr_uidx = -1;
          const StigiType *ev_goal_ty = NULL;
          for (uint32_t li = 0; li < ctx->local_assump_count; li++) {
            if (ctx->local_assumps[li].name.len > 0 &&
                ctx->local_assumps[li].name.len == bname.len &&
                memcmp(ctx->local_assumps[li].name.ptr, bname.ptr, bname.len) == 0 &&
                ctx->local_assumps[li].cap_type != NULL) {
              int32_t rt = stigi_image_resolve_name(ctx->img, ctx->uidx,
                                                    ctx->local_assumps[li].cap_type->name.ptr,
                                                    ctx->local_assumps[li].cap_type->name.len);
              if (rt >= 0 && ctx->img->units[rt].ast->decl->kind == STIGI_DECL_TRAIT) {
                tr_uidx = rt;
                ev_goal_ty = ctx->local_assumps[li].cap_type;
                break;
              }
            }
          }
          if (tr_uidx < 0) {
            int32_t rt = stigi_image_resolve_name(ctx->img, ctx->uidx, bname.ptr, bname.len);
            if (rt >= 0 && ctx->img->units[rt].ast->decl->kind == STIGI_DECL_TRAIT) {
              tr_uidx = rt;
            }
          }
          if (tr_uidx >= 0) {
            const StigiDecl *td = ctx->img->units[tr_uidx].ast->decl;
            const StigiDecl *op_decl = NULL;
            for (uint32_t m = 0; m < td->u.trait_decl.member_count; m++) {
              const StigiDecl *md = td->u.trait_decl.members[m];
              if (md->kind == STIGI_DECL_DEF && md->name.len == opname.len &&
                  memcmp(md->name.ptr, opname.ptr, opname.len) == 0) {
                op_decl = md;
                break;
              }
            }
            uint32_t argc = e->u.call.arg_count;
            const char **arg_vals = argc > 0 ? malloc((size_t)argc * sizeof *arg_vals) : NULL;
            const char **arg_tys = argc > 0 ? malloc((size_t)argc * sizeof *arg_tys) : NULL;
            if (argc > 0 && (arg_vals == NULL || arg_tys == NULL)) {
              free(arg_vals); free(arg_tys);
              ctx->err = diag_set(ctx->diag, STIGI_ERR_OOM, -1, "OOM");
              return ext_new_val(ctx, "Unit");
            }
            for (uint32_t i = 0; i < argc; i++) {
              ExtVal av = ext_expr(ctx, e->u.call.args[i], NULL, false);
              arg_vals[i] = av.sym;
              arg_tys[i] = av.ty;
            }
            Arena ga;
            arena_init(&ga);
            const StigiType *goal_ty = ev_goal_ty;
            if (goal_ty == NULL) {
              uint32_t tpc = td->u.trait_decl.type_param_count;
              StigiType *gt = arena_alloc(&ga, sizeof *gt);
              if (gt != NULL) {
                memset(gt, 0, sizeof *gt);
                gt->kind = STIGI_TYPE_NAMED;
                gt->name = (StigiStr){ ctx->img->units[tr_uidx].name,
                                       (uint32_t)ctx->img->units[tr_uidx].name_len };
                gt->type_arg_count = tpc;
                if (tpc > 0) {
                  gt->type_args = arena_alloc(&ga, (size_t)tpc * sizeof *gt->type_args);
                  for (uint32_t tp = 0; tp < tpc && gt->type_args != NULL; tp++) {
                    if (tp < e->u.call.type_arg_count) {
                      gt->type_args[tp] = e->u.call.type_args[tp];
                    } else if (op_decl != NULL) {
                      StigiStr tpn = td->u.trait_decl.type_params[tp].name;
                      const StigiDefHead *oh = &op_decl->u.def_decl.head;
                      for (uint32_t p = 0; p < oh->param_count && p < argc; p++) {
                        if (oh->params[p].type != NULL &&
                            oh->params[p].type->kind == STIGI_TYPE_NAMED &&
                            oh->params[p].type->name.len == tpn.len &&
                            memcmp(oh->params[p].type->name.ptr, tpn.ptr, tpn.len) == 0 &&
                            arg_tys[p] != NULL) {
                          gt->type_args[tp] = ext_make_named_ast_type(&ga, arg_tys[p]);
                          break;
                        }
                      }
                    }
                  }
                }
                goal_ty = gt;
              }
            }
            ext_emit_given_lookup(ctx, goal_ty);
            arena_free(&ga);

            const char *pt = ext_new_pt(ctx);
            const char *op = ext_new_op(ctx);
            const char *rty = hint_ty != NULL ? hint_ty : "U64";
            if (op_decl != NULL && op_decl->u.def_decl.head.ret_type != NULL) {
              rty = type_name_sym(ctx, op_decl->u.def_decl.head.ret_type);
            }
            ExtVal rv = ext_new_val(ctx, rty);
            const char *callee_s = ctx_fmt(ctx, "%s.%.*s#0", ctx->img->units[tr_uidx].name,
                                           (int)opname.len, opname.ptr);
            ctx_emit4(ctx, "operation", col_sym(op), col_sym(argc > 0 ? "Call" : "Call0"),
                      col_sym(pt), col_sym(ctx->cur_sc));
            ctx_emit2(ctx, "operationOf", col_sym(op), col_sym(ctx->inst_sym));
            ctx_emit3(ctx, "callOf", col_sym(op), col_sym(callee_s), col_sym("Normal"));
            ctx_emit2(ctx, "result", col_sym(op), col_sym(rv.sym));
            ctx_emit2(ctx, "emitted", col_sym(op), col_sym("CallOf"));
            ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Result"));
            for (uint32_t i = 0; i < argc; i++) {
              ctx_emit3(ctx, "operand", col_sym(op), col_i64((int64_t)i), col_sym(arg_vals[i]));
              ctx_emit3(ctx, "callArg", col_sym(op), col_i64((int64_t)i), col_sym(arg_vals[i]));
              ctx_emit3(ctx, "argVia", col_sym(op), col_i64((int64_t)i), col_sym(arg_vals[i]));
            }
            if (argc > 0) {
              ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Operand"));
              ctx_emit2(ctx, "emitted", col_sym(op), col_sym("ArgVia"));
            }
            free(arg_vals);
            free(arg_tys);
            return rv;
          }
        }
      }

      /* 3. Builtin operations (Array.tryFill, Array.tryGrow, bit/shift/wrapping/narrowing/Test.expect) */
      if (e->u.call.callee->kind == STIGI_EXPR_GLOBAL ||
          e->u.call.callee->kind == STIGI_EXPR_LOCAL) {
        StigiStr cn = e->u.call.callee->u.ident.name;

        /* Array.tryFill{region}(cap, init) */
        if (str_eq_c(cn, "Array.tryFill") && e->u.call.arg_count == 2) {
          ExtVal capv = ext_expr(ctx, e->u.call.args[0], "U64", false);
          ExtVal initv = ext_expr(ctx, e->u.call.args[1], NULL, false);
          const char *pt = ext_new_pt(ctx);
          const char *op = ext_new_op(ctx);
          ExtVal rv = ext_new_val(ctx, "ArrayFillResult");
          const char *elem_ty = initv.ty != NULL ? initv.ty : "U64";
          const char *reg_sym = NULL;
          if (e->u.call.cap_arg_count > 0) {
            ExtCap *rc = ext_find_cap_by_binder(ctx, e->u.call.cap_args[0]);
            if (rc != NULL) reg_sym = rc->prov_sym;
          }
          if (reg_sym == NULL) {
            ExtCap *rc = ext_find_cap_by_unit(ctx, "Stigi.Region");
            if (rc != NULL) reg_sym = rc->prov_sym;
          }
          ctx_emit4(ctx, "operation", col_sym(op), col_sym("Fill"), col_sym(pt),
                    col_sym(ctx->cur_sc));
          ctx_emit2(ctx, "operationOf", col_sym(op), col_sym(ctx->inst_sym));
          ctx_emit2(ctx, "fillOp", col_sym(op), col_sym(elem_ty));
          ctx_emit2(ctx, "result", col_sym(op), col_sym(rv.sym));
          ctx_emit2(ctx, "emitted", col_sym(op), col_sym("FillOp"));
          ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Result"));
          ctx_emit2(ctx, "needsCapability", col_sym(op), col_sym("Stigi.Region"));
          if (reg_sym != NULL) {
            ctx_emit2(ctx, "provider", col_sym(op), col_sym(reg_sym));
          }
          rv.try_kind = TRY_FILL;
          rv.try_region = reg_sym;
          rv.try_cap = capv.sym;
          rv.try_elem_ty = elem_ty;
          return rv;
        }

        /* Array.tryGrow{region}(arr, nextCap, fill) */
        if (str_eq_c(cn, "Array.tryGrow") && e->u.call.arg_count == 3) {
          ExtVal arrv = ext_expr(ctx, e->u.call.args[0], NULL, false);
          ExtVal capv = ext_expr(ctx, e->u.call.args[1], "U64", false);
          ExtVal fillv = ext_expr(ctx, e->u.call.args[2], arrv.elem_ty, false);
          const char *pt = ext_new_pt(ctx);
          const char *op = ext_new_op(ctx);
          ExtVal rv = ext_new_val(ctx, "ArrayGrowResult");
          const char *reg_sym = NULL;
          if (e->u.call.cap_arg_count > 0) {
            ExtCap *rc = ext_find_cap_by_binder(ctx, e->u.call.cap_args[0]);
            if (rc != NULL) reg_sym = rc->prov_sym;
          }
          if (reg_sym == NULL) {
            ExtCap *rc = ext_find_cap_by_unit(ctx, "Stigi.Region");
            if (rc != NULL) reg_sym = rc->prov_sym;
          }
          ctx_emit4(ctx, "operation", col_sym(op), col_sym("Grow"), col_sym(pt),
                    col_sym(ctx->cur_sc));
          ctx_emit2(ctx, "operationOf", col_sym(op), col_sym(ctx->inst_sym));
          ctx_emit3(ctx, "operand", col_sym(op), col_i64(0), col_sym(arrv.sym));
          ctx_emit3(ctx, "operand", col_sym(op), col_i64(1), col_sym(capv.sym));
          ctx_emit3(ctx, "operand", col_sym(op), col_i64(2), col_sym(fillv.sym));
          ctx_emit2(ctx, "result", col_sym(op), col_sym(rv.sym));
          ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Operand"));
          ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Result"));
          ctx_emit2(ctx, "consume", col_sym(pt), col_sym(arrv.sym));
          if (e->u.call.args[0]->kind == STIGI_EXPR_LOCAL) {
            ExtVar *av = ext_find_var(ctx, e->u.call.args[0]->u.ident.name);
            if (av != NULL) av->param_consumed = true;
          }
          ctx_emit2(ctx, "needsCapability", col_sym(op), col_sym("Stigi.Region"));
          if (reg_sym != NULL) {
            ctx_emit2(ctx, "provider", col_sym(op), col_sym(reg_sym));
          }
          rv.try_kind = TRY_GROW;
          rv.try_region = reg_sym;
          rv.try_cap = capv.sym;
          rv.try_elem_ty = arrv.elem_ty != NULL ? arrv.elem_ty : fillv.ty;
          rv.try_old_obj = arrv.sym;
          rv.try_old_len = arrv.len_sym;
          return rv;
        }

        /* bitwiseAnd */
        if ((str_eq_c(cn, "bitwiseAnd") || str_eq_c(cn, "U64.bitwiseAnd") ||
             str_eq_c(cn, "U32.bitwiseAnd") || str_eq_c(cn, "U8.bitwiseAnd")) &&
            e->u.call.arg_count == 2) {
          ExtVal lv = ext_expr(ctx, e->u.call.args[0], hint_ty, false);
          ExtVal rv = ext_expr(ctx, e->u.call.args[1], lv.ty, false);
          const char *pt = ext_new_pt(ctx);
          const char *op = ext_new_op(ctx);
          ExtVal res = ext_new_val(ctx, lv.ty);
          ctx_emit4(ctx, "operation", col_sym(op), col_sym("And"), col_sym(pt),
                    col_sym(ctx->cur_sc));
          ctx_emit2(ctx, "operationOf", col_sym(op), col_sym(ctx->inst_sym));
          ctx_emit5(ctx, "andOp", col_sym(op), col_sym(res.sym), col_sym(lv.sym),
                    col_sym(rv.sym), col_sym(ctx->cur_sc));
          ctx_emit3(ctx, "operand", col_sym(op), col_i64(0), col_sym(lv.sym));
          ctx_emit3(ctx, "operand", col_sym(op), col_i64(1), col_sym(rv.sym));
          ctx_emit2(ctx, "result", col_sym(op), col_sym(res.sym));
          ctx_emit2(ctx, "emitted", col_sym(op), col_sym("AndOp"));
          ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Operand"));
          ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Result"));
          return res;
        }

        /* shiftLeft / shiftRight / rotateLeft / rotateRight */
        if ((str_eq_c(cn, "shiftLeft") || str_eq_c(cn, "shiftRight") ||
             str_eq_c(cn, "rotateLeft") || str_eq_c(cn, "rotateRight") ||
             str_eq_c(cn, "U64.shiftLeft") || str_eq_c(cn, "U64.shiftRight") ||
             str_eq_c(cn, "U32.shiftLeft") || str_eq_c(cn, "U32.shiftRight") ||
             str_eq_c(cn, "U32.rotateRight")) &&
            e->u.call.arg_count == 2) {
          const char *sty = (cn.len >= 4 && memcmp(cn.ptr, "U32.", 4) == 0) ? "U32" : "U64";
          ExtVal lv = ext_expr(ctx, e->u.call.args[0], sty, false);
          ExtVal rv = ext_expr(ctx, e->u.call.args[1], sty, false);
          const char *pt = ext_new_pt(ctx);
          const char *op = ext_new_op(ctx);
          ExtVal res = ext_new_val(ctx, lv.ty);
          ctx_emit4(ctx, "operation", col_sym(op), col_sym("Shift"), col_sym(pt),
                    col_sym(ctx->cur_sc));
          ctx_emit2(ctx, "operationOf", col_sym(op), col_sym(ctx->inst_sym));
          ctx_emit5(ctx, "shiftOp", col_sym(op), col_sym(res.sym), col_sym(lv.sym),
                    col_sym(rv.sym), col_sym(ctx->cur_sc));
          ctx_emit3(ctx, "operand", col_sym(op), col_i64(0), col_sym(lv.sym));
          ctx_emit3(ctx, "operand", col_sym(op), col_i64(1), col_sym(rv.sym));
          ctx_emit2(ctx, "result", col_sym(op), col_sym(res.sym));
          ctx_emit2(ctx, "emitted", col_sym(op), col_sym("ShiftOp"));
          ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Operand"));
          ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Result"));
          return res;
        }

        /* Unary wrapping hash finalizer U64.mix (no overflow obligation, arith.dl:4) */
        if (str_eq_c(cn, "U64.mix") && e->u.call.arg_count == 1) {
          ExtVal av = ext_expr(ctx, e->u.call.args[0], "U64", false);
          const char *pt = ext_new_pt(ctx);
          const char *op = ext_new_op(ctx);
          ExtVal res = ext_new_val(ctx, "U64");
          ctx_emit4(ctx, "operation", col_sym(op), col_sym("WrapOp"), col_sym(pt),
                    col_sym(ctx->cur_sc));
          ctx_emit2(ctx, "operationOf", col_sym(op), col_sym(ctx->inst_sym));
          ctx_emit3(ctx, "operand", col_sym(op), col_i64(0), col_sym(av.sym));
          ctx_emit2(ctx, "result", col_sym(op), col_sym(res.sym));
          ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Operand"));
          ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Result"));
          return res;
        }

        /* Wrapping & bitwise-or/xor ops (no overflow obligation, arith.dl:4) */
        if ((str_eq_c(cn, "U32.wrappingAdd") || str_eq_c(cn, "U32.wrappingSubtract") ||
             str_eq_c(cn, "U32.wrappingMultiply") ||
             str_eq_c(cn, "U64.wrappingAdd") || str_eq_c(cn, "U64.wrappingSubtract") ||
             str_eq_c(cn, "U64.wrappingMultiply") ||
             str_eq_c(cn, "bitwiseOr") || str_eq_c(cn, "bitwiseXor") ||
             str_eq_c(cn, "U64.bitwiseOr") || str_eq_c(cn, "U64.bitwiseXor") ||
             str_eq_c(cn, "U32.bitwiseOr") || str_eq_c(cn, "U32.bitwiseXor")) &&
            e->u.call.arg_count == 2) {
          const char *wty = (cn.len >= 4 && memcmp(cn.ptr, "U32.", 4) == 0) ? "U32" : "U64";
          ExtVal lv = ext_expr(ctx, e->u.call.args[0], wty, false);
          ExtVal rv = ext_expr(ctx, e->u.call.args[1], lv.ty, false);
          const char *pt = ext_new_pt(ctx);
          const char *op = ext_new_op(ctx);
          ExtVal res = ext_new_val(ctx, lv.ty);
          ctx_emit4(ctx, "operation", col_sym(op), col_sym("WrapOp"), col_sym(pt),
                    col_sym(ctx->cur_sc));
          ctx_emit2(ctx, "operationOf", col_sym(op), col_sym(ctx->inst_sym));
          ctx_emit3(ctx, "operand", col_sym(op), col_i64(0), col_sym(lv.sym));
          ctx_emit3(ctx, "operand", col_sym(op), col_i64(1), col_sym(rv.sym));
          ctx_emit2(ctx, "result", col_sym(op), col_sym(res.sym));
          ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Operand"));
          ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Result"));
          if (str_eq_c(cn, "U32.bitwiseAnd") || str_eq_c(cn, "U64.bitwiseAnd")) {
            ctx_emit3(ctx, "baseLeq", col_sym(res.sym), col_sym(rv.sym), col_sym(ctx->cur_sc));
          }
          return res;
        }

        /* Narrowing / bit-preserving integer conversions */
        if ((str_eq_c(cn, "U8.ofU64") || str_eq_c(cn, "U32.ofU64") ||
             str_eq_c(cn, "U64.ofI64") || str_eq_c(cn, "U64.ofI32") ||
             str_eq_c(cn, "U64.ofU8") || str_eq_c(cn, "U64.ofU32") ||
             str_eq_c(cn, "I64.ofU64") || str_eq_c(cn, "I32.ofU64")) &&
            e->u.call.arg_count == 1) {
          ExtVal av = ext_expr(ctx, e->u.call.args[0], NULL, false);
          const char *pt = ext_new_pt(ctx);
          const char *op = ext_new_op(ctx);
          const char *rty = "U64";
          if (str_eq_c(cn, "U8.ofU64")) rty = "U8";
          else if (str_eq_c(cn, "U32.ofU64")) rty = "U32";
          else if (str_eq_c(cn, "I64.ofU64")) rty = "I64";
          else if (str_eq_c(cn, "I32.ofU64")) rty = "I32";
          ExtVal res = ext_new_val(ctx, rty);
          ctx_emit4(ctx, "operation", col_sym(op), col_sym("Narrow"), col_sym(pt),
                    col_sym(ctx->cur_sc));
          ctx_emit2(ctx, "operationOf", col_sym(op), col_sym(ctx->inst_sym));
          ctx_emit3(ctx, "operand", col_sym(op), col_i64(0), col_sym(av.sym));
          ctx_emit2(ctx, "result", col_sym(op), col_sym(res.sym));
          ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Operand"));
          ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Result"));
          if (str_eq_c(cn, "U8.ofU64")) {
            ExtVal lim = ext_lit_val(ctx, "U64", tupu_i128_from_u64(256ULL));
            ctx_emit5(ctx, "belowOwed", col_sym(op), col_sym(av.sym), col_sym(lim.sym),
                      col_sym(ctx->cur_sc), col_sym("precondition-unproved"));
          } else if (str_eq_c(cn, "U32.ofU64")) {
            ExtVal lim = ext_lit_val(ctx, "U64", tupu_i128_from_u64(4294967296ULL));
            ctx_emit5(ctx, "belowOwed", col_sym(op), col_sym(av.sym), col_sym(lim.sym),
                      col_sym(ctx->cur_sc), col_sym("precondition-unproved"));
          } else if (str_eq_c(cn, "U64.ofI64") || str_eq_c(cn, "U64.ofI32")) {
            ExtVal z = ext_lit_val(ctx, str_eq_c(cn, "U64.ofI32") ? "I32" : "I64",
                                   tupu_i128_from_i64(0));
            ctx_emit5(ctx, "leqOwed", col_sym(op), col_sym(z.sym), col_sym(av.sym),
                      col_sym(ctx->cur_sc), col_sym("precondition-unproved"));
          } else if (str_eq_c(cn, "U64.ofU8")) {
            ExtVal lim = ext_lit_val(ctx, "U64", tupu_i128_from_u64(256ULL));
            ctx_emit3(ctx, "baseBelow", col_sym(res.sym), col_sym(lim.sym), col_sym(ctx->cur_sc));
          } else if (str_eq_c(cn, "U64.ofU32")) {
            ExtVal lim = ext_lit_val(ctx, "U64", tupu_i128_from_u64(4294967296ULL));
            ctx_emit3(ctx, "baseBelow", col_sym(res.sym), col_sym(lim.sym), col_sym(ctx->cur_sc));
          }
          if (!str_eq_c(cn, "I64.ofU64") && !str_eq_c(cn, "I32.ofU64")) {
            ctx_emit3(ctx, "baseLeq", col_sym(av.sym), col_sym(res.sym), col_sym(ctx->cur_sc));
            ctx_emit3(ctx, "baseLeq", col_sym(res.sym), col_sym(av.sym), col_sym(ctx->cur_sc));
          }
          return res;
        }

        /* Stigi.Test.expect */
        if (str_eq_c(cn, "Stigi.Test.expect") && e->u.call.arg_count == 3) {
          ExtVal g = ext_expr(ctx, e->u.call.args[0], NULL, false);
          ExtVal w = ext_expr(ctx, e->u.call.args[1], g.ty, false);
          ExtVal r = ext_expr(ctx, e->u.call.args[2], NULL, false);
          const char *pt = ext_new_pt(ctx);
          const char *op = ext_new_op(ctx);
          ExtVal rv = ext_new_val(ctx, "Stigi.Test.Result");
          ctx_emit4(ctx, "operation", col_sym(op), col_sym("Expect"), col_sym(pt),
                    col_sym(ctx->cur_sc));
          ctx_emit2(ctx, "operationOf", col_sym(op), col_sym(ctx->inst_sym));
          ctx_emit3(ctx, "operand", col_sym(op), col_i64(0), col_sym(g.sym));
          ctx_emit3(ctx, "operand", col_sym(op), col_i64(1), col_sym(w.sym));
          ctx_emit3(ctx, "operand", col_sym(op), col_i64(2), col_sym(r.sym));
          ctx_emit2(ctx, "result", col_sym(op), col_sym(rv.sym));
          ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Operand"));
          ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Result"));
          ctx_emit2(ctx, "statesFact", col_sym(ctx->unit_sym), col_sym("Expectation"));
          return rv;
        }
      }

      /* 4. Enum constructor call: CaseName(args...) or Enum.CaseName(args...) */
      if (e->u.call.callee->kind == STIGI_EXPR_LOCAL ||
          e->u.call.callee->kind == STIGI_EXPR_GLOBAL) {
        StigiStr cn = e->u.call.callee->u.ident.name;
        uint32_t case_idx = 0;
        bool ambig = false;
        int32_t enum_idx = stigi_image_resolve_enum_case(ctx->img, ctx->uidx, cn.ptr, cn.len,
                                                         &case_idx, &ambig);
        if (enum_idx >= 0 && !ambig) {
          const StigiDecl *ed = ctx->img->units[enum_idx].ast->decl;
          const StigiEnumCase *ec = &ed->u.enum_decl.cases[case_idx];
          uint32_t argc = e->u.call.arg_count;
          const char **arg_vals = argc > 0 ? malloc((size_t)argc * sizeof *arg_vals) : NULL;
          if (argc > 0 && arg_vals == NULL) {
            ctx->err = diag_set(ctx->diag, STIGI_ERR_OOM, -1, "OOM");
            return ext_new_val(ctx, "Unit");
          }
          for (uint32_t i = 0; i < argc; i++) {
            const char *fh = (i < ec->field_count) ? type_name_sym(ctx, ec->fields[i].type) : NULL;
            arg_vals[i] = ext_expr(ctx, e->u.call.args[i], fh, false).sym;
          }
          const char *pt = ext_new_pt(ctx);
          const char *op = ext_new_op(ctx);
          const char *ety = ctx_strdup(ctx, ctx->img->units[enum_idx].name,
                                       ctx->img->units[enum_idx].name_len);
          const char *cname = ctx_strdup(ctx, ec->name.ptr, ec->name.len);
          ExtVal rv = ext_new_val(ctx, ety);
          ctx_emit4(ctx, "operation", col_sym(op), col_sym("Ctor"), col_sym(pt),
                    col_sym(ctx->cur_sc));
          ctx_emit2(ctx, "operationOf", col_sym(op), col_sym(ctx->inst_sym));
          ctx_emit2(ctx, "result", col_sym(op), col_sym(rv.sym));
          ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Result"));
          ctx_emit2(ctx, "builtBy", col_sym(rv.sym), col_sym(cname));
          for (uint32_t i = 0; i < argc; i++) {
            ctx_emit3(ctx, "operand", col_sym(op), col_i64((int64_t)i), col_sym(arg_vals[i]));
            if (i < ec->field_count) {
              uint32_t ruidx = 0;
              const StigiDecl *rt = find_refined_type_decl(ctx->img, (uint32_t)enum_idx,
                                                           ec->fields[i].type, &ruidx, 0);
              if (rt != NULL) {
                ext_emit_refined_owed(ctx, op, rt, ruidx, ctx->cur_sc, arg_vals[i]);
              }
            }
          }
          if (ec->has_facts) {
            uint32_t scnt = argc < ec->field_count ? argc : ec->field_count;
            uint32_t saved_u = ctx->uidx;
            ctx->uidx = (uint32_t)enum_idx;
            ext_emit_owed_subst(ctx, op, &ec->facts, ctx->cur_sc, "precondition-unproved",
                                rv.sym, ec->fields, arg_vals, scnt);
            ctx->uidx = saved_u;
          }
          free(arg_vals);
          return rv;
        }
      }

      /* 5. Normal def / lemma / extern call */
      uint32_t argc = e->u.call.arg_count;
      const char **arg_vals = argc > 0 ? malloc((size_t)argc * sizeof *arg_vals) : NULL;
      const char **arg_tys = argc > 0 ? malloc((size_t)argc * sizeof *arg_tys) : NULL;
      if (argc > 0 && (arg_vals == NULL || arg_tys == NULL)) {
        free(arg_vals); free(arg_tys);
        ctx->err = diag_set(ctx->diag, STIGI_ERR_OOM, -1, "OOM");
        return ext_new_val(ctx, "Unit");
      }

      int32_t tgt = -1;
      if (e->u.call.callee->kind == STIGI_EXPR_LOCAL ||
          e->u.call.callee->kind == STIGI_EXPR_GLOBAL) {
        StigiStr cn = e->u.call.callee->u.ident.name;
        tgt = stigi_image_resolve_name(ctx->img, ctx->uidx, cn.ptr, cn.len);
      }

      const StigiDefHead *ch = NULL;
      if (tgt >= 0) {
        const StigiDecl *cd = ctx->img->units[tgt].ast->decl;
        if (cd->kind == STIGI_DECL_DEF || cd->kind == STIGI_DECL_LEMMA_DEF) {
          ch = &cd->u.def_decl.head;
        } else if (cd->kind == STIGI_DECL_EXTERN && !cd->u.extern_decl.is_val) {
          ch = &cd->u.extern_decl.head;
        }
      }

      for (uint32_t i = 0; i < argc; i++) {
        const char *ah = (ch != NULL && i < ch->param_count)
                             ? type_name_sym(ctx, ch->params[i].type)
                             : NULL;
        ExtVal av = ext_expr(ctx, e->u.call.args[i], ah, false);
        arg_vals[i] = av.sym;
        arg_tys[i] = av.ty;
      }

      const char *pt = ext_new_pt(ctx);
      const char *op = ext_new_op(ctx);
      const char *ret_ty = hint_ty != NULL ? hint_ty : "U64";
      const char *callee_inst = "unknown#0";
      bool callee_is_def = false;
      if (tgt >= 0) {
        callee_inst = ctx_fmt(ctx, "%s#0", ctx->img->units[tgt].name);
        const StigiDecl *cd = ctx->img->units[tgt].ast->decl;
        if (cd->kind == STIGI_DECL_DEF || cd->kind == STIGI_DECL_LEMMA_DEF) {
          callee_is_def = true;
          if (cd->u.def_decl.head.ret_type != NULL) {
            ret_ty = type_name_sym(ctx, cd->u.def_decl.head.ret_type);
          }
        } else if (cd->kind == STIGI_DECL_EXTERN && !cd->u.extern_decl.is_val &&
                   cd->u.extern_decl.head.ret_type != NULL) {
          ret_ty = type_name_sym(ctx, cd->u.extern_decl.head.ret_type);
        }
      } else if (e->u.call.callee->kind == STIGI_EXPR_LOCAL ||
                 e->u.call.callee->kind == STIGI_EXPR_GLOBAL) {
        StigiStr cn = e->u.call.callee->u.ident.name;
        callee_inst = ctx_fmt(ctx, "%.*s#0", (int)cn.len, cn.ptr);
      }

      ExtVal rv = ext_new_val(ctx, ret_ty);
      const char *call_kind = argc > 0 ? "Call" : "Call0";
      /* True tail position ONLY (never blindly mark non-tail self-calls as Tail!) */
      const char *cg_kind = (in_tail && callee_is_def && tgt >= 0 &&
                             ctx->img->units[tgt].ast->decl->kind == STIGI_DECL_DEF)
                                ? "Tail"
                                : "Normal";
      ctx_emit4(ctx, "operation", col_sym(op), col_sym(call_kind), col_sym(pt),
                col_sym(ctx->cur_sc));
      ctx_emit2(ctx, "operationOf", col_sym(op), col_sym(ctx->inst_sym));
      ctx_emit3(ctx, "callOf", col_sym(op), col_sym(callee_inst), col_sym(cg_kind));
      ctx_emit2(ctx, "result", col_sym(op), col_sym(rv.sym));
      ctx_emit2(ctx, "emitted", col_sym(op), col_sym("CallOf"));
      ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Result"));
      for (uint32_t i = 0; i < argc; i++) {
        ctx_emit3(ctx, "operand", col_sym(op), col_i64((int64_t)i), col_sym(arg_vals[i]));
        ctx_emit3(ctx, "callArg", col_sym(op), col_i64((int64_t)i), col_sym(arg_vals[i]));
        ctx_emit3(ctx, "argVia", col_sym(op), col_i64((int64_t)i), col_sym(arg_vals[i]));
      }
      if (argc > 0) {
        ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Operand"));
        ctx_emit2(ctx, "emitted", col_sym(op), col_sym("ArgVia"));
      }

      /* Recursive progressStep emission (direct or mutual SCC) when enclosing def states decreases(m) */
      if (ctx->def_dec_site != NULL && ctx->def_dec_param_idx >= 0 && tgt >= 0 && ch != NULL) {
        int32_t cal_dec_idx = -1;
        if ((uint32_t)tgt == ctx->uidx) {
          cal_dec_idx = ctx->def_dec_param_idx;
        } else {
          const StigiExpr *cdec = ch->decreases;
          for (uint32_t g = 0; g < ch->pre_group_count && cdec == NULL; g++) {
            if (ch->pre_groups[g].is_fact) cdec = find_decreases_in_fg(&ch->pre_groups[g].facts);
          }
          for (uint32_t g = 0; g < ch->post_val_fact_count && cdec == NULL; g++) {
            cdec = find_decreases_in_fg(&ch->post_val_facts[g]);
          }
          if (cdec != NULL && (cdec->kind == STIGI_EXPR_LOCAL || cdec->kind == STIGI_EXPR_GLOBAL)) {
            for (uint32_t pi = 0; pi < ch->param_count; pi++) {
              if (ch->params[pi].name.len == cdec->u.ident.name.len &&
                  memcmp(ch->params[pi].name.ptr, cdec->u.ident.name.ptr,
                         cdec->u.ident.name.len) == 0) {
                cal_dec_idx = (int32_t)pi;
                break;
              }
            }
          }
        }
        if (cal_dec_idx >= 0 && (uint32_t)cal_dec_idx < argc) {
          ctx_emit5(ctx, "progressStep", col_sym(ctx->def_dec_site), col_sym(op),
                    col_sym(ctx->def_dec_val), col_sym(arg_vals[cal_dec_idx]),
                    col_sym(ctx->cur_sc));
        }
      }

      /* Callee preconditions, refinements, exit credits, capability needs/providers */
      if (ch != NULL) {
        uint32_t scnt = argc < ch->param_count ? argc : ch->param_count;
        for (uint32_t i = 0; i < scnt; i++) {
          uint32_t ruidx = 0;
          const StigiDecl *rt = find_refined_type_decl(ctx->img, (uint32_t)tgt,
                                                       ch->params[i].type, &ruidx, 0);
          if (rt != NULL) {
            ext_emit_refined_owed(ctx, op, rt, ruidx, ctx->cur_sc, arg_vals[i]);
          }
        }
        for (uint32_t g = 0; g < ch->pre_group_count; g++) {
          if (ch->pre_groups[g].is_fact) {
            uint32_t saved_u = ctx->uidx;
            ctx->uidx = (uint32_t)tgt;
            ext_emit_owed_subst(ctx, op, &ch->pre_groups[g].facts, ctx->cur_sc,
                                "precondition-unproved", rv.sym, ch->params, arg_vals, scnt);
            ctx->uidx = saved_u;
          } else {
            for (uint32_t c = 0; c < ch->pre_groups[g].cap_count; c++) {
              const StigiType *cap_ty = ch->pre_groups[g].caps[c].cap_type;
              StigiStr cap_n = cap_ty->name;
              int32_t cidx = stigi_image_resolve_name(ctx->img, (uint32_t)tgt, cap_n.ptr, cap_n.len);
              if (cidx >= 0 && ctx->img->units[cidx].ast->decl->kind == STIGI_DECL_TRAIT) {
                /* S125: implicit trait constraint resolved via Bezem-bounded uKanren */
                Arena ga;
                arena_init(&ga);
                uint32_t tpc = ch->type_param_count;
                const StigiType **inst_targs = tpc > 0 ? arena_alloc(&ga, (size_t)tpc * sizeof *inst_targs) : NULL;
                for (uint32_t tp = 0; tp < tpc && inst_targs != NULL; tp++) {
                  if (tp < e->u.call.type_arg_count) {
                    inst_targs[tp] = e->u.call.type_args[tp];
                  } else {
                    inst_targs[tp] = NULL;
                    StigiStr tpn = ch->type_params[tp].name;
                    for (uint32_t p = 0; p < ch->param_count && p < argc; p++) {
                      if (ch->params[p].type != NULL &&
                          ch->params[p].type->kind == STIGI_TYPE_NAMED &&
                          ch->params[p].type->name.len == tpn.len &&
                          memcmp(ch->params[p].type->name.ptr, tpn.ptr, tpn.len) == 0 &&
                          arg_tys[p] != NULL) {
                        inst_targs[tp] = ext_make_named_ast_type(&ga, arg_tys[p]);
                        break;
                      }
                    }
                  }
                }
                StigiType *goal_ty = ext_subst_ast_type(&ga, cap_ty, ch->type_params, tpc,
                                                        inst_targs, tpc);
                if (goal_ty != NULL && goal_ty->kind == STIGI_TYPE_NAMED) {
                  goal_ty->name = (StigiStr){ ctx->img->units[cidx].name,
                                              (uint32_t)ctx->img->units[cidx].name_len };
                }
                ext_emit_given_lookup(ctx, goal_ty);
                arena_free(&ga);
              } else {
                const char *cap_s = (cidx >= 0) ? ctx_strdup(ctx, ctx->img->units[cidx].name,
                                                             ctx->img->units[cidx].name_len)
                                                : ctx_strdup(ctx, cap_n.ptr, cap_n.len);
                ctx_emit2(ctx, "needsCapability", col_sym(op), col_sym(cap_s));
                ExtCap *prov = ext_find_cap_by_unit(ctx, cap_s);
                if (prov != NULL) {
                  ctx_emit2(ctx, "provider", col_sym(op), col_sym(prov->prov_sym));
                }
              }
            }
          }
        }
        for (uint32_t g = 0; g < ch->post_val_fact_count; g++) {
          uint32_t saved_u = ctx->uidx;
          ctx->uidx = (uint32_t)tgt;
          ext_emit_owed_subst(ctx, op, &ch->post_val_facts[g], ctx->cur_sc,
                              "precondition-unproved", rv.sym, ch->params, arg_vals, scnt);
          ctx->uidx = saved_u;
        }
        for (uint32_t g = 0; g < ch->exit_fact_count; g++) {
          uint32_t saved_u = ctx->uidx;
          ctx->uidx = (uint32_t)tgt;
          ext_emit_credits_subst(ctx, &ch->exit_facts[g], ctx->cur_sc, rv.sym,
                                 ch->params, arg_vals, scnt);
          ctx->uidx = saved_u;
        }
        if (ch->ret_type != NULL) {
          uint32_t ruidx = 0;
          const StigiDecl *rt = find_refined_type_decl(ctx->img, (uint32_t)tgt,
                                                       ch->ret_type, &ruidx, 0);
          if (rt != NULL) {
            ext_emit_refined_credits(ctx, rt, ruidx, ctx->cur_sc, rv.sym);
          }
        }
      }
      free(arg_vals);
      free(arg_tys);
      return rv;
    }

    case STIGI_EXPR_BINOP: {
      ExtVal lv = ext_expr(ctx, e->u.binop.lhs, hint_ty, false);
      ExtVal rv = ext_expr(ctx, e->u.binop.rhs, lv.ty, false);
      const char *pt = ext_new_pt(ctx);
      const char *op = ext_new_op(ctx);
      const char *res_ty = (e->u.binop.op >= STIGI_OP_LT && e->u.binop.op <= STIGI_OP_OR)
                               ? "Bool"
                               : (lv.ty != NULL ? lv.ty : (hint_ty != NULL ? hint_ty : "U64"));
      ExtVal res = ext_new_val(ctx, res_ty);
      const char *kind_s = "Add";
      const char *rel_s = "addOp";
      const char *row_s = "AddOp";
      bool is_arith = true;
      switch (e->u.binop.op) {
        case STIGI_OP_ADD: kind_s = "Add"; rel_s = "addOp"; row_s = "AddOp"; break;
        case STIGI_OP_SUB: kind_s = "Sub"; rel_s = "subtractOp"; row_s = "SubtractOp"; break;
        case STIGI_OP_MUL: kind_s = "Mul"; rel_s = "multiplyOp"; row_s = "MultiplyOp"; break;
        case STIGI_OP_DIV: kind_s = "Div"; rel_s = "quotientOp"; row_s = "QuotientOp"; break;
        case STIGI_OP_MOD: kind_s = "Mod"; rel_s = "remainderOp"; row_s = "RemainderOp"; break;
        default: is_arith = false; break;
      }
      if (is_arith) {
        const char *op_lhs = lv.sym;
        const char *op_rhs = rv.sym;
        /* Canonicalize commutative `lit + x` to `addOp(op, res, x, lit, sc)` so bounds.dl
         * successor and offset rules (`addOp(_, a, x, k, _), lit(k, ...)`) match both orders. */
        if (e->u.binop.op == STIGI_OP_ADD &&
            e->u.binop.lhs->kind == STIGI_EXPR_LIT_INT &&
            e->u.binop.rhs->kind != STIGI_EXPR_LIT_INT) {
          op_lhs = rv.sym;
          op_rhs = lv.sym;
        }
        ctx_emit4(ctx, "operation", col_sym(op), col_sym(kind_s), col_sym(pt),
                  col_sym(ctx->cur_sc));
        ctx_emit2(ctx, "operationOf", col_sym(op), col_sym(ctx->inst_sym));
        ctx_emit5(ctx, rel_s, col_sym(op), col_sym(res.sym), col_sym(op_lhs),
                  col_sym(op_rhs), col_sym(ctx->cur_sc));
        ctx_emit3(ctx, "operand", col_sym(op), col_i64(0), col_sym(op_lhs));
        ctx_emit3(ctx, "operand", col_sym(op), col_i64(1), col_sym(op_rhs));
        ctx_emit2(ctx, "result", col_sym(op), col_sym(res.sym));
        ctx_emit2(ctx, "emitted", col_sym(op), col_sym(row_s));
        ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Operand"));
        ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Result"));
      } else {
        ctx_emit4(ctx, "operation", col_sym(op), col_sym("Ref"), col_sym(pt),
                  col_sym(ctx->cur_sc));
        ctx_emit2(ctx, "operationOf", col_sym(op), col_sym(ctx->inst_sym));
        ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Ref"));
      }
      return res;
    }

    case STIGI_EXPR_UNOP: {
      /* Exact negative literal: emit direct negative TupuI128 so INT_MIN fits cleanly */
      if (e->u.unop.op == STIGI_OP_NEG && e->u.unop.operand != NULL &&
          e->u.unop.operand->kind == STIGI_EXPR_LIT_INT &&
          !e->u.unop.operand->u.lit_int.overflow) {
        const char *pt = ext_new_pt(ctx);
        const char *op = ext_new_op(ctx);
        const char *ty = is_int_type_name(hint_ty) ? hint_ty : "I64";
        TupuI128 num = i128_neg_u64(e->u.unop.operand->u.lit_int.value);
        ctx_emit4(ctx, "operation", col_sym(op), col_sym("Lit"), col_sym(pt),
                  col_sym(ctx->cur_sc));
        ctx_emit2(ctx, "operationOf", col_sym(op), col_sym(ctx->inst_sym));
        ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Lit"));
        return ext_lit_val(ctx, ty, num);
      }

      ExtVal opnd = ext_expr(ctx, e->u.unop.operand, hint_ty, false);
      const char *pt = ext_new_pt(ctx);
      const char *op = ext_new_op(ctx);
      if (e->u.unop.op == STIGI_OP_NEG) {
        const char *ty = opnd.ty != NULL ? opnd.ty : "I64";
        ExtVal zero = ext_lit_val(ctx, ty, tupu_i128_from_i64(0));
        ExtVal res = ext_new_val(ctx, ty);
        ctx_emit4(ctx, "operation", col_sym(op), col_sym("Sub"), col_sym(pt),
                  col_sym(ctx->cur_sc));
        ctx_emit2(ctx, "operationOf", col_sym(op), col_sym(ctx->inst_sym));
        ctx_emit5(ctx, "subtractOp", col_sym(op), col_sym(res.sym), col_sym(zero.sym),
                  col_sym(opnd.sym), col_sym(ctx->cur_sc));
        ctx_emit3(ctx, "operand", col_sym(op), col_i64(0), col_sym(zero.sym));
        ctx_emit3(ctx, "operand", col_sym(op), col_i64(1), col_sym(opnd.sym));
        ctx_emit2(ctx, "result", col_sym(op), col_sym(res.sym));
        ctx_emit2(ctx, "emitted", col_sym(op), col_sym("SubtractOp"));
        ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Operand"));
        ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Result"));
        return res;
      }
      ctx_emit4(ctx, "operation", col_sym(op), col_sym("UnOp"), col_sym(pt), col_sym(ctx->cur_sc));
      ctx_emit2(ctx, "operationOf", col_sym(op), col_sym(ctx->inst_sym));
      ctx_emit2(ctx, "emitted", col_sym(op), col_sym("UnOp"));
      return ext_new_val(ctx, "Bool");
    }

    case STIGI_EXPR_IF: {
      const char *pre_sc = ctx->cur_sc;
      bool saved_top = ctx->in_top_block;
      ctx->in_top_block = false;
      uint32_t saved_vc = ctx->var_count;
      const char **pre_vals = snapshot_var_vals(ctx, saved_vc);

      const char *t_start = ext_new_pt_raw(ctx);
      const char *f_start = ext_new_pt_raw(ctx);
      const char *ts = NULL;
      const char *fs_sc = NULL;
      ext_emit_cond_branch(ctx, e->u.if_expr.cond, t_start, f_start, &ts, &fs_sc);
      if (ts == NULL) ts = ext_new_sc(ctx, pre_sc);
      if (fs_sc == NULL) fs_sc = ext_new_sc(ctx, pre_sc);

      /* Then branch */
      ctx->cur_sc = ts;
      ctx->cur_pt = t_start;
      ExtVal tv = ext_expr(ctx, e->u.if_expr.then_br, hint_ty, in_tail);
      const char *t_end = ctx->cur_pt;
      const char *t_end_sc = ctx->cur_sc;
      const char **then_vals = snapshot_var_vals(ctx, saved_vc);

      /* Else branch (restore pre-branch vars!) */
      ctx->var_count = saved_vc;
      restore_var_vals(ctx, pre_vals, saved_vc);
      ctx->cur_sc = fs_sc;
      ctx->cur_pt = f_start;
      ExtVal fv = ext_expr(ctx, e->u.if_expr.else_br, tv.ty, in_tail);
      const char *f_end = ctx->cur_pt;
      const char *f_end_sc = ctx->cur_sc;
      const char **else_vals = snapshot_var_vals(ctx, saved_vc);

      /* Join point + phi for mutated vars and expression result */
      ctx->var_count = saved_vc;
      const char *join_pt = ext_new_pt_raw(ctx);
      ctx_emit2(ctx, "next", col_sym(t_end), col_sym(join_pt));
      ctx_emit2(ctx, "next", col_sym(f_end), col_sym(join_pt));
      ctx_emit1(ctx, "join", col_sym(join_pt));
      ctx->cur_pt = join_pt;
      ctx->cur_sc = pre_sc;

      for (uint32_t k = 0; k < saved_vc && then_vals != NULL && else_vals != NULL; k++) {
        if (strcmp(then_vals[k], else_vals[k]) != 0) {
          ExtVal m = ext_new_val(ctx, ctx->vars[k].type_sym);
          ctx_emit6(ctx, "phi", col_sym(m.sym), col_sym(then_vals[k]), col_sym(t_end_sc),
                    col_sym(else_vals[k]), col_sym(f_end_sc), col_sym(pre_sc));
          ctx->vars[k].val_sym = m.sym;
        } else {
          ctx->vars[k].val_sym = then_vals[k];
        }
      }
      free(pre_vals);
      free(then_vals);
      free(else_vals);
      ctx->in_top_block = saved_top;

      ExtVal merged = ext_new_val(ctx, tv.ty);
      ctx_emit6(ctx, "phi", col_sym(merged.sym), col_sym(tv.sym), col_sym(t_end_sc),
                col_sym(fv.sym), col_sym(f_end_sc), col_sym(pre_sc));
      return merged;
    }

    case STIGI_EXPR_WHILE: {
      const char *pre_sc = ctx->cur_sc;
      bool saved_top = ctx->in_top_block;
      ctx->in_top_block = false;
      uint32_t saved_vc = ctx->var_count;

      /* Identify which outer vars are mutated inside the loop body */
      bool *mutated = saved_vc > 0 ? calloc(saved_vc, sizeof *mutated) : NULL;
      if (saved_vc > 0 && mutated == NULL) {
        ctx->err = diag_set(ctx->diag, STIGI_ERR_OOM, -1, "OOM");
        return ext_new_val(ctx, "Unit");
      }
      collect_assigned_vars(ctx, e->u.while_expr.body, mutated, saved_vc);

      /* Detect first-statement bare @<...> loop invariant & optional decreases(m) */
      const StigiFactGroup *inv_fg = NULL;
      uint32_t body_start_idx = 0;
      const StigiExpr *wbody = e->u.while_expr.body;
      if (wbody != NULL && wbody->kind == STIGI_EXPR_BLOCK && wbody->u.block.count > 0 &&
          wbody->u.block.stmts[0]->kind == STIGI_EXPR_ASSERT &&
          wbody->u.block.stmts[0]->u.assert_expr.expr == NULL) {
        inv_fg = &wbody->u.block.stmts[0]->u.assert_expr.facts;
        body_start_idx = 1;
      } else if (wbody != NULL && wbody->kind == STIGI_EXPR_ASSERT &&
                 wbody->u.assert_expr.expr == NULL) {
        inv_fg = &wbody->u.assert_expr.facts;
        body_start_idx = 1;
      }
      const StigiExpr *dec_expr = e->u.while_expr.decreases;
      if (dec_expr == NULL && inv_fg != NULL) {
        dec_expr = find_decreases_in_fg(inv_fg);
      }

      const char **pre_vals = snapshot_var_vals(ctx, saved_vc);
      const char **head_vals = saved_vc > 0 ? calloc(saved_vc, sizeof *head_vals) : NULL;
      if (saved_vc > 0 && head_vals == NULL) {
        free(mutated);
        free(pre_vals);
        ctx->err = diag_set(ctx->diag, STIGI_ERR_OOM, -1, "OOM");
        return ext_new_val(ctx, "Unit");
      }

      const char *head_pt = ext_new_pt(ctx);
      const char *head_op = ext_new_op(ctx);
      const char *head_sc = ext_new_sc(ctx, pre_sc);
      const char *site = ctx_fmt(ctx, "%s:loop%s", ctx->inst_sym, head_pt);
      ctx_emit4(ctx, "operation", col_sym(head_op), col_sym("LoopHead"), col_sym(head_pt),
                col_sym(head_sc));
      ctx_emit2(ctx, "operationOf", col_sym(head_op), col_sym(ctx->inst_sym));
      ctx_emit2(ctx, "emitted", col_sym(head_op), col_sym("BackEdge"));
      ctx_emit1(ctx, "loopHead", col_sym(head_pt));
      ctx_emit2(ctx, "loopSite", col_sym(site), col_sym(head_pt));
      ctx_emit2(ctx, "loopOf", col_sym(site), col_sym(ctx->inst_sym));

      /* 1. Check loop invariant at pre-loop entry edge (with pre-loop var SSA values) */
      if (inv_fg != NULL) {
        if (fact_group_has_safety(inv_fg)) {
          ctx_emit2(ctx, "statesFact", col_sym(ctx->unit_sym), col_sym("Invariant"));
        }
        ext_emit_owed_subst(ctx, head_op, inv_fg, pre_sc, "invariant-unproved",
                            NULL, NULL, NULL, 0);
      }

      /* 2. Allocate fresh loop-head SSA values for mutated vars */
      for (uint32_t k = 0; k < saved_vc; k++) {
        if (mutated[k]) {
          ExtVal hv = ext_new_val(ctx, ctx->vars[k].type_sym);
          head_vals[k] = hv.sym;
          ctx->vars[k].val_sym = hv.sym;
        } else {
          head_vals[k] = pre_vals[k];
        }
      }

      /* 3. Credit loop invariant in head_sc (referencing head_vals) */
      ctx->cur_sc = head_sc;
      ctx->cur_pt = head_pt;
      if (inv_fg != NULL) {
        ext_emit_credits_subst(ctx, inv_fg, head_sc, NULL, NULL, NULL, 0);
      }

      /* 4. Evaluate loop-head decreases measure in head_sc */
      const char *m_head_sym = NULL;
      if (dec_expr != NULL) {
        ExtVal mh = ext_expr(ctx, dec_expr, "U64", false);
        m_head_sym = mh.sym;
        ctx_emit2(ctx, "decreasesAt", col_sym(site), col_sym(m_head_sym));
      }

      /* 5. Extract condition with short-circuit && / || */
      const char *body_start_pt = ext_new_pt_raw(ctx);
      const char *exit_pt = ext_new_pt_raw(ctx);
      const char *body_sc = NULL;
      const char *exit_sc = NULL;
      ext_emit_cond_branch(ctx, e->u.while_expr.cond, body_start_pt, exit_pt,
                           &body_sc, &exit_sc);
      if (body_sc == NULL) body_sc = ext_new_sc(ctx, head_sc);
      if (exit_sc == NULL) exit_sc = ext_new_sc(ctx, head_sc);

      /* 6. Extract loop body (skipping stmt 0 if it was the loop invariant) */
      ctx->cur_sc = body_sc;
      ctx->cur_pt = body_start_pt;
      if (wbody != NULL && wbody->kind == STIGI_EXPR_BLOCK) {
        uint32_t bvc = ctx->var_count;
        for (uint32_t i = body_start_idx; i < wbody->u.block.count; i++) {
          (void)ext_expr(ctx, wbody->u.block.stmts[i], NULL, false);
        }
        ctx->var_count = bvc;
      } else if (body_start_idx == 0 && wbody != NULL) {
        (void)ext_expr(ctx, wbody, NULL, false);
      }

      const char *back_pt = ctx->cur_pt;
      const char *back_sc = ctx->cur_sc;

      /* 7. Check loop invariant at back edge (with post-body var SSA values) */
      if (inv_fg != NULL) {
        ext_emit_owed_subst(ctx, head_op, inv_fg, back_sc, "invariant-unproved",
                            NULL, NULL, NULL, 0);
      }

      /* 8. Evaluate next measure at back edge and emit progressStep */
      if (dec_expr != NULL && m_head_sym != NULL) {
        ExtVal mn = ext_expr(ctx, dec_expr, "U64", false);
        back_pt = ctx->cur_pt;
        back_sc = ctx->cur_sc;
        ctx_emit5(ctx, "progressStep", col_sym(site), col_sym(back_pt),
                  col_sym(m_head_sym), col_sym(mn.sym), col_sym(back_sc));
      }

      /* 9. Emit phi merging pre-loop and back-edge SSA values into head_vals */
      for (uint32_t k = 0; k < saved_vc; k++) {
        if (mutated[k]) {
          ctx_emit6(ctx, "phi", col_sym(head_vals[k]), col_sym(pre_vals[k]), col_sym(pre_sc),
                    col_sym(ctx->vars[k].val_sym), col_sym(back_sc), col_sym(head_sc));
        }
      }
      ctx_emit2(ctx, "backEdge", col_sym(back_pt), col_sym(head_pt));
      ctx_emit2(ctx, "next", col_sym(back_pt), col_sym(head_pt));

      /* 10. Exit loop: mutated vars hold their head_vals at exit_sc */
      ctx->var_count = saved_vc;
      restore_var_vals(ctx, head_vals, saved_vc);
      ctx->cur_pt = exit_pt;
      ctx->cur_sc = exit_sc;
      ctx->in_top_block = saved_top;

      free(mutated);
      free(pre_vals);
      free(head_vals);
      return ext_new_val(ctx, "Unit");
    }

    case STIGI_EXPR_MATCH: {
      ExtVal sv = ext_expr(ctx, e->u.match_expr.scrutinee, NULL, false);
      const char *pre_sc = ctx->cur_sc;
      bool saved_top = ctx->in_top_block;
      ctx->in_top_block = false;
      uint32_t saved_vc = ctx->var_count;
      const char **pre_vals = snapshot_var_vals(ctx, saved_vc);

      const char *match_pt = ext_new_pt(ctx);
      const char *match_op = ext_new_op(ctx);
      ctx_emit4(ctx, "operation", col_sym(match_op), col_sym("Match"), col_sym(match_pt),
                col_sym(pre_sc));
      ctx_emit2(ctx, "operationOf", col_sym(match_op), col_sym(ctx->inst_sym));
      ctx_emit2(ctx, "emitted", col_sym(match_op), col_sym("Match"));

      uint32_t narm = e->u.match_expr.arm_count;
      const char *join_pt = ext_new_pt_raw(ctx);
      const char **acc_var_vals = snapshot_var_vals(ctx, saved_vc);
      const char **acc_var_scs = saved_vc > 0 ? malloc((size_t)saved_vc * sizeof *acc_var_scs) : NULL;
      if (saved_vc > 0 && acc_var_scs != NULL) {
        for (uint32_t k = 0; k < saved_vc; k++) acc_var_scs[k] = pre_sc;
      }

      ExtVal acc = ext_new_val(ctx, hint_ty != NULL ? hint_ty : "U64");
      const char *prev_sc = pre_sc;

      for (uint32_t i = 0; i < narm; i++) {
        const StigiMatchArm *arm = &e->u.match_expr.arms[i];
        ctx->var_count = saved_vc;
        restore_var_vals(ctx, pre_vals, saved_vc);

        const char *arm_sc = ext_new_sc(ctx, pre_sc);
        ctx->cur_sc = arm_sc;
        ctx->cur_pt = match_pt; /* Every arm branches from match_pt! */
        const char *arm_pt = ext_new_pt(ctx);
        const char *arm_op = ext_new_op(ctx);
        const char *cname = arm->pat.name.len > 0
                                ? ctx_strdup(ctx, arm->pat.name.ptr, arm->pat.name.len)
                                : "_";

        ctx_emit4(ctx, "operation", col_sym(arm_op), col_sym("MatchArm"), col_sym(arm_pt),
                  col_sym(arm_sc));
        ctx_emit2(ctx, "operationOf", col_sym(arm_op), col_sym(ctx->inst_sym));
        ctx_emit3(ctx, "matchArm", col_sym(arm_op), col_sym(sv.sym), col_sym(cname));
        ctx_emit2(ctx, "emitted", col_sym(arm_op), col_sym("MatchArm"));

        /* Resolve enum case declaration & bind pattern variables */
        if (arm->pat.kind == STIGI_PAT_CTOR && arm->pat.field_count > 0) {
          uint32_t case_idx = 0;
          bool ambig = false;
          const StigiEnumCase *ec = NULL;
          int32_t enum_idx = stigi_image_resolve_enum_case(ctx->img, ctx->uidx, arm->pat.name.ptr,
                                                           arm->pat.name.len, &case_idx, &ambig);
          if (enum_idx >= 0 && !ambig) {
            ec = &ctx->img->units[enum_idx].ast->decl->u.enum_decl.cases[case_idx];
          }
          uint32_t pac = arm->pat.field_count;
          const char **bvals = malloc((size_t)pac * sizeof *bvals);
          if (bvals == NULL) {
            ctx->err = diag_set(ctx->diag, STIGI_ERR_OOM, -1, "OOM");
            break;
          }
          for (uint32_t f = 0; f < pac; f++) {
            StigiStr fname = arm->pat.fields[f];
            bool is_wild = (fname.len == 0 || (fname.len == 1 && fname.ptr[0] == '_'));
            const char *fty = (ec != NULL && f < ec->field_count)
                                  ? type_name_sym(ctx, ec->fields[f].type)
                                  : "U64";
            ExtVal bv = ext_new_val(ctx, fty);
            bvals[f] = bv.sym;
            if (!is_wild) {
              ExtVar *v = ext_push_var(ctx);
              if (v != NULL) {
                v->name = fname;
                v->val_sym = bv.sym;
                v->type_sym = fty;
                if (ec != NULL && f < ec->field_count) {
                  v->len_sym = ext_array_len_sym(ctx, ec->fields[f].type, ec->fields, bvals, f,
                                                 &v->is_array, &v->elem_ty);
                  uint32_t ruidx = 0;
                  const StigiDecl *rt = find_refined_type_decl(ctx->img, (uint32_t)enum_idx,
                                                               ec->fields[f].type, &ruidx, 0);
                  if (rt != NULL) {
                    v->refined_decl = rt;
                    v->refined_uidx = ruidx;
                    ext_emit_refined_credits(ctx, rt, ruidx, arm_sc, bv.sym);
                  }
                } else if (sv.try_kind == TRY_FILL && str_eq_c(arm->pat.name, "Filled") && f == 0) {
                  v->is_array = true;
                  v->type_sym = "Array";
                  v->len_sym = sv.try_cap;
                  v->elem_ty = sv.try_elem_ty;
                  if (sv.try_region != NULL) {
                    ctx_emit3(ctx, "alloc", col_sym(bv.sym), col_sym(sv.try_region),
                              col_sym(arm_pt));
                    ctx_emit2(ctx, "ownerName", col_sym(bv.sym), col_sym(bv.sym));
                  }
                } else if (sv.try_kind == TRY_GROW && str_eq_c(arm->pat.name, "Grown") && f == 0) {
                  v->is_array = true;
                  v->type_sym = "Array";
                  v->len_sym = sv.try_cap;
                  v->elem_ty = sv.try_elem_ty;
                  if (sv.try_region != NULL) {
                    ctx_emit3(ctx, "alloc", col_sym(bv.sym), col_sym(sv.try_region),
                              col_sym(arm_pt));
                    ctx_emit2(ctx, "ownerName", col_sym(bv.sym), col_sym(bv.sym));
                  }
                } else if (sv.try_kind == TRY_GROW && str_eq_c(arm->pat.name, "GrowFailed") &&
                           f == 0) {
                  v->is_array = true;
                  v->type_sym = "Array";
                  v->len_sym = sv.try_old_len;
                  v->elem_ty = sv.try_elem_ty;
                  if (sv.try_old_obj != NULL) {
                    ctx_emit2(ctx, "regain", col_sym(arm_pt), col_sym(sv.try_old_obj));
                    ctx_emit2(ctx, "alias", col_sym(bv.sym), col_sym(sv.try_old_obj));
                    v->val_sym = sv.try_old_obj;
                  }
                }
              }
            }
          }
          if (ec != NULL && ec->has_facts) {
            uint32_t scnt = pac < ec->field_count ? pac : ec->field_count;
            uint32_t saved_u = ctx->uidx;
            ctx->uidx = (uint32_t)enum_idx;
            ext_emit_credits_subst(ctx, &ec->facts, arm_sc, sv.sym, ec->fields, bvals, scnt);
            ctx->uidx = saved_u;
          }
          free(bvals);
        }

        ExtVal av = ext_expr(ctx, arm->body, hint_ty, in_tail);
        const char *arm_end_pt = ctx->cur_pt;
        const char *arm_end_sc = ctx->cur_sc;
        ctx_emit2(ctx, "next", col_sym(arm_end_pt), col_sym(join_pt));

        if (i == 0) {
          acc = av;
          prev_sc = arm_end_sc;
          for (uint32_t k = 0; k < saved_vc && acc_var_vals != NULL; k++) {
            acc_var_vals[k] = ctx->vars[k].val_sym;
            if (acc_var_scs != NULL) acc_var_scs[k] = arm_end_sc;
          }
        } else {
          ExtVal merged = ext_new_val(ctx, acc.ty);
          ctx_emit6(ctx, "phi", col_sym(merged.sym), col_sym(acc.sym), col_sym(prev_sc),
                    col_sym(av.sym), col_sym(arm_end_sc), col_sym(pre_sc));
          acc = merged;
          prev_sc = pre_sc;
          for (uint32_t k = 0; k < saved_vc && acc_var_vals != NULL; k++) {
            if (strcmp(acc_var_vals[k], ctx->vars[k].val_sym) != 0) {
              const char *asc = acc_var_scs != NULL ? acc_var_scs[k] : pre_sc;
              ExtVal mv = ext_new_val(ctx, ctx->vars[k].type_sym);
              ctx_emit6(ctx, "phi", col_sym(mv.sym), col_sym(acc_var_vals[k]), col_sym(asc),
                        col_sym(ctx->vars[k].val_sym), col_sym(arm_end_sc), col_sym(pre_sc));
              acc_var_vals[k] = mv.sym;
              if (acc_var_scs != NULL) acc_var_scs[k] = pre_sc;
            }
          }
        }
      }

      ctx->var_count = saved_vc;
      restore_var_vals(ctx, acc_var_vals, saved_vc);
      free(pre_vals);
      free(acc_var_vals);
      free(acc_var_scs);

      if (narm > 1) {
        ctx_emit1(ctx, "join", col_sym(join_pt));
      } else if (narm == 0) {
        ctx_emit2(ctx, "next", col_sym(match_pt), col_sym(join_pt));
      }
      ctx->cur_pt = join_pt;
      ctx->cur_sc = pre_sc;
      ctx->in_top_block = saved_top;
      return acc;
    }

    case STIGI_EXPR_VAL: {
      const char *ty = e->u.val_stmt.type != NULL ? type_name_sym(ctx, e->u.val_stmt.type) : NULL;
      ExtVal iv = ext_expr(ctx, e->u.val_stmt.init, ty, false);
      const char *pt = ext_new_pt(ctx);
      const char *op = ext_new_op(ctx);
      ctx_emit4(ctx, "operation", col_sym(op), col_sym("Val"), col_sym(pt), col_sym(ctx->cur_sc));
      ctx_emit2(ctx, "operationOf", col_sym(op), col_sym(ctx->inst_sym));
      ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Val"));
      if (e->u.val_stmt.pat.kind == STIGI_PAT_VAR) {
        uint32_t ruidx = 0;
        const StigiDecl *rt = e->u.val_stmt.type != NULL
                                  ? find_refined_type_decl(ctx->img, ctx->uidx, e->u.val_stmt.type,
                                                           &ruidx, 0)
                                  : NULL;
        if (rt != NULL) {
          ext_emit_refined_owed(ctx, op, rt, ruidx, ctx->cur_sc, iv.sym);
          ext_emit_refined_credits(ctx, rt, ruidx, ctx->cur_sc, iv.sym);
        }
        ExtVar *v = ext_push_var(ctx);
        if (v != NULL) {
          v->name = e->u.val_stmt.pat.name;
          v->val_sym = iv.sym;
          v->type_sym = ty != NULL ? ty : iv.ty;
          v->is_static = iv.is_static;
          v->refined_decl = rt;
          v->refined_uidx = ruidx;
          if (e->u.val_stmt.type != NULL) {
            v->len_sym = ext_array_len_sym(ctx, e->u.val_stmt.type, NULL, NULL, 0,
                                           &v->is_array, &v->elem_ty);
          }
          if (!v->is_array && iv.is_array) {
            v->is_array = true;
            v->len_sym = iv.len_sym;
            v->elem_ty = iv.elem_ty;
          }
        }
      } else if (e->u.val_stmt.pat.kind == STIGI_PAT_CTOR && e->u.val_stmt.pat.field_count > 0) {
        uint32_t case_idx = 0;
        bool ambig = false;
        const StigiEnumCase *ec = NULL;
        int32_t enum_idx = stigi_image_resolve_enum_case(ctx->img, ctx->uidx,
                                                         e->u.val_stmt.pat.name.ptr,
                                                         e->u.val_stmt.pat.name.len,
                                                         &case_idx, &ambig);
        if (enum_idx >= 0 && !ambig) {
          ec = &ctx->img->units[enum_idx].ast->decl->u.enum_decl.cases[case_idx];
        }
        uint32_t pac = e->u.val_stmt.pat.field_count;
        const char **bvals = malloc((size_t)pac * sizeof *bvals);
        if (bvals == NULL) {
          ctx->err = diag_set(ctx->diag, STIGI_ERR_OOM, -1, "OOM");
        } else {
          for (uint32_t f = 0; f < pac; f++) {
            StigiStr fname = e->u.val_stmt.pat.fields[f];
            bool is_wild = (fname.len == 0 || (fname.len == 1 && fname.ptr[0] == '_'));
            const char *fty = (ec != NULL && f < ec->field_count)
                                  ? type_name_sym(ctx, ec->fields[f].type)
                                  : "U64";
            ExtVal bv = ext_new_val(ctx, fty);
            bvals[f] = bv.sym;
            if (!is_wild) {
              ExtVar *v = ext_push_var(ctx);
              if (v != NULL) {
                v->name = fname;
                v->val_sym = bv.sym;
                v->type_sym = fty;
                if (ec != NULL && f < ec->field_count) {
                  v->len_sym = ext_array_len_sym(ctx, ec->fields[f].type, ec->fields, bvals, f,
                                                 &v->is_array, &v->elem_ty);
                  uint32_t ruidx = 0;
                  const StigiDecl *rt = find_refined_type_decl(ctx->img, (uint32_t)enum_idx,
                                                               ec->fields[f].type, &ruidx, 0);
                  if (rt != NULL) {
                    v->refined_decl = rt;
                    v->refined_uidx = ruidx;
                    ext_emit_refined_credits(ctx, rt, ruidx, ctx->cur_sc, bv.sym);
                  }
                }
              }
            }
          }
          if (ec != NULL && ec->has_facts) {
            uint32_t scnt = pac < ec->field_count ? pac : ec->field_count;
            uint32_t saved_u = ctx->uidx;
            ctx->uidx = (uint32_t)enum_idx;
            ext_emit_credits_subst(ctx, &ec->facts, ctx->cur_sc, iv.sym, ec->fields, bvals, scnt);
            ctx->uidx = saved_u;
          }
          free(bvals);
        }
      }
      return iv;
    }

    case STIGI_EXPR_VAR: {
      const char *ty = e->u.var_stmt.type != NULL ? type_name_sym(ctx, e->u.var_stmt.type) : NULL;
      ExtVal iv = ext_expr(ctx, e->u.var_stmt.init, ty, false);
      const char *pt = ext_new_pt(ctx);
      const char *op = ext_new_op(ctx);
      ctx_emit4(ctx, "operation", col_sym(op), col_sym("Var"), col_sym(pt), col_sym(ctx->cur_sc));
      ctx_emit2(ctx, "operationOf", col_sym(op), col_sym(ctx->inst_sym));
      ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Var"));
      uint32_t ruidx = 0;
      const StigiDecl *rt = e->u.var_stmt.type != NULL
                                ? find_refined_type_decl(ctx->img, ctx->uidx, e->u.var_stmt.type,
                                                         &ruidx, 0)
                                : NULL;
      if (rt != NULL) {
        ext_emit_refined_owed(ctx, op, rt, ruidx, ctx->cur_sc, iv.sym);
        ext_emit_refined_credits(ctx, rt, ruidx, ctx->cur_sc, iv.sym);
      }
      ExtVar *v = ext_push_var(ctx);
      if (v != NULL) {
        v->name = e->u.var_stmt.name;
        v->val_sym = iv.sym;
        v->type_sym = ty != NULL ? ty : iv.ty;
        v->is_static = iv.is_static;
        v->has_inv = e->u.var_stmt.has_inv;
        v->inv = e->u.var_stmt.inv;
        v->refined_decl = rt;
        v->refined_uidx = ruidx;
        if (e->u.var_stmt.type != NULL) {
          v->len_sym = ext_array_len_sym(ctx, e->u.var_stmt.type, NULL, NULL, 0,
                                         &v->is_array, &v->elem_ty);
        }
        if (!v->is_array && iv.is_array) {
          v->is_array = true;
          v->len_sym = iv.len_sym;
          v->elem_ty = iv.elem_ty;
        }
      }
      if (e->u.var_stmt.has_inv) {
        if (fact_group_has_safety(&e->u.var_stmt.inv)) {
          ctx_emit2(ctx, "statesFact", col_sym(ctx->unit_sym), col_sym("Invariant"));
        }
        ext_emit_owed_subst(ctx, op, &e->u.var_stmt.inv, ctx->cur_sc, "assertion-unproved",
                            iv.sym, NULL, NULL, 0);
        const char *post_sc = ext_new_sc(ctx, ctx->cur_sc);
        ext_emit_credits_subst(ctx, &e->u.var_stmt.inv, post_sc, iv.sym, NULL, NULL, 0);
        ctx->cur_sc = post_sc;
      }
      return iv;
    }

    case STIGI_EXPR_ASSIGN: {
      ExtVar *ev = ext_find_var(ctx, e->u.assign_stmt.name);
      const char *hint = ev != NULL ? ev->type_sym : NULL;
      ExtVal vv = ext_expr(ctx, e->u.assign_stmt.value, hint, false);
      /* Re-lookup in case vars reallocated while extracting rhs */
      ev = ext_find_var(ctx, e->u.assign_stmt.name);
      const char *pt = ext_new_pt(ctx);
      const char *op = ext_new_op(ctx);
      ctx_emit4(ctx, "operation", col_sym(op), col_sym("Assign"), col_sym(pt),
                col_sym(ctx->cur_sc));
      ctx_emit2(ctx, "operationOf", col_sym(op), col_sym(ctx->inst_sym));
      ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Assign"));
      if (ev != NULL) {
        /* Bind directly to the new SSA value (exact equality + preserves addOp/subOp provenance!) */
        ev->val_sym = vv.sym;
        if (vv.is_array) {
          ev->is_array = true;
          ev->len_sym = vv.len_sym;
          ev->elem_ty = vv.elem_ty;
          ev->is_static = vv.is_static;
        }
        if (ev->refined_decl != NULL) {
          ext_emit_refined_owed(ctx, op, ev->refined_decl, ev->refined_uidx, ctx->cur_sc, vv.sym);
          ext_emit_refined_credits(ctx, ev->refined_decl, ev->refined_uidx, ctx->cur_sc, vv.sym);
        }
        if (ev->has_inv) {
          ext_emit_owed_subst(ctx, op, &ev->inv, ctx->cur_sc, "assertion-unproved",
                              vv.sym, NULL, NULL, 0);
          const char *post_sc = ext_new_sc(ctx, ctx->cur_sc);
          ext_emit_credits_subst(ctx, &ev->inv, post_sc, vv.sym, NULL, NULL, 0);
          ctx->cur_sc = post_sc;
        }
      }
      return vv;
    }

    case STIGI_EXPR_INDEX_READ: {
      ExtVal tv = ext_expr(ctx, e->u.index_expr.target, NULL, false);
      ExtVal iv = ext_expr(ctx, e->u.index_expr.index, "U64", false);
      bool b_read = (e->u.index_expr.target->kind == STIGI_EXPR_LOCAL);
      return emit_index_read_op(ctx, tv.sym, tv.len_sym,
                                tv.elem_ty != NULL ? tv.elem_ty : hint_ty, iv, b_read);
    }

    case STIGI_EXPR_INDEX_WRITE: {
      ExtVal iv = ext_expr(ctx, e->u.index_expr.index, "U64", false);
      const char *obj_sym = NULL;
      const char *len_sym = NULL;
      const char *val_hint = NULL;
      ExtVar *ev = NULL;
      if (e->u.index_expr.target->kind == STIGI_EXPR_LOCAL) {
        ev = ext_find_var(ctx, e->u.index_expr.target->u.ident.name);
        if (ev != NULL) {
          obj_sym = ev->val_sym;
          len_sym = ev->len_sym;
          val_hint = ev->elem_ty;
          if (ev->is_param) ev->param_mutated = true;
        }
      } else {
        ExtVal tv = ext_expr(ctx, e->u.index_expr.target, NULL, false);
        obj_sym = tv.sym;
        len_sym = tv.len_sym;
        val_hint = tv.elem_ty;
      }
      ExtVal vv = ext_expr(ctx, e->u.index_expr.value, val_hint, false);
      if (obj_sym == NULL) obj_sym = ext_new_val(ctx, "Array").sym;
      if (len_sym == NULL) len_sym = ext_new_val(ctx, "U64").sym;

      const char *pt = ext_new_pt(ctx);
      const char *op = ext_new_op(ctx);
      ctx_emit4(ctx, "operation", col_sym(op), col_sym("IndexWrite"), col_sym(pt),
                col_sym(ctx->cur_sc));
      ctx_emit2(ctx, "operationOf", col_sym(op), col_sym(ctx->inst_sym));
      ctx_emit4(ctx, "indexWrite", col_sym(op), col_sym(iv.sym), col_sym(len_sym),
                col_sym(ctx->cur_sc));
      ctx_emit3(ctx, "elementWrite", col_sym(op), col_sym(obj_sym), col_sym(pt));
      ctx_emit2(ctx, "overwrite", col_sym(pt), col_sym(obj_sym));
      ctx_emit2(ctx, "writeVia", col_sym(op), col_sym(obj_sym));
      ctx_emit3(ctx, "operand", col_sym(op), col_i64(0), col_sym(iv.sym));
      ctx_emit3(ctx, "operand", col_sym(op), col_i64(1), col_sym(vv.sym));
      ctx_emit2(ctx, "emitted", col_sym(op), col_sym("IndexWrite"));
      ctx_emit2(ctx, "emitted", col_sym(op), col_sym("ElementWrite"));
      ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Overwrite"));
      ctx_emit2(ctx, "emitted", col_sym(op), col_sym("WriteVia"));
      ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Operand"));
      ctx_emit2(ctx, "use", col_sym(pt), col_sym(obj_sym));
      ctx_emit3(ctx, "access", col_sym(obj_sym), col_sym(iv.sym), col_sym(ctx->cur_sc));
      ctx_emit2(ctx, "needsCapability", col_sym(op), col_sym("Stigi.Mut"));
      ExtCap *mut_cap = ext_find_cap_by_unit(ctx, "Stigi.Mut");
      if (mut_cap != NULL) {
        ctx_emit2(ctx, "provider", col_sym(op), col_sym(mut_cap->prov_sym));
      }
      return ext_new_val(ctx, "Unit");
    }

    case STIGI_EXPR_HANDLE: {
      for (uint32_t i = 0; i < e->u.handle_stmt.arg_count; i++) {
        (void)ext_expr(ctx, e->u.handle_stmt.args[i], NULL, false);
      }
      const char *pt = ext_new_pt(ctx);
      const char *op = ext_new_op(ctx);
      const char *h_id = ext_new_atom(ctx);
      ctx_emit4(ctx, "operation", col_sym(op), col_sym("Handle"), col_sym(pt),
                col_sym(ctx->cur_sc));
      ctx_emit2(ctx, "operationOf", col_sym(op), col_sym(ctx->inst_sym));
      ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Handle"));
      ctx_emit2(ctx, "handleSite", col_sym(h_id), col_sym(ctx->in_top_block ? "Top" : "Nested"));

      /* Resolve handler -> capability unit */
      const char *cap_unit = NULL;
      bool is_arena = false;
      bool is_heap = false;
      StigiStr hn = e->u.handle_stmt.handler_name;
      if (str_eq_c(hn, "Stigi.Region.arena") || str_eq_c(hn, "Stigi.Arena")) {
        cap_unit = "Stigi.Region";
        is_arena = true;
      } else if (str_eq_c(hn, "Stigi.Region.heap") || str_eq_c(hn, "Stigi.Heap")) {
        cap_unit = "Stigi.Region";
        is_heap = true;
      } else {
        int32_t hidx = stigi_image_resolve_name(ctx->img, ctx->uidx, hn.ptr, hn.len);
        if (hidx >= 0 && ctx->img->units[hidx].ast->decl->kind == STIGI_DECL_HANDLER) {
          const StigiDecl *hd = ctx->img->units[hidx].ast->decl;
          if (hd->u.handler_decl.for_cap_type != NULL) {
            StigiStr cn = hd->u.handler_decl.for_cap_type->name;
            int32_t cidx = stigi_image_resolve_name(ctx->img, (uint32_t)hidx, cn.ptr, cn.len);
            if (cidx >= 0) {
              cap_unit = ctx_strdup(ctx, ctx->img->units[cidx].name,
                                    ctx->img->units[cidx].name_len);
            } else {
              cap_unit = ctx_strdup(ctx, cn.ptr, cn.len);
            }
          }
        }
      }

      const char *prov = ctx_fmt(ctx, "%s:hcap%u", ctx->inst_sym, ctx->cap_count);
      if (cap_unit != NULL) {
        ctx_emit2(ctx, "handleTarget", col_sym(h_id), col_sym(cap_unit));
        ctx_emit2(ctx, "capabilityOf", col_sym(prov), col_sym(cap_unit));
        if (strcmp(cap_unit, "Stigi.Region") == 0) {
          /* Builtin Stigi.Region has operations */
          ctx_emit2(ctx, "capabilityOperation", col_sym("Stigi.Region"),
                    col_sym("Stigi.Region.alloc"));
        }
      }
      if (is_arena) ctx_emit1(ctx, "arena", col_sym(prov));
      if (is_heap) ctx_emit1(ctx, "heap", col_sym(prov));

      /* Record nesting against any outer open handle */
      for (uint32_t i = ctx->cap_count; i > 0; i--) {
        if (ctx->caps[i - 1].from_handle && !ctx->caps[i - 1].closed) {
          ctx_emit2(ctx, "handleNest", col_sym(prov), col_sym(ctx->caps[i - 1].prov_sym));
          break;
        }
      }

      uint32_t saved_cap_count = ctx->cap_count;
      ExtCap *nc = ext_push_cap(ctx);
      if (nc != NULL) {
        nc->binder = e->u.handle_stmt.binder;
        nc->prov_sym = prov;
        nc->unit_sym = cap_unit != NULL ? cap_unit : "";
        nc->first_pt = pt;
        nc->from_handle = true;
        nc->closed = false;
      }

      if (e->u.handle_stmt.body != NULL) {
        ExtVal bv = ext_expr(ctx, e->u.handle_stmt.body, hint_ty, in_tail);
        if (cap_unit != NULL && !is_arena && !is_heap) {
          int32_t cidx = stigi_image_find(ctx->img, cap_unit, strlen(cap_unit));
          if (cidx >= 0 && ctx->img->units[cidx].ast->decl->kind == STIGI_DECL_CAPABILITY &&
              ctx->img->units[cidx].ast->decl->u.cap_decl.state_count > 0) {
            ctx_emit3(ctx, "handleRange", col_sym(prov), col_sym(pt), col_sym(ctx->cur_pt));
          }
        } else if (is_arena || is_heap) {
          ctx_emit3(ctx, "handleRange", col_sym(prov), col_sym(pt), col_sym(ctx->cur_pt));
        }
        ctx->cap_count = saved_cap_count;
        return bv;
      }
      return ext_new_val(ctx, "Unit");
    }

    case STIGI_EXPR_ASSERT: {
      ExtVal rv = e->u.assert_expr.expr != NULL
                      ? ext_expr(ctx, e->u.assert_expr.expr, hint_ty, in_tail)
                      : ext_new_val(ctx, "Unit");
      const char *pt = ext_new_pt(ctx);
      const char *op = ext_new_op(ctx);
      ctx_emit4(ctx, "operation", col_sym(op), col_sym("Assert"), col_sym(pt),
                col_sym(ctx->cur_sc));
      ctx_emit2(ctx, "operationOf", col_sym(op), col_sym(ctx->inst_sym));
      ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Assert"));
      if (fact_group_has_safety(&e->u.assert_expr.facts)) {
        ctx_emit2(ctx, "statesFact", col_sym(ctx->unit_sym), col_sym("Assertion"));
      }
      ext_emit_owed_subst(ctx, op, &e->u.assert_expr.facts, ctx->cur_sc, "assertion-unproved",
                          rv.sym, NULL, NULL, 0);
      const char *post_sc = ext_new_sc(ctx, ctx->cur_sc);
      ext_emit_credits_subst(ctx, &e->u.assert_expr.facts, post_sc, rv.sym, NULL, NULL, 0);
      ctx->cur_sc = post_sc;
      return rv;
    }

    case STIGI_EXPR_DENY: {
      const char *pt = ext_new_pt(ctx);
      const char *op = ext_new_op(ctx);
      ctx_emit4(ctx, "operation", col_sym(op), col_sym("Deny"), col_sym(pt), col_sym(ctx->cur_sc));
      ctx_emit2(ctx, "operationOf", col_sym(op), col_sym(ctx->inst_sym));
      ctx_emit2(ctx, "emitted", col_sym(op), col_sym("Deny"));
      ctx_emit2(ctx, "statesFact", col_sym(ctx->unit_sym), col_sym("Assertion"));
      ext_emit_denial(ctx, &e->u.deny_expr.item);
      return ext_new_val(ctx, "Unit");
    }

    case STIGI_EXPR_BLOCK: {
      uint32_t saved_vc = ctx->var_count;
      uint32_t saved_cc = ctx->cap_count;
      uint32_t saved_dc = ctx->denial_count;
      ExtVal last = ext_new_val(ctx, "Unit");
      for (uint32_t i = 0; i < e->u.block.count; i++) {
        bool is_last = (i + 1U == e->u.block.count);
        const char *h = is_last ? hint_ty : NULL;
        last = ext_expr(ctx, e->u.block.stmts[i], h, is_last ? in_tail : false);
        if (!is_last && !is_stmt_or_effect(ctx, e->u.block.stmts[i])) {
          const char *esym = ctx_fmt(ctx, "%s:e%u", ctx->inst_sym, ctx->next_atom++);
          ctx_emit1(ctx, "expression", col_sym(esym));
        }
      }
      /* Close any statement-form handles opened in this block */
      for (uint32_t c = ctx->cap_count; c > saved_cc; c--) {
        ExtCap *hc = &ctx->caps[c - 1];
        if (!hc->from_handle || hc->closed) continue;
        hc->closed = true;
        if (strcmp(hc->unit_sym, "Stigi.Region") == 0) {
          ctx_emit3(ctx, "handleRange", col_sym(hc->prov_sym), col_sym(hc->first_pt),
                    col_sym(ctx->cur_pt));
        } else if (hc->unit_sym[0] != '\0') {
          int32_t cidx = stigi_image_find(ctx->img, hc->unit_sym, strlen(hc->unit_sym));
          if (cidx >= 0 && ctx->img->units[cidx].ast->decl->kind == STIGI_DECL_CAPABILITY &&
              ctx->img->units[cidx].ast->decl->u.cap_decl.state_count > 0) {
            ctx_emit3(ctx, "handleRange", col_sym(hc->prov_sym), col_sym(hc->first_pt),
                      col_sym(ctx->cur_pt));
          }
        }
      }
      ctx->denial_count = saved_dc;
      ctx->cap_count = saved_cc;
      ctx->var_count = saved_vc;
      return last;
    }

    case STIGI_EXPR_LOCAL_DECL:
      return ext_new_val(ctx, "Unit");
  }
  return ext_new_val(ctx, "Unit");
}

/* ---- Helper: extract body of one def/lemma/handler-op/bodyful-extern ---- */

static bool try_extract_depth_bound(const StigiImage *img, uint32_t uidx,
                                    const StigiDefHead *h, StigiStr dec_param,
                                    int64_t *out_limit) {
  for (uint32_t g = 0; g < h->pre_group_count + h->post_val_fact_count; g++) {
    const StigiFactGroup *fg = g < h->pre_group_count
                                   ? (h->pre_groups[g].is_fact ? &h->pre_groups[g].facts : NULL)
                                   : &h->post_val_facts[g - h->pre_group_count];
    if (fg == NULL) continue;
    for (uint32_t i = 0; i < fg->count; i++) {
      const StigiFactItem *it = &fg->items[i];
      const StigiExpr *lhs = NULL, *rhs = NULL;
      bool is_strict = false;
      if (it->kind == STIGI_FACT_ATOM && it->arg_count == 2) {
        if (str_eq_c(it->atom_name, "below") || str_eq_c(it->atom_name, "Stigi.Facts.below")) {
          lhs = it->args[0]; rhs = it->args[1]; is_strict = true;
        } else if (str_eq_c(it->atom_name, "lessOrEqual") ||
                   str_eq_c(it->atom_name, "Stigi.Facts.lessOrEqual")) {
          lhs = it->args[0]; rhs = it->args[1]; is_strict = false;
        }
      } else if (it->kind == STIGI_FACT_CMP && (it->op == STIGI_OP_LT || it->op == STIGI_OP_LE)) {
        lhs = it->lhs; rhs = it->rhs; is_strict = (it->op == STIGI_OP_LT);
      }
      if (lhs == NULL || rhs == NULL) continue;
      if ((lhs->kind == STIGI_EXPR_LOCAL || lhs->kind == STIGI_EXPR_GLOBAL) &&
          lhs->u.ident.name.len == dec_param.len &&
          memcmp(lhs->u.ident.name.ptr, dec_param.ptr, dec_param.len) == 0) {
        uint64_t kval = 0;
        bool has_k = false;
        if (rhs->kind == STIGI_EXPR_LIT_INT && !rhs->u.lit_int.overflow) {
          kval = rhs->u.lit_int.value;
          has_k = true;
        } else if (rhs->kind == STIGI_EXPR_LOCAL || rhs->kind == STIGI_EXPR_GLOBAL) {
          int32_t vidx = stigi_image_resolve_name(img, uidx, rhs->u.ident.name.ptr,
                                                  rhs->u.ident.name.len);
          TupuI128 n;
          if (vidx >= 0 && eval_top_val_int(img, (uint32_t)vidx, &n, NULL) && n.hi == 0) {
            kval = n.lo;
            has_k = true;
          }
        }
        if (has_k && kval > 0 && kval <= (uint64_t)INT64_MAX - 1ULL) {
          *out_limit = is_strict ? (int64_t)kval : (int64_t)(kval + 1ULL);
          return true;
        }
      }
    }
  }
  return false;
}

static Status extract_def_body(const StigiImage *img, uint32_t unit_idx, const char *u_sym,
                               const char *inst_sym, const StigiDefHead *h, const StigiExpr *body,
                               bool is_lemma, bool is_root_def, uint64_t frame_bytes,
                               StigiFactSet *fs, Diag *diag) {
  Status st = emit2(fs, "instanceOf", col_sym(inst_sym), col_sym(u_sym), diag);
  if (st != STIGI_OK) return st;
  st = emit1(fs, "defHasBody", col_sym(u_sym), diag);
  if (st != STIGI_OK) return st;
  st = emit2(fs, "frameBytes", col_sym(inst_sym), col_u64(frame_bytes), diag);
  if (st != STIGI_OK) return st;
  if (is_root_def) {
    st = emit1(fs, "rootInstance", col_sym(inst_sym), diag);
    if (st != STIGI_OK) return st;
  }
  if (is_lemma) {
    st = emit1(fs, "lemmaUnit", col_sym(u_sym), diag);
    if (st != STIGI_OK) return st;
    if (body != NULL) {
      st = emit1(fs, "lemmaHasBody", col_sym(u_sym), diag);
      if (st != STIGI_OK) return st;
    }
  }

  ExtCtx ctx;
  memset(&ctx, 0, sizeof ctx);
  ctx.img = img;
  ctx.uidx = unit_idx;
  ctx.unit_sym = u_sym;
  ctx.inst_sym = inst_sym;
  ctx.fs = fs;
  ctx.diag = diag;
  ctx.err = STIGI_OK;
  ctx.def_dec_param_idx = -1;
  ctx.in_top_block = true;
  ctx.rigid_tparams = h->type_params;
  ctx.rigid_tparam_count = h->type_param_count;
  {
    const StigiDecl *ud = img->units[unit_idx].ast->decl;
    if (ud->kind == STIGI_DECL_GIVEN && ud->u.given_decl.type_param_count > 0 &&
        h->type_param_count == 0) {
      ctx.rigid_tparams = ud->u.given_decl.type_params;
      ctx.rigid_tparam_count = ud->u.given_decl.type_param_count;
    }
    uint32_t acap = 0;
    for (uint32_t i = 0; i < h->pre_group_count; i++) {
      if (!h->pre_groups[i].is_fact) acap += h->pre_groups[i].cap_count;
    }
    if (ud->kind == STIGI_DECL_GIVEN) acap += ud->u.given_decl.cap_param_count;
    if (acap > 0) {
      ctx.local_assumps = malloc((size_t)acap * sizeof *ctx.local_assumps);
      if (ctx.local_assumps != NULL) {
        for (uint32_t i = 0; i < h->pre_group_count; i++) {
          if (!h->pre_groups[i].is_fact) {
            for (uint32_t c = 0; c < h->pre_groups[i].cap_count; c++) {
              ctx.local_assumps[ctx.local_assump_count++] = h->pre_groups[i].caps[c];
            }
          }
        }
        if (ud->kind == STIGI_DECL_GIVEN) {
          for (uint32_t c = 0; c < ud->u.given_decl.cap_param_count; c++) {
            ctx.local_assumps[ctx.local_assump_count++] = ud->u.given_decl.cap_params[c];
          }
        }
      }
    }
  }

  const char *s0 = ext_new_sc(&ctx, NULL);
  ctx.cur_sc = s0;
  const char *p0 = ext_new_pt(&ctx);
  ctx.entry_pt = p0;
  ctx_emit1(&ctx, "entry", col_sym(p0));

  for (uint32_t i = 0; i < h->param_count; i++) {
    const StigiParam *pr = &h->params[i];
    const char *ty_s = type_name_sym(&ctx, pr->type);
    const char *vs = ctx_fmt(&ctx, "%s:p:%.*s", inst_sym, (int)pr->name.len, pr->name.ptr);
    ctx_emit1(&ctx, "value", col_sym(vs));
    ctx_emit2(&ctx, "valueType", col_sym(vs), col_sym(ty_s));
    if (strcmp(ty_s, "U64") == 0) ctx_emit1(&ctx, "u64", col_sym(vs));
    ctx_emit3(&ctx, "param", col_sym(inst_sym), col_i64((int64_t)i), col_sym(vs));
    ExtVar *v = ext_push_var(&ctx);
    if (v != NULL) {
      v->name = pr->name;
      v->val_sym = vs;
      v->type_sym = ty_s;
      v->is_param = true;
      uint32_t ruidx = 0;
      const StigiDecl *rt = find_refined_type_decl(ctx.img, ctx.uidx, pr->type, &ruidx, 0);
      if (rt != NULL) {
        v->refined_decl = rt;
        v->refined_uidx = ruidx;
        ext_emit_refined_credits(&ctx, rt, ruidx, s0, vs);
      }
    }
  }
  for (uint32_t i = 0; i < h->param_count && i < ctx.var_count; i++) {
    bool is_arr = false;
    const char *ety = "U64";
    const char *ls = ext_array_len_sym(&ctx, h->params[i].type, NULL, NULL, 0, &is_arr, &ety);
    if (is_arr) {
      ctx.vars[i].is_array = true;
      ctx.vars[i].len_sym = ls;
      ctx.vars[i].elem_ty = ety;
      ctx_emit2(&ctx, "ownerName", col_sym(ctx.vars[i].val_sym), col_sym(ctx.vars[i].val_sym));
    }
  }

  const StigiExpr *dec_expr = h->decreases;
  for (uint32_t i = 0; i < h->pre_group_count; i++) {
    if (h->pre_groups[i].is_fact) {
      if (fact_group_has_safety(&h->pre_groups[i].facts)) {
        ctx_emit2(&ctx, "statesFact", col_sym(u_sym), col_sym("Entry"));
      }
      ext_emit_credits_subst(&ctx, &h->pre_groups[i].facts, s0, NULL, NULL, NULL, 0);
      if (dec_expr == NULL) dec_expr = find_decreases_in_fg(&h->pre_groups[i].facts);
    } else {
      for (uint32_t c = 0; c < h->pre_groups[i].cap_count; c++) {
        StigiStr cn = h->pre_groups[i].caps[c].cap_type->name;
        int32_t cidx = stigi_image_resolve_name(ctx.img, ctx.uidx, cn.ptr, cn.len);
        const char *cs = (cidx >= 0) ? ctx_strdup(&ctx, ctx.img->units[cidx].name,
                                                  ctx.img->units[cidx].name_len)
                                     : ctx_strdup(&ctx, cn.ptr, cn.len);
        /* S125: signatures receive protocols (capability/trait), never concrete handler/given */
        const char *sp_atom = ext_new_atom(&ctx);
        bool is_concrete = stigi_kanren_is_concrete_protocol(ctx.img, ctx.uidx,
                                                             h->pre_groups[i].caps[c].cap_type);
        ctx_emit2(&ctx, "sigProtocol", col_sym(sp_atom),
                  col_sym(is_concrete ? "Concrete" : "Protocol"));
        if (cidx >= 0 && ctx.img->units[cidx].ast->decl->kind == STIGI_DECL_TRAIT) {
          continue;
        }
        ctx_emit2(&ctx, "rowOf", col_sym(u_sym), col_sym(cs));
        const char *prov = ctx_fmt(&ctx, "%s:cap%u", inst_sym, ctx.cap_count);
        ExtCap *ec = ext_push_cap(&ctx);
        if (ec != NULL) {
          ec->binder = h->pre_groups[i].caps[c].name;
          ec->prov_sym = prov;
          ec->unit_sym = cs;
          ec->first_pt = p0;
          ec->from_handle = false;
        }
        ctx_emit2(&ctx, "capabilityOf", col_sym(prov), col_sym(cs));
      }
    }
  }
  for (uint32_t i = 0; i < h->post_val_fact_count; i++) {
    if (fact_group_has_safety(&h->post_val_facts[i])) {
      ctx_emit2(&ctx, "statesFact", col_sym(u_sym), col_sym("Entry"));
    }
    ext_emit_credits_subst(&ctx, &h->post_val_facts[i], s0, NULL, NULL, NULL, 0);
    if (dec_expr == NULL) dec_expr = find_decreases_in_fg(&h->post_val_facts[i]);
  }

  if (dec_expr != NULL) {
    const char *site = ctx_fmt(&ctx, "%s:def", inst_sym);
    const char *mv = ext_resolve_fact_val(&ctx, dec_expr, NULL, NULL, NULL, 0);
    ctx_emit2(&ctx, "decreasesAt", col_sym(site), col_sym(mv));
    ctx_emit2(&ctx, "defSite", col_sym(site), col_sym(inst_sym));
    ctx.def_dec_site = site;
    ctx.def_dec_val = mv;
    if (dec_expr->kind == STIGI_EXPR_LOCAL || dec_expr->kind == STIGI_EXPR_GLOBAL) {
      for (uint32_t i = 0; i < h->param_count; i++) {
        if (h->params[i].name.len == dec_expr->u.ident.name.len &&
            memcmp(h->params[i].name.ptr, dec_expr->u.ident.name.ptr,
                   dec_expr->u.ident.name.len) == 0) {
          ctx.def_dec_param_idx = (int32_t)i;
          int64_t lim = 0;
          if (try_extract_depth_bound(img, unit_idx, h, h->params[i].name, &lim)) {
            ctx_emit2(&ctx, "depthBound", col_sym(inst_sym), col_i64(lim));
          }
          break;
        }
      }
    }
  }

  const char *ret_hint = h->ret_type != NULL ? type_name_sym(&ctx, h->ret_type) : NULL;
  ExtVal ret_v = ext_expr(&ctx, body, ret_hint, true);

  if (h->ret_type != NULL) {
    uint32_t ruidx = 0;
    const StigiDecl *rt = find_refined_type_decl(ctx.img, ctx.uidx, h->ret_type, &ruidx, 0);
    if (rt != NULL) {
      const char *ret_op = ext_new_op(&ctx);
      ext_emit_refined_owed(&ctx, ret_op, rt, ruidx, ctx.cur_sc, ret_v.sym);
    }
  }

  for (uint32_t i = 0; i < h->exit_fact_count; i++) {
    if (fact_group_has_safety(&h->exit_facts[i])) {
      ctx_emit2(&ctx, "statesFact", col_sym(u_sym), col_sym("Exit"));
    }
    const char *ex_op = ext_new_op(&ctx);
    ext_emit_owed_subst(&ctx, ex_op, &h->exit_facts[i], ctx.cur_sc, "exit-unproved",
                        ret_v.sym, NULL, NULL, 0);
  }

  if (is_pure_total_leaf_def(img, unit_idx, h, body)) {
    ctx_emit1(&ctx, "pureTotalPredicate", col_sym(u_sym));
  }

  const char *exit_pt = ext_new_pt(&ctx);
  /* Mutated non-consumed parameters are visible to the caller on return (clears write-unread) */
  for (uint32_t i = 0; i < ctx.var_count; i++) {
    if (ctx.vars[i].is_param && ctx.vars[i].param_mutated && !ctx.vars[i].param_consumed) {
      ctx_emit2(&ctx, "use", col_sym(exit_pt), col_sym(ctx.vars[i].val_sym));
    }
  }
  ctx_emit2(&ctx, "returnOf", col_sym(exit_pt), col_sym(ret_v.sym));
  ctx_emit1(&ctx, "exit", col_sym(exit_pt));

  Status ret = ctx.err;
  ext_ctx_free(&ctx);
  return ret;
}

/* ---- Unit-level extraction ---- */

static const char *decl_kind_sym(StigiDeclKind k) {
  switch (k) {
    case STIGI_DECL_ENUM: return "Enum";
    case STIGI_DECL_TYPE: return "Type";
    case STIGI_DECL_DEF: return "Def";
    case STIGI_DECL_LEMMA_DEF: return "Def";
    case STIGI_DECL_VAL: return "Val";
    case STIGI_DECL_CAPABILITY: return "Capability";
    case STIGI_DECL_HANDLER: return "Handler";
    case STIGI_DECL_TRAIT: return "Trait";
    case STIGI_DECL_GIVEN: return "Given";
    case STIGI_DECL_EXTERN: return "Extern";
    case STIGI_DECL_RELATION: return "Relation";
    case STIGI_DECL_RULE: return "Rule";
    case STIGI_DECL_TEST: return "Test";
    case STIGI_DECL_NEG_TEST: return "NegTest";
  }
  return "Def";
}

/* Lightweight interprocedural summary extractor for callees (i != unit_idx in stigi_extract_unit_closed).
 * Emits call graph, stack frame/depth, lemma totality, and handle-param mutation summaries with ZERO
 * scope/value/lit/operation rows — eliminating cross-unit O(N_units^3) blowup in bounds.dl (performance.md 5). */
static void summary_walk_calls(const StigiImage *img, uint32_t unit_idx, const char *inst_sym,
                               const StigiDefHead *h, const StigiExpr *e, bool in_tail,
                               uint32_t *stub_seq, bool *has_undec_loop, bool *has_fact,
                               bool *param_overwrites, bool *param_returns, bool *param_consumes,
                               StigiFactSet *fs, Diag *diag, Status *err) {
  if (*err != STIGI_OK || e == NULL) return;
  switch (e->kind) {
    case STIGI_EXPR_LOCAL:
    case STIGI_EXPR_GLOBAL:
      if (in_tail && param_returns != NULL) {
        for (uint32_t k = 0; k < h->param_count; k++) {
          if (h->params[k].name.len == e->u.ident.name.len &&
              memcmp(h->params[k].name.ptr, e->u.ident.name.ptr, h->params[k].name.len) == 0) {
            param_returns[k] = true;
            break;
          }
        }
      }
      break;
    case STIGI_EXPR_INDEX_WRITE:
      if (param_overwrites != NULL && e->u.index_expr.target != NULL &&
          e->u.index_expr.target->kind == STIGI_EXPR_LOCAL) {
        for (uint32_t k = 0; k < h->param_count; k++) {
          if (h->params[k].name.len == e->u.index_expr.target->u.ident.name.len &&
              memcmp(h->params[k].name.ptr, e->u.index_expr.target->u.ident.name.ptr,
                     h->params[k].name.len) == 0) {
            param_overwrites[k] = true;
            break;
          }
        }
      }
      summary_walk_calls(img, unit_idx, inst_sym, h, e->u.index_expr.target, false, stub_seq,
                         has_undec_loop, has_fact, param_overwrites, param_returns, param_consumes, fs, diag, err);
      summary_walk_calls(img, unit_idx, inst_sym, h, e->u.index_expr.index, false, stub_seq,
                         has_undec_loop, has_fact, param_overwrites, param_returns, param_consumes, fs, diag, err);
      summary_walk_calls(img, unit_idx, inst_sym, h, e->u.index_expr.value, false, stub_seq,
                         has_undec_loop, has_fact, param_overwrites, param_returns, param_consumes, fs, diag, err);
      break;
    case STIGI_EXPR_INDEX_READ:
      summary_walk_calls(img, unit_idx, inst_sym, h, e->u.index_expr.target, false, stub_seq,
                         has_undec_loop, has_fact, param_overwrites, param_returns, param_consumes, fs, diag, err);
      summary_walk_calls(img, unit_idx, inst_sym, h, e->u.index_expr.index, false, stub_seq,
                         has_undec_loop, has_fact, param_overwrites, param_returns, param_consumes, fs, diag, err);
      break;
    case STIGI_EXPR_CALL:
      if (e->u.call.callee != NULL &&
          (e->u.call.callee->kind == STIGI_EXPR_LOCAL || e->u.call.callee->kind == STIGI_EXPR_GLOBAL)) {
        StigiStr cn = e->u.call.callee->u.ident.name;
        if (str_eq_c(cn, "Array.tryGrow") && e->u.call.arg_count == 3 &&
            param_consumes != NULL && e->u.call.args[0]->kind == STIGI_EXPR_LOCAL) {
          for (uint32_t k = 0; k < h->param_count; k++) {
            if (h->params[k].name.len == e->u.call.args[0]->u.ident.name.len &&
                memcmp(h->params[k].name.ptr, e->u.call.args[0]->u.ident.name.ptr,
                       h->params[k].name.len) == 0) {
              param_consumes[k] = true;
              break;
            }
          }
        }
        int32_t tgt = stigi_image_resolve_name(img, unit_idx, cn.ptr, cn.len);
        if (tgt >= 0) {
          const StigiDecl *td = img->units[tgt].ast->decl;
          if (td->kind == STIGI_DECL_DEF || td->kind == STIGI_DECL_LEMMA_DEF ||
              (td->kind == STIGI_DECL_EXTERN && !td->u.extern_decl.is_val)) {
            const char *sop = fs_fmt(fs, "%s:sumcall%u", inst_sym, (*stub_seq)++);
            const char *cinst = fs_fmt(fs, "%s#0", img->units[tgt].name);
            if (sop == NULL || cinst == NULL) {
              *err = diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
              return;
            }
            const char *ck = (in_tail && (uint32_t)tgt == unit_idx) ? "Tail" : "Normal";
            *err = emit3(fs, "callOf", col_sym(sop), col_sym(cinst), col_sym(ck), diag);
            if (*err != STIGI_OK) return;
            *err = emit2(fs, "operationOf", col_sym(sop), col_sym(inst_sym), diag);
            if (*err != STIGI_OK) return;
          }
        }
      }
      for (uint32_t i = 0; i < e->u.call.arg_count; i++) {
        summary_walk_calls(img, unit_idx, inst_sym, h, e->u.call.args[i], false, stub_seq,
                           has_undec_loop, has_fact, param_overwrites, param_returns, param_consumes, fs, diag, err);
      }
      break;
    case STIGI_EXPR_UNOP:
      summary_walk_calls(img, unit_idx, inst_sym, h, e->u.unop.operand, false, stub_seq,
                         has_undec_loop, has_fact, param_overwrites, param_returns, param_consumes, fs, diag, err);
      break;
    case STIGI_EXPR_BINOP:
      summary_walk_calls(img, unit_idx, inst_sym, h, e->u.binop.lhs, false, stub_seq,
                         has_undec_loop, has_fact, param_overwrites, param_returns, param_consumes, fs, diag, err);
      summary_walk_calls(img, unit_idx, inst_sym, h, e->u.binop.rhs, false, stub_seq,
                         has_undec_loop, has_fact, param_overwrites, param_returns, param_consumes, fs, diag, err);
      break;
    case STIGI_EXPR_IF:
      summary_walk_calls(img, unit_idx, inst_sym, h, e->u.if_expr.cond, false, stub_seq,
                         has_undec_loop, has_fact, param_overwrites, param_returns, param_consumes, fs, diag, err);
      summary_walk_calls(img, unit_idx, inst_sym, h, e->u.if_expr.then_br, in_tail, stub_seq,
                         has_undec_loop, has_fact, param_overwrites, param_returns, param_consumes, fs, diag, err);
      summary_walk_calls(img, unit_idx, inst_sym, h, e->u.if_expr.else_br, in_tail, stub_seq,
                         has_undec_loop, has_fact, param_overwrites, param_returns, param_consumes, fs, diag, err);
      break;
    case STIGI_EXPR_WHILE: {
      bool has_dec = (e->u.while_expr.decreases != NULL);
      if (!has_dec && e->u.while_expr.body != NULL &&
          e->u.while_expr.body->kind == STIGI_EXPR_BLOCK &&
          e->u.while_expr.body->u.block.count > 0 &&
          e->u.while_expr.body->u.block.stmts[0]->kind == STIGI_EXPR_ASSERT &&
          e->u.while_expr.body->u.block.stmts[0]->u.assert_expr.expr == NULL) {
        has_dec = (find_decreases_in_fg(&e->u.while_expr.body->u.block.stmts[0]->u.assert_expr.facts) != NULL);
      }
      if (!has_dec && has_undec_loop != NULL) *has_undec_loop = true;
      summary_walk_calls(img, unit_idx, inst_sym, h, e->u.while_expr.cond, false, stub_seq,
                         has_undec_loop, has_fact, param_overwrites, param_returns, param_consumes, fs, diag, err);
      summary_walk_calls(img, unit_idx, inst_sym, h, e->u.while_expr.body, false, stub_seq,
                         has_undec_loop, has_fact, param_overwrites, param_returns, param_consumes, fs, diag, err);
      break;
    }
    case STIGI_EXPR_MATCH:
      summary_walk_calls(img, unit_idx, inst_sym, h, e->u.match_expr.scrutinee, false, stub_seq,
                         has_undec_loop, has_fact, param_overwrites, param_returns, param_consumes, fs, diag, err);
      for (uint32_t i = 0; i < e->u.match_expr.arm_count; i++) {
        summary_walk_calls(img, unit_idx, inst_sym, h, e->u.match_expr.arms[i].body, in_tail,
                           stub_seq, has_undec_loop, has_fact, param_overwrites, param_returns,
                           param_consumes, fs, diag, err);
      }
      break;
    case STIGI_EXPR_BLOCK:
      for (uint32_t i = 0; i < e->u.block.count; i++) {
        bool last = (i + 1U == e->u.block.count);
        summary_walk_calls(img, unit_idx, inst_sym, h, e->u.block.stmts[i], in_tail && last,
                           stub_seq, has_undec_loop, has_fact, param_overwrites, param_returns,
                           param_consumes, fs, diag, err);
      }
      break;
    case STIGI_EXPR_VAL:
      summary_walk_calls(img, unit_idx, inst_sym, h, e->u.val_stmt.init, false, stub_seq,
                         has_undec_loop, has_fact, param_overwrites, param_returns, param_consumes, fs, diag, err);
      break;
    case STIGI_EXPR_VAR:
      if (e->u.var_stmt.has_inv && fact_group_has_safety(&e->u.var_stmt.inv) && has_fact != NULL) {
        *has_fact = true;
      }
      summary_walk_calls(img, unit_idx, inst_sym, h, e->u.var_stmt.init, false, stub_seq,
                         has_undec_loop, has_fact, param_overwrites, param_returns, param_consumes, fs, diag, err);
      break;
    case STIGI_EXPR_ASSIGN:
      summary_walk_calls(img, unit_idx, inst_sym, h, e->u.assign_stmt.value, false, stub_seq,
                         has_undec_loop, has_fact, param_overwrites, param_returns, param_consumes, fs, diag, err);
      break;
    case STIGI_EXPR_HANDLE:
      for (uint32_t i = 0; i < e->u.handle_stmt.arg_count; i++) {
        summary_walk_calls(img, unit_idx, inst_sym, h, e->u.handle_stmt.args[i], false, stub_seq,
                           has_undec_loop, has_fact, param_overwrites, param_returns, param_consumes, fs, diag, err);
      }
      summary_walk_calls(img, unit_idx, inst_sym, h, e->u.handle_stmt.body, in_tail, stub_seq,
                         has_undec_loop, has_fact, param_overwrites, param_returns, param_consumes, fs, diag, err);
      break;
    case STIGI_EXPR_ASSERT:
      if (fact_group_has_safety(&e->u.assert_expr.facts) && has_fact != NULL) *has_fact = true;
      summary_walk_calls(img, unit_idx, inst_sym, h, e->u.assert_expr.expr, in_tail, stub_seq,
                         has_undec_loop, has_fact, param_overwrites, param_returns, param_consumes, fs, diag, err);
      break;
    default:
      break;
  }
}

static Status extract_def_summary(const StigiImage *img, uint32_t unit_idx, const char *u_sym,
                                  const char *inst_sym, const StigiDefHead *h,
                                  const StigiExpr *body, bool is_lemma, uint64_t frame_bytes,
                                  StigiFactSet *fs, Diag *diag) {
  Status st = emit2(fs, "instanceOf", col_sym(inst_sym), col_sym(u_sym), diag);
  if (st != STIGI_OK) return st;
  st = emit1(fs, "defHasBody", col_sym(u_sym), diag);
  if (st != STIGI_OK) return st;
  st = emit2(fs, "frameBytes", col_sym(inst_sym), col_u64(frame_bytes), diag);
  if (st != STIGI_OK) return st;
  if (is_lemma) {
    st = emit1(fs, "lemmaUnit", col_sym(u_sym), diag);
    if (st != STIGI_OK) return st;
    if (body != NULL) {
      st = emit1(fs, "lemmaHasBody", col_sym(u_sym), diag);
      if (st != STIGI_OK) return st;
    }
  }
  bool has_fact = false;
  const StigiExpr *dec_expr = h->decreases;
  for (uint32_t i = 0; i < h->pre_group_count; i++) {
    if (h->pre_groups[i].is_fact) {
      if (fact_group_has_safety(&h->pre_groups[i].facts)) has_fact = true;
      if (dec_expr == NULL) dec_expr = find_decreases_in_fg(&h->pre_groups[i].facts);
    } else {
      for (uint32_t c = 0; c < h->pre_groups[i].cap_count; c++) {
        StigiStr cn = h->pre_groups[i].caps[c].cap_type->name;
        const char *cs = fs_strdup(fs, cn.ptr, cn.len);
        if (cs == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
        st = emit2(fs, "rowOf", col_sym(u_sym), col_sym(cs), diag);
        if (st != STIGI_OK) return st;
      }
    }
  }
  for (uint32_t i = 0; i < h->post_val_fact_count; i++) {
    if (fact_group_has_safety(&h->post_val_facts[i])) has_fact = true;
    if (dec_expr == NULL) dec_expr = find_decreases_in_fg(&h->post_val_facts[i]);
  }
  for (uint32_t i = 0; i < h->exit_fact_count; i++) {
    if (fact_group_has_safety(&h->exit_facts[i])) has_fact = true;
  }
  if (dec_expr != NULL && (dec_expr->kind == STIGI_EXPR_LOCAL || dec_expr->kind == STIGI_EXPR_GLOBAL)) {
    for (uint32_t i = 0; i < h->param_count; i++) {
      if (h->params[i].name.len == dec_expr->u.ident.name.len &&
          memcmp(h->params[i].name.ptr, dec_expr->u.ident.name.ptr,
                 dec_expr->u.ident.name.len) == 0) {
        int64_t lim = 0;
        if (try_extract_depth_bound(img, unit_idx, h, h->params[i].name, &lim)) {
          st = emit2(fs, "depthBound", col_sym(inst_sym), col_i64(lim), diag);
          if (st != STIGI_OK) return st;
        }
        break;
      }
    }
  }

  uint32_t stub_seq = 0;
  bool has_undec_loop = false;
  uint32_t pc = h->param_count;
  bool *p_ow = pc > 0 ? calloc(pc, sizeof *p_ow) : NULL;
  bool *p_ret = pc > 0 ? calloc(pc, sizeof *p_ret) : NULL;
  bool *p_con = pc > 0 ? calloc(pc, sizeof *p_con) : NULL;
  summary_walk_calls(img, unit_idx, inst_sym, h, body, true, &stub_seq, &has_undec_loop, &has_fact,
                     p_ow, p_ret, p_con, fs, diag, &st);
  if (st == STIGI_OK && has_fact) {
    st = emit2(fs, "statesFact", col_sym(u_sym), col_sym("Entry"), diag);
  }
  if (st == STIGI_OK && has_undec_loop) {
    const char *lsite = fs_fmt(fs, "%s:sumloop", inst_sym);
    if (lsite == NULL) st = diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
    else st = emit2(fs, "loopOf", col_sym(lsite), col_sym(inst_sym), diag);
  }
  for (uint32_t k = 0; k < pc && st == STIGI_OK; k++) {
    bool needs_sum = (p_ow != NULL && p_ow[k]) || (p_ret != NULL && p_ret[k]) ||
                     (p_con != NULL && p_con[k]);
    if (!needs_sum || !type_is_handle_carrying(img, unit_idx, h->params[k].type, 0)) continue;
    const char *ps = fs_fmt(fs, "%s:sp:%u", inst_sym, k);
    const char *sp = fs_fmt(fs, "%s:spt", inst_sym);
    if (ps == NULL || sp == NULL) { st = diag_set(diag, STIGI_ERR_OOM, -1, "OOM"); break; }
    st = emit3(fs, "param", col_sym(inst_sym), col_i64((int64_t)k), col_sym(ps), diag);
    if (st == STIGI_OK && p_ow != NULL && p_ow[k]) st = emit2(fs, "overwrite", col_sym(sp), col_sym(ps), diag);
    if (st == STIGI_OK && p_ret != NULL && p_ret[k]) st = emit2(fs, "returnOf", col_sym(sp), col_sym(ps), diag);
    if (st == STIGI_OK && p_con != NULL && p_con[k]) st = emit2(fs, "consume", col_sym(sp), col_sym(ps), diag);
  }
  free(p_ow);
  free(p_ret);
  free(p_con);
  return st;
}

static Status extract_one_unit_inner(const StigiImage *img, uint32_t unit_idx,
                                     bool force_root, bool summary_only,
                                     StigiFactSet *fs, Diag *diag) {
  const StigiImageUnit *u = &img->units[unit_idx];
  const StigiDecl *d = u->ast->decl;
  const char *u_sym = fs_strdup(fs, u->name, u->name_len);
  if (u_sym == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");

  Status st = emit2(fs, "unit", col_sym(u_sym), col_sym(decl_kind_sym(d->kind)), diag);
  if (st != STIGI_OK) return st;
  st = emit2(fs, "unitName", col_sym(u_sym), col_sym(u_sym), diag);
  if (st != STIGI_OK) return st;
  if (u->is_root || force_root) {
    st = emit1(fs, "root", col_sym(u_sym), diag);
    if (st != STIGI_OK) return st;
  }
  if (u->name_len >= 6 && memcmp(u->name, "Stigi.", 6) == 0) {
    st = emit1(fs, "underStigi", col_sym(u_sym), diag);
    if (st != STIGI_OK) return st;
  }

  /* Enum: handleCarrying */
  if (d->kind == STIGI_DECL_ENUM) {
    StigiType ety;
    memset(&ety, 0, sizeof ety);
    ety.kind = STIGI_TYPE_NAMED;
    ety.name.ptr = u->name;
    ety.name.len = u->name_len;
    if (type_is_handle_carrying(img, unit_idx, &ety, 0)) {
      st = emit1(fs, "handleCarrying", col_sym(u_sym), diag);
      if (st != STIGI_OK) return st;
    }
    return STIGI_OK;
  }

  /* Relation: relationColumns & userRelation */
  if (d->kind == STIGI_DECL_RELATION) {
    st = emit2(fs, "relationColumns", col_sym(u_sym),
               col_i64((int64_t)d->u.relation_decl.column_count), diag);
    if (st != STIGI_OK) return st;
    if (!(u->name_len >= 6 && memcmp(u->name, "Stigi.", 6) == 0)) {
      st = emit1(fs, "userRelation", col_sym(u_sym), diag);
      if (st != STIGI_OK) return st;
    }
    return STIGI_OK;
  }

  /* Rule: kernelRule, ruleHeadKernel, ruleReads, clearsRule */
  if (d->kind == STIGI_DECL_RULE) {
    bool is_k = (u->name_len >= 6 && memcmp(u->name, "Stigi.", 6) == 0);
    if (is_k) {
      st = emit1(fs, "kernelRule", col_sym(u_sym), diag);
      if (st != STIGI_OK) return st;
    }
    const StigiFactItem *hd = &d->u.rule_decl.head;
    if (hd->kind == STIGI_FACT_ATOM) {
      if (hd->atom_name.len >= 6 && memcmp(hd->atom_name.ptr, "Stigi.", 6) == 0) {
        st = emit1(fs, "ruleHeadKernel", col_sym(u_sym), diag);
        if (st != STIGI_OK) return st;
      }
      if ((str_eq_c(hd->atom_name, "cleared") || str_eq_c(hd->atom_name, "Stigi.cleared")) &&
          hd->arg_count == 2 && hd->args[1]->kind == STIGI_EXPR_LIT_STR &&
          hd->args[1]->u.lit_str.raw.len >= 2) {
        const char *cs = fs_strdup(fs, hd->args[1]->u.lit_str.raw.ptr + 1,
                                   hd->args[1]->u.lit_str.raw.len - 2);
        if (cs == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
        st = emit2(fs, "clearsRule", col_sym(u_sym), col_sym(cs), diag);
        if (st != STIGI_OK) return st;
      }
    }
    for (uint32_t i = 0; i < d->u.rule_decl.body_count; i++) {
      const StigiFactItem *bi = &d->u.rule_decl.body[i];
      if (bi->kind != STIGI_FACT_ATOM) continue;
      int32_t ridx = stigi_image_resolve_name(img, unit_idx, bi->atom_name.ptr, bi->atom_name.len);
      const char *rn = ridx >= 0 ? fs_strdup(fs, img->units[ridx].name, img->units[ridx].name_len)
                                 : fs_strdup(fs, bi->atom_name.ptr, bi->atom_name.len);
      if (rn == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
      st = emit2(fs, "ruleReads", col_sym(u_sym), col_sym(rn), diag);
      if (st != STIGI_OK) return st;
    }
    return STIGI_OK;
  }

  /* Test: emit testRow for every capability in its row (catches test-leaks-capability in Datalog) */
  if (d->kind == STIGI_DECL_TEST) {
    for (uint32_t c = 0; c < d->u.val_decl.cap_count; c++) {
      if (d->u.val_decl.caps[c].cap_type == NULL) continue;
      StigiStr cn = d->u.val_decl.caps[c].cap_type->name;
      const char *cs = fs_strdup(fs, cn.ptr, cn.len);
      if (cs == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
      st = emit2(fs, "testRow", col_sym(u_sym), col_sym(cs), diag);
      if (st != STIGI_OK) return st;
    }
    return STIGI_OK;
  }

  /* Extern: body-less (S72) or body-ful C-DSL */
  if (d->kind == STIGI_DECL_EXTERN && !d->u.extern_decl.is_val) {
    const char *inst_sym = fs_fmt(fs, "%s#0", u_sym);
    if (inst_sym == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
    const StigiDefHead *eh = &d->u.extern_decl.head;
    if (d->u.extern_decl.body == NULL) {
      st = emit2(fs, "instanceOf", col_sym(inst_sym), col_sym(u_sym), diag);
      if (st != STIGI_OK) return st;
      st = emit1(fs, "bodylessExtern", col_sym(inst_sym), diag);
      if (st != STIGI_OK) return st;
      for (uint32_t p = 0; p < eh->param_count; p++) {
        if (type_is_handle_carrying(img, unit_idx, eh->params[p].type, 0)) {
          st = emit2(fs, "externParam", col_sym(inst_sym), col_i64((int64_t)p), diag);
          if (st != STIGI_OK) return st;
        }
      }
      /* Scan extern facts for externStack(bytes) and summary atoms */
      for (uint32_t g = 0; g < eh->pre_group_count + eh->post_val_fact_count + eh->exit_fact_count; g++) {
        const StigiFactGroup *fg = NULL;
        if (g < eh->pre_group_count) {
          fg = eh->pre_groups[g].is_fact ? &eh->pre_groups[g].facts : NULL;
        } else if (g < eh->pre_group_count + eh->post_val_fact_count) {
          fg = &eh->post_val_facts[g - eh->pre_group_count];
        } else {
          fg = &eh->exit_facts[g - eh->pre_group_count - eh->post_val_fact_count];
        }
        if (fg == NULL) continue;
        for (uint32_t i = 0; i < fg->count; i++) {
          const StigiFactItem *it = &fg->items[i];
          if (it->kind != STIGI_FACT_ATOM) continue;
          if ((str_eq_c(it->atom_name, "externStack") || str_eq_c(it->atom_name, "stackBytes")) &&
              it->arg_count == 1 && it->args[0]->kind == STIGI_EXPR_LIT_INT &&
              !it->args[0]->u.lit_int.overflow) {
            st = emit2(fs, "externStack", col_sym(inst_sym),
                       col_u64(it->args[0]->u.lit_int.value), diag);
            if (st != STIGI_OK) return st;
          } else if (str_eq_c(it->atom_name, "overwrites") ||
                     str_eq_c(it->atom_name, "borrows") ||
                     str_eq_c(it->atom_name, "consumes") ||
                     str_eq_c(it->atom_name, "resultFresh") ||
                     str_eq_c(it->atom_name, "externSummary")) {
            st = emit1(fs, "externSummary", col_sym(inst_sym), diag);
            if (st != STIGI_OK) return st;
          }
        }
      }
      return STIGI_OK;
    }
    if (summary_only) {
      return extract_def_summary(img, unit_idx, u_sym, inst_sym, eh, d->u.extern_decl.body,
                                 false, 64ULL, fs, diag);
    }
    return extract_def_body(img, unit_idx, u_sym, inst_sym, eh, d->u.extern_decl.body,
                            false, false, 64ULL, fs, diag);
  }

  /* Capability: states, start, stop, operations, entryState, exitState, stateRef */
  if (d->kind == STIGI_DECL_CAPABILITY) {
    for (uint32_t i = 0; i < d->u.cap_decl.state_count; i++) {
      const char *ss = fs_strdup(fs, d->u.cap_decl.states[i].ptr, d->u.cap_decl.states[i].len);
      if (ss == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
      st = emit2(fs, "stateOf", col_sym(u_sym), col_sym(ss), diag);
      if (st != STIGI_OK) return st;
    }
    if (d->u.cap_decl.start_state.len > 0) {
      const char *ss = fs_strdup(fs, d->u.cap_decl.start_state.ptr,
                                 d->u.cap_decl.start_state.len);
      if (ss == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
      st = emit2(fs, "startState", col_sym(u_sym), col_sym(ss), diag);
      if (st != STIGI_OK) return st;
    }
    for (uint32_t i = 0; i < d->u.cap_decl.stop_state_count; i++) {
      const char *ss = fs_strdup(fs, d->u.cap_decl.stop_states[i].ptr,
                                 d->u.cap_decl.stop_states[i].len);
      if (ss == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
      st = emit2(fs, "stopState", col_sym(u_sym), col_sym(ss), diag);
      if (st != STIGI_OK) return st;
    }
    for (uint32_t i = 0; i < d->u.cap_decl.member_count; i++) {
      const StigiDecl *m = d->u.cap_decl.members[i];
      if (m->kind != STIGI_DECL_DEF) continue;
      const char *op_s = fs_fmt(fs, "%s.%.*s", u_sym, (int)m->name.len, m->name.ptr);
      if (op_s == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
      st = emit2(fs, "capabilityOperation", col_sym(u_sym), col_sym(op_s), diag);
      if (st != STIGI_OK) return st;

      const StigiDefHead *mh = &m->u.def_decl.head;
      /* entryState from at(self, S) in entry facts */
      for (uint32_t g = 0; g < mh->pre_group_count + mh->post_val_fact_count; g++) {
        const StigiFactGroup *fg = g < mh->pre_group_count
                                       ? (mh->pre_groups[g].is_fact ? &mh->pre_groups[g].facts : NULL)
                                       : &mh->post_val_facts[g - mh->pre_group_count];
        if (fg == NULL) continue;
        for (uint32_t k = 0; k < fg->count; k++) {
          const StigiFactItem *it = &fg->items[k];
          if (it->kind == STIGI_FACT_ATOM &&
              (str_eq_c(it->atom_name, "at") || str_eq_c(it->atom_name, "Stigi.Facts.at")) &&
              it->arg_count == 2 &&
              (it->args[1]->kind == STIGI_EXPR_LOCAL || it->args[1]->kind == STIGI_EXPR_GLOBAL)) {
            const char *ss = fs_strdup(fs, it->args[1]->u.ident.name.ptr,
                                       it->args[1]->u.ident.name.len);
            const char *sr = fs_fmt(fs, "%s:es:%u:%u", op_s, g, k);
            if (ss == NULL || sr == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
            st = emit2(fs, "entryState", col_sym(op_s), col_sym(ss), diag);
            if (st != STIGI_OK) return st;
            st = emit3(fs, "stateRef", col_sym(sr), col_sym(u_sym), col_sym(ss), diag);
            if (st != STIGI_OK) return st;
          }
        }
      }
      /* exitState from at(self, T) in exit facts */
      for (uint32_t g = 0; g < mh->exit_fact_count; g++) {
        const StigiFactGroup *fg = &mh->exit_facts[g];
        for (uint32_t k = 0; k < fg->count; k++) {
          const StigiFactItem *it = &fg->items[k];
          if (it->kind == STIGI_FACT_ATOM &&
              (str_eq_c(it->atom_name, "at") || str_eq_c(it->atom_name, "Stigi.Facts.at")) &&
              it->arg_count == 2 &&
              (it->args[1]->kind == STIGI_EXPR_LOCAL || it->args[1]->kind == STIGI_EXPR_GLOBAL)) {
            const char *ss = fs_strdup(fs, it->args[1]->u.ident.name.ptr,
                                       it->args[1]->u.ident.name.len);
            const char *sr = fs_fmt(fs, "%s:xs:%u:%u", op_s, g, k);
            if (ss == NULL || sr == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
            st = emit2(fs, "exitState", col_sym(op_s), col_sym(ss), diag);
            if (st != STIGI_OK) return st;
            st = emit3(fs, "stateRef", col_sym(sr), col_sym(u_sym), col_sym(ss), diag);
            if (st != STIGI_OK) return st;
          }
        }
      }
    }
    return STIGI_OK;
  }

  /* Handler: handlerUnit + extract each handler operation def */
  if (d->kind == STIGI_DECL_HANDLER) {
    st = emit1(fs, "handlerUnit", col_sym(u_sym), diag);
    if (st != STIGI_OK || summary_only) return st;
    for (uint32_t i = 0; i < d->u.handler_decl.def_count; i++) {
      const StigiDecl *opd = d->u.handler_decl.defs[i];
      if (opd->kind != STIGI_DECL_DEF || stigi_def_body(opd) == NULL) continue;
      const char *op_inst = fs_fmt(fs, "%s.%.*s#0", u_sym, (int)opd->name.len, opd->name.ptr);
      if (op_inst == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
      /* Handler ops often wrap pure mocks/primitives: state an Entry fact if none explicit */
      st = emit2(fs, "statesFact", col_sym(u_sym), col_sym("Entry"), diag);
      if (st != STIGI_OK) return st;
      st = extract_def_body(img, unit_idx, u_sym, op_inst, &opd->u.def_decl.head,
                            stigi_def_body(opd), false, false, 128ULL, fs, diag);
      if (st != STIGI_OK) return st;
    }
    return STIGI_OK;
  }

  /* Trait / Given (S125): givenRow purity + sigProtocol + extract given methods */
  if (d->kind == STIGI_DECL_TRAIT) {
    for (uint32_t i = 0; i < d->u.trait_decl.member_count; i++) {
      const StigiDecl *md = d->u.trait_decl.members[i];
      if (md->kind != STIGI_DECL_DEF) continue;
      const char *op_inst = fs_fmt(fs, "%s.%.*s#0", u_sym, (int)md->name.len, md->name.ptr);
      if (op_inst == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
      st = emit2(fs, "instanceOf", col_sym(op_inst), col_sym(u_sym), diag);
      if (st != STIGI_OK) return st;
      st = emit2(fs, "frameBytes", col_sym(op_inst), col_u64(64ULL), diag);
      if (st != STIGI_OK) return st;
    }
    return STIGI_OK;
  }
  if (d->kind == STIGI_DECL_GIVEN) {
    if (summary_only) return STIGI_OK;
    bool eff = stigi_kanren_given_is_effectful(img, unit_idx);
    st = emit2(fs, "givenRow", col_sym(u_sym), col_sym(eff ? "Effectful" : "Ok"), diag);
    if (st != STIGI_OK) return st;
    for (uint32_t c = 0; c < d->u.given_decl.cap_param_count; c++) {
      const char *sp_atom = fs_fmt(fs, "%s:sp:%u", u_sym, c);
      bool is_conc = stigi_kanren_is_concrete_protocol(img, unit_idx,
                                                       d->u.given_decl.cap_params[c].cap_type);
      st = emit2(fs, "sigProtocol", col_sym(sp_atom), col_sym(is_conc ? "Concrete" : "Protocol"), diag);
      if (st != STIGI_OK) return st;
    }
    for (uint32_t i = 0; i < d->u.given_decl.def_count; i++) {
      const StigiDecl *opd = d->u.given_decl.defs[i];
      if (opd->kind != STIGI_DECL_DEF || stigi_def_body(opd) == NULL) continue;
      const char *op_inst = fs_fmt(fs, "%s.%.*s#0", u_sym, (int)opd->name.len, opd->name.ptr);
      if (op_inst == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
      st = emit2(fs, "statesFact", col_sym(u_sym), col_sym("Entry"), diag);
      if (st != STIGI_OK) return st;
      st = extract_def_body(img, unit_idx, u_sym, op_inst, &opd->u.def_decl.head,
                            stigi_def_body(opd), false, false, 128ULL, fs, diag);
      if (st != STIGI_OK) return st;
    }
    return STIGI_OK;
  }

  if (d->kind != STIGI_DECL_DEF && d->kind != STIGI_DECL_LEMMA_DEF) {
    return STIGI_OK;
  }

  const char *inst_sym = fs_fmt(fs, "%s#0", u_sym);
  if (inst_sym == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
  bool is_lemma = (d->kind == STIGI_DECL_LEMMA_DEF);
  uint64_t fbytes = 0ULL;
  if (!is_lemma) {
    if (stigi_def_frame_bytes(img, unit_idx, &fbytes) != STIGI_OK) fbytes = 64ULL;
  }
  if (summary_only) {
    return extract_def_summary(img, unit_idx, u_sym, inst_sym, &d->u.def_decl.head,
                               stigi_def_body(d), is_lemma, fbytes, fs, diag);
  }
  bool is_root_def = (!is_lemma && u->is_root);
  return extract_def_body(img, unit_idx, u_sym, inst_sym, &d->u.def_decl.head,
                          stigi_def_body(d), is_lemma, is_root_def, fbytes, fs, diag);
}

/* ---- Public extraction entry points ---- */

Status stigi_extract_unit(const StigiImage *img, uint32_t unit_idx,
                          StigiFactSet *fs, Diag *diag) {
  if (img == NULL || fs == NULL || unit_idx >= img->unit_count) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_extract_unit: invalid arguments");
  }
  Status st = emit_kernel_tables(fs, diag);
  if (st != STIGI_OK) return st;
  return extract_one_unit_inner(img, unit_idx, true, false, fs, diag);
}

static bool def_has_decreases(const StigiDefHead *h) {
  if (h->decreases != NULL) return true;
  for (uint32_t i = 0; i < h->pre_group_count; i++) {
    if (h->pre_groups[i].is_fact && find_decreases_in_fg(&h->pre_groups[i].facts) != NULL) {
      return true;
    }
  }
  for (uint32_t i = 0; i < h->post_val_fact_count; i++) {
    if (find_decreases_in_fg(&h->post_val_facts[i]) != NULL) return true;
  }
  return false;
}

Status stigi_extract_unit_closed(const StigiImage *img, uint32_t unit_idx,
                                 bool kernel_mode, StigiFactSet *fs, Diag *diag) {
  if (img == NULL || fs == NULL || unit_idx >= img->unit_count) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_extract_unit_closed: invalid arguments");
  }
  Status st = emit_kernel_tables(fs, diag);
  if (st != STIGI_OK) return st;
  st = emit1(fs, "imageMode", col_sym(kernel_mode ? "Kernel" : "User"), diag);
  if (st != STIGI_OK) return st;

  /* BFS over dependency edges from unit_idx so interprocedural checks see all callees/caps/handlers */
  uint32_t n = img->unit_count;
  bool *vis = calloc(n, sizeof *vis);
  uint32_t *q = malloc((size_t)n * sizeof *q);
  if (vis == NULL || q == NULL) {
    free(vis);
    free(q);
    return diag_set(diag, STIGI_ERR_OOM, -1, "extract_unit_closed OOM");
  }
  uint32_t qh = 0, qt = 0;
  vis[unit_idx] = true;
  q[qt++] = unit_idx;
  while (qh < qt) {
    uint32_t cur = q[qh++];
    for (uint32_t e = 0; e < img->edge_count; e++) {
      if (img->edges[e].from == cur && img->edges[e].to < n && !vis[img->edges[e].to]) {
        vis[img->edges[e].to] = true;
        q[qt++] = img->edges[e].to;
      }
    }
  }
  free(q);

  bool checking_lemma = (img->units[unit_idx].ast->decl->kind == STIGI_DECL_LEMMA_DEF);
  for (uint32_t i = 0; i < n && st == STIGI_OK; i++) {
    if (!vis[i]) continue;
    /* Do not pull unrelated Test/NegTest units into a non-test unit's Datalog check */
    StigiDeclKind dk = img->units[i].ast->decl->kind;
    if (i != unit_idx && (dk == STIGI_DECL_TEST || dk == STIGI_DECL_NEG_TEST)) continue;
    bool sum_only = (i != unit_idx);
    if (sum_only && checking_lemma && dk == STIGI_DECL_DEF &&
        def_has_decreases(&img->units[i].ast->decl->u.def_decl.head)) {
      sum_only = false;
    }
    st = extract_one_unit_inner(img, i, i == unit_idx, sum_only, fs, diag);
  }
  for (uint32_t e = 0; e < img->edge_count && st == STIGI_OK; e++) {
    uint32_t f = img->edges[e].from;
    uint32_t t = img->edges[e].to;
    if (f < n && t < n && vis[f] && vis[t]) {
      const char *fsym = fs_strdup(fs, img->units[f].name, img->units[f].name_len);
      const char *tsym = fs_strdup(fs, img->units[t].name, img->units[t].name_len);
      if (fsym == NULL || tsym == NULL) {
        st = diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
        break;
      }
      st = emit2(fs, "edge", col_sym(fsym), col_sym(tsym), diag);
    }
  }
  free(vis);
  return st;
}

Status stigi_extract_image(const StigiImage *img, StigiFactSet *fs, Diag *diag) {
  if (img == NULL || fs == NULL) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_extract_image: invalid arguments");
  }
  Status st = emit_kernel_tables(fs, diag);
  if (st != STIGI_OK) return st;
  for (uint32_t i = 0; i < img->unit_count; i++) {
    st = extract_one_unit_inner(img, i, false, false, fs, diag);
    if (st != STIGI_OK) return st;
  }
  for (uint32_t e = 0; e < img->edge_count; e++) {
    uint32_t f = img->edges[e].from;
    uint32_t t = img->edges[e].to;
    if (f < img->unit_count && t < img->unit_count) {
      const char *fsym = fs_strdup(fs, img->units[f].name, img->units[f].name_len);
      const char *tsym = fs_strdup(fs, img->units[t].name, img->units[t].name_len);
      if (fsym == NULL || tsym == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
      st = emit2(fs, "edge", col_sym(fsym), col_sym(tsym), diag);
      if (st != STIGI_OK) return st;
    }
  }
  return STIGI_OK;
}

/* ---- Load exact 128-bit facts into tupu ---- */

Status stigi_facts_load_into_dl(const StigiFactSet *fs, StigiDl *dl, Diag *diag) {
  if (fs == NULL || dl == NULL) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_facts_load_into_dl: invalid arguments");
  }
  for (uint32_t i = 0; i < fs->count; i++) {
    const StigiFactRow *r = &fs->rows[i];
    int rel_id = stigi_dl_rel_id(dl, r->rel);
    if (rel_id < 0) continue;
    int expected = stigi_dl_rel_ncols(dl, rel_id);
    if (expected != r->ncols) {
      return diag_set(diag, STIGI_ERR_DATALOG, -1,
                      "relation '%s' arity mismatch: schema=%d, emitted=%d",
                      r->rel, expected, r->ncols);
    }
    TupuI128 vals[STIGI_MAX_FACT_COLS];
    for (int c = 0; c < r->ncols; c++) {
      if (r->cols[c].is_num) {
        vals[c] = r->cols[c].num;
      } else {
        uint32_t sym_id = 0;
        Status st = stigi_dl_intern(dl, r->cols[c].sym, strlen(r->cols[c].sym), &sym_id, diag);
        if (st != STIGI_OK) return st;
        vals[c] = tupu_i128_from_u64((uint64_t)sym_id);
      }
    }
    Status st = stigi_dl_add_i128(dl, rel_id, vals, r->ncols, diag);
    if (st != STIGI_OK) return st;
  }
  return STIGI_OK;
}

/* ---- Canonical sorted reprint & export ---- */

static int cmp_fact_row(const void *pa, const void *pb) {
  const StigiFactRow *a = *(const StigiFactRow *const *)pa;
  const StigiFactRow *b = *(const StigiFactRow *const *)pb;
  int c = strcmp(a->rel, b->rel);
  if (c != 0) return c;
  if (a->ncols != b->ncols) return a->ncols < b->ncols ? -1 : 1;
  for (int i = 0; i < a->ncols; i++) {
    if (a->cols[i].is_num != b->cols[i].is_num) return a->cols[i].is_num ? -1 : 1;
    if (a->cols[i].is_num) {
      c = i128_cmp(a->cols[i].num, b->cols[i].num);
      if (c != 0) return c;
    } else {
      c = strcmp(a->cols[i].sym, b->cols[i].sym);
      if (c != 0) return c;
    }
  }
  return 0;
}

typedef struct {
  char *buf;
  size_t len;
  size_t cap;
  bool oom;
} StrBuf;

static void sb_append(StrBuf *sb, const char *s, size_t n) {
  if (sb->oom || n == 0) return;
  if (n > SIZE_MAX - sb->len - 1U) { sb->oom = true; return; }
  if (sb->len + n + 1U > sb->cap) {
    size_t ncap = sb->cap == 0 ? 1024U : sb->cap;
    while (ncap < sb->len + n + 1U) {
      if (ncap > SIZE_MAX / 2U) { sb->oom = true; return; }
      ncap *= 2U;
    }
    char *nb = realloc(sb->buf, ncap);
    if (nb == NULL) { sb->oom = true; return; }
    sb->buf = nb;
    sb->cap = ncap;
  }
  memcpy(sb->buf + sb->len, s, n);
  sb->len += n;
  sb->buf[sb->len] = '\0';
}

Status stigi_facts_print(const StigiFactSet *fs, char **out_str, size_t *out_len,
                         Arena *arena, Diag *diag) {
  if (fs == NULL || out_str == NULL || arena == NULL) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_facts_print: invalid arguments");
  }
  const StigiFactRow **sorted = NULL;
  if (fs->count > 0) {
    sorted = malloc((size_t)fs->count * sizeof *sorted);
    if (sorted == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
    for (uint32_t i = 0; i < fs->count; i++) sorted[i] = &fs->rows[i];
    qsort(sorted, fs->count, sizeof *sorted, cmp_fact_row);
  }

  StrBuf sb = {NULL, 0, 0, false};
  for (uint32_t i = 0; i < fs->count && !sb.oom; i++) {
    const StigiFactRow *r = sorted[i];
    sb_append(&sb, r->rel, strlen(r->rel));
    sb_append(&sb, "(", 1);
    for (int c = 0; c < r->ncols; c++) {
      if (c > 0) sb_append(&sb, ", ", 2);
      if (r->cols[c].is_num) {
        char nbuf[48];
        size_t nn = stigi_i128_format(r->cols[c].num, nbuf);
        sb_append(&sb, nbuf, nn);
      } else {
        sb_append(&sb, "\"", 1);
        sb_append(&sb, r->cols[c].sym, strlen(r->cols[c].sym));
        sb_append(&sb, "\"", 1);
      }
    }
    sb_append(&sb, ").\n", 3);
  }
  free(sorted);
  if (sb.oom) {
    free(sb.buf);
    return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
  }
  char *res = arena_alloc(arena, sb.len + 1U);
  if (res == NULL) { free(sb.buf); return diag_set(diag, STIGI_ERR_OOM, -1, "OOM"); }
  if (sb.buf != NULL) memcpy(res, sb.buf, sb.len + 1U);
  else res[0] = '\0';
  free(sb.buf);
  *out_str = res;
  if (out_len != NULL) *out_len = sb.len;
  return STIGI_OK;
}

static bool is_ident_c(unsigned char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

Status stigi_facts_write_dir(const StigiFactSet *fs, const char *dir,
                             const char *dl_src, Diag *diag) {
  if (fs == NULL || dir == NULL || dl_src == NULL) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_facts_write_dir: invalid arguments");
  }
  const char *p = dl_src;
  while (*p != '\0') {
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    if (strncmp(p, ".input", 6) == 0 && (p[6] == ' ' || p[6] == '\t')) {
      p += 6;
      while (*p != '\0' && *p != '\n') {
        while (*p == ' ' || *p == '\t' || *p == ',') p++;
        if (*p == '\0' || *p == '\n' || *p == '(' || (*p == '/' && p[1] == '/')) break;
        const char *start = p;
        while (is_ident_c((unsigned char)*p)) p++;
        size_t rlen = (size_t)(p - start);
        if (rlen > 0) {
          char path[512];
          int pn = snprintf(path, sizeof path, "%s/%.*s.facts", dir, (int)rlen, start);
          if (pn < 0 || (size_t)pn >= sizeof path) {
            return diag_set(diag, STIGI_ERR_IO, -1, "path too long for %.*s", (int)rlen, start);
          }
          FILE *f = fopen(path, "w");
          if (f == NULL) {
            return diag_set(diag, STIGI_ERR_IO, -1, "cannot create %s", path);
          }
          if (fclose(f) != 0) {
            return diag_set(diag, STIGI_ERR_IO, -1, "close failed on %s", path);
          }
        }
      }
    }
    while (*p != '\0' && *p != '\n') p++;
  }

  if (fs->count == 0) return STIGI_OK;
  const StigiFactRow **sorted = malloc((size_t)fs->count * sizeof *sorted);
  if (sorted == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
  for (uint32_t i = 0; i < fs->count; i++) sorted[i] = &fs->rows[i];
  qsort(sorted, fs->count, sizeof *sorted, cmp_fact_row);

  FILE *cur_f = NULL;
  const char *cur_rel = NULL;
  Status st = STIGI_OK;
  for (uint32_t i = 0; i < fs->count; i++) {
    const StigiFactRow *r = sorted[i];
    if (cur_rel == NULL || strcmp(cur_rel, r->rel) != 0) {
      if (cur_f != NULL) {
        if (fclose(cur_f) != 0) {
          cur_f = NULL;
          st = diag_set(diag, STIGI_ERR_IO, -1, "close failed on %s", cur_rel);
          break;
        }
        cur_f = NULL;
      }
      char path[512];
      int pn = snprintf(path, sizeof path, "%s/%s.facts", dir, r->rel);
      if (pn < 0 || (size_t)pn >= sizeof path) {
        st = diag_set(diag, STIGI_ERR_IO, -1, "path too long for %s", r->rel);
        break;
      }
      cur_f = fopen(path, "w");
      if (cur_f == NULL) {
        st = diag_set(diag, STIGI_ERR_IO, -1, "cannot open %s", path);
        break;
      }
      cur_rel = r->rel;
    }
    for (int c = 0; c < r->ncols; c++) {
      if (c > 0 && fputc('\t', cur_f) == EOF) {
        st = diag_set(diag, STIGI_ERR_IO, -1, "write error on %s", r->rel);
        break;
      }
      if (r->cols[c].is_num) {
        char nbuf[48];
        size_t nn = stigi_i128_format(r->cols[c].num, nbuf);
        if (fwrite(nbuf, 1, nn, cur_f) != nn) {
          st = diag_set(diag, STIGI_ERR_IO, -1, "write error on %s", r->rel);
          break;
        }
      } else {
        if (fputs(r->cols[c].sym, cur_f) == EOF) {
          st = diag_set(diag, STIGI_ERR_IO, -1, "write error on %s", r->rel);
          break;
        }
      }
    }
    if (st != STIGI_OK) break;
    if (fputc('\n', cur_f) == EOF) {
      st = diag_set(diag, STIGI_ERR_IO, -1, "write error on %s", r->rel);
      break;
    }
  }
  if (cur_f != NULL) {
    if (fclose(cur_f) != 0 && st == STIGI_OK) {
      st = diag_set(diag, STIGI_ERR_IO, -1, "close failed on %s", cur_rel);
    }
  }
  free(sorted);
  return st;
}
