#include "extract.h"

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static const char *fs_fmt(StigiFactSet *fs, const char *fmt, ...) {
  char tmp[256];
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(tmp, sizeof tmp, fmt, ap);
  va_end(ap);
  if (n < 0) return NULL;
  size_t len = (size_t)n < sizeof(tmp) ? (size_t)n : sizeof(tmp) - 1U;
  return fs_strdup(fs, tmp, len);
}

static StigiFactCol col_sym(const char *s) {
  StigiFactCol c;
  c.is_num = false;
  c.num = 0;
  c.sym = s != NULL ? s : "";
  return c;
}

static StigiFactCol col_num(int64_t v) {
  StigiFactCol c;
  c.is_num = true;
  c.num = v;
  c.sym = NULL;
  return c;
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

static Status emit_kernel_tables(StigiFactSet *fs, Diag *diag) {
  Status st = emit3(fs, "typeRange", col_sym("U8"), col_num(0), col_num(255), diag);
  if (st != STIGI_OK) return st;
  st = emit2(fs, "typeWidth", col_sym("U8"), col_num(8), diag);
  if (st != STIGI_OK) return st;
  st = emit3(fs, "typeRange", col_sym("U32"), col_num(0), col_num(4294967295LL), diag);
  if (st != STIGI_OK) return st;
  st = emit2(fs, "typeWidth", col_sym("U32"), col_num(32), diag);
  if (st != STIGI_OK) return st;
  st = emit3(fs, "typeRange", col_sym("U64"), col_num(0), col_num(9223372036854775807LL), diag);
  if (st != STIGI_OK) return st;
  st = emit2(fs, "typeWidth", col_sym("U64"), col_num(64), diag);
  if (st != STIGI_OK) return st;
  st = emit3(fs, "typeRange", col_sym("I32"), col_num(-2147483648LL), col_num(2147483647LL), diag);
  if (st != STIGI_OK) return st;
  st = emit2(fs, "typeWidth", col_sym("I32"), col_num(32), diag);
  if (st != STIGI_OK) return st;
  st = emit3(fs, "typeRange", col_sym("I64"), col_num(-9223372036854775807LL),
             col_num(9223372036854775807LL), diag);
  if (st != STIGI_OK) return st;
  st = emit2(fs, "typeWidth", col_sym("I64"), col_num(64), diag);
  if (st != STIGI_OK) return st;

  st = emit1(fs, "taintSource", col_sym("Stigi.File"), diag);
  if (st != STIGI_OK) return st;
  st = emit1(fs, "taintSource", col_sym("Stigi.Socket"), diag);
  if (st != STIGI_OK) return st;
  st = emit1(fs, "taintSource", col_sym("Stigi.Clock"), diag);
  if (st != STIGI_OK) return st;

  /* requiresRow table (S37, Fix #13) */
  static const char *const k_req[][2] = {
    {"IndexRead", "IndexRead"}, {"IndexRead", "Result"},
    {"IndexWrite", "IndexWrite"}, {"IndexWrite", "Overwrite"}, {"IndexWrite", "WriteVia"},
    {"Add", "AddOp"}, {"Add", "Operand"}, {"Add", "Result"},
    {"Sub", "SubtractOp"}, {"Sub", "Operand"}, {"Sub", "Result"},
    {"Mul", "MultiplyOp"}, {"Mul", "Operand"}, {"Mul", "Result"},
    {"Div", "QuotientOp"}, {"Div", "Operand"}, {"Div", "Result"},
    {"Mod", "RemainderOp"}, {"Mod", "Operand"}, {"Mod", "Result"},
    {"Cond", "Condition"}, {"Cond", "TrueEdge"}, {"Cond", "FalseEdge"},
    {"CondBool", "TrueEdge"}, {"CondBool", "FalseEdge"},
    {"Call", "CallOf"}, {"Call", "Operand"}, {"Call", "ArgVia"}, {"Call", "Result"},
    {"Call0", "CallOf"}, {"Call0", "Result"},
    {"LoopHead", "BackEdge"},
    {"Assert", "Assert"}, {"Val", "Val"}, {"Var", "Var"}, {"Assign", "Assign"},
    {"Lit", "Lit"}, {"Ref", "Ref"}, {"Match", "Match"}, {"MatchArm", "MatchArm"},
    {"Handle", "Handle"}, {"UnOp", "UnOp"}, {"Deny", "Deny"}, {"Block", "Block"}
  };
  for (size_t i = 0; i < sizeof(k_req) / sizeof(k_req[0]); i++) {
    st = emit2(fs, "requiresRow", col_sym(k_req[i][0]), col_sym(k_req[i][1]), diag);
    if (st != STIGI_OK) return st;
  }
  return STIGI_OK;
}

typedef struct {
  const char *sym;
  const char *ty;
} ExtVal;

typedef struct {
  StigiStr name;
  const char *val_sym;
  const char *len_sym;
  const char *type_sym;
  bool has_inv;
  StigiFactGroup inv;
} ExtVar;

typedef struct {
  const StigiImage *img;
  uint32_t uidx;
  const char *unit_sym;
  const char *inst_sym;
  StigiFactSet *fs;
  Diag *diag;
  uint32_t next_op;
  uint32_t next_pt;
  uint32_t next_sc;
  uint32_t next_val;
  const char *cur_pt;
  const char *cur_sc;
  ExtVar vars[128];
  uint32_t var_count;
  const char *cap_provs[32];
  const char *cap_units[32];
  uint32_t cap_count;
} ExtCtx;

static ExtVar *ext_find_var(ExtCtx *ctx, StigiStr name) {
  for (uint32_t i = ctx->var_count; i > 0; i--) {
    if (ctx->vars[i - 1].name.len == name.len &&
        memcmp(ctx->vars[i - 1].name.ptr, name.ptr, name.len) == 0) {
      return &ctx->vars[i - 1];
    }
  }
  return NULL;
}

static const char *ext_new_op(ExtCtx *ctx) {
  return fs_fmt(ctx->fs, "%s:o%u", ctx->inst_sym, ctx->next_op++);
}

static const char *ext_new_pt(ExtCtx *ctx) {
  const char *pt = fs_fmt(ctx->fs, "%s:p%u", ctx->inst_sym, ctx->next_pt++);
  if (ctx->cur_pt != NULL && pt != NULL) {
    (void)emit2(ctx->fs, "next", col_sym(ctx->cur_pt), col_sym(pt), ctx->diag);
  }
  ctx->cur_pt = pt;
  return pt;
}

static const char *ext_new_sc(ExtCtx *ctx, const char *parent_sc) {
  const char *sc = fs_fmt(ctx->fs, "%s:s%u", ctx->inst_sym, ctx->next_sc++);
  if (sc != NULL) {
    (void)emit1(ctx->fs, "scope", col_sym(sc), ctx->diag);
    if (parent_sc != NULL) {
      (void)emit2(ctx->fs, "parent", col_sym(sc), col_sym(parent_sc), ctx->diag);
    }
  }
  return sc;
}

static ExtVal ext_new_val(ExtCtx *ctx, const char *ty_sym) {
  const char *ty = ty_sym != NULL ? ty_sym : "U64";
  const char *v = fs_fmt(ctx->fs, "%s:v%u", ctx->inst_sym, ctx->next_val++);
  if (v != NULL) {
    (void)emit1(ctx->fs, "value", col_sym(v), ctx->diag);
    (void)emit2(ctx->fs, "valueType", col_sym(v), col_sym(ty), ctx->diag);
    if (strcmp(ty, "U64") == 0) {
      (void)emit1(ctx->fs, "u64", col_sym(v), ctx->diag);
    }
  }
  ExtVal ev = {v, ty};
  return ev;
}

static bool str_eq_c(StigiStr s, const char *lit) {
  size_t n = strlen(lit);
  return s.len == n && memcmp(s.ptr, lit, n) == 0;
}

/* Fix #14 (S21): only safety-relation atoms clear facts-missing */
static bool is_safety_atom(StigiStr name) {
  return str_eq_c(name, "below") || str_eq_c(name, "Stigi.Facts.below") ||
         str_eq_c(name, "lessOrEqual") || str_eq_c(name, "Stigi.Facts.lessOrEqual") ||
         str_eq_c(name, "at") || str_eq_c(name, "valid") ||
         str_eq_c(name, "overwrites") || str_eq_c(name, "Stigi.Facts.overwrites");
}

static bool fact_group_has_safety(const StigiFactGroup *fg) {
  for (uint32_t i = 0; i < fg->count; i++) {
    if (fg->items[i].kind == STIGI_FACT_ATOM && is_safety_atom(fg->items[i].atom_name)) {
      return true;
    }
  }
  return false;
}

static int64_t clamp_u64_to_i64(uint64_t v) {
  return v > (uint64_t)INT64_MAX ? INT64_MAX : (int64_t)v;
}

static const char *ext_resolve_fact_val(ExtCtx *ctx, const StigiExpr *e, const char *res_sym,
                                        const StigiParam *subst_params, const char *const *subst_vals,
                                        uint32_t subst_count) {
  if (e == NULL) return ext_new_val(ctx, "U64").sym;
  if (e->kind == STIGI_EXPR_LOCAL || e->kind == STIGI_EXPR_GLOBAL) {
    if (str_eq_c(e->u.ident.name, "result") && res_sym != NULL) return res_sym;
    for (uint32_t i = 0; i < subst_count; i++) {
      if (subst_params[i].name.len == e->u.ident.name.len &&
          memcmp(subst_params[i].name.ptr, e->u.ident.name.ptr, e->u.ident.name.len) == 0) {
        return subst_vals[i];
      }
    }
    ExtVar *ev = ext_find_var(ctx, e->u.ident.name);
    if (ev != NULL) return ev->val_sym;
    return fs_fmt(ctx->fs, "%s:sym:%.*s", ctx->inst_sym, (int)e->u.ident.name.len,
                  e->u.ident.name.ptr);
  }
  if (e->kind == STIGI_EXPR_LIT_INT) {
    ExtVal v = ext_new_val(ctx, "U64");
    (void)emit2(ctx->fs, "lit", col_sym(v.sym), col_num(clamp_u64_to_i64(e->u.lit_int.value)),
                ctx->diag);
    return v.sym;
  }
  return ext_new_val(ctx, "U64").sym;
}

static void ext_emit_credits_subst(ExtCtx *ctx, const StigiFactGroup *fg, const char *sc,
                                   const char *res_sym, const StigiParam *sp,
                                   const char *const *sv, uint32_t scnt) {
  for (uint32_t i = 0; i < fg->count; i++) {
    const StigiFactItem *it = &fg->items[i];
    if (it->kind != STIGI_FACT_ATOM || it->arg_count != 2) continue;
    const char *vx = ext_resolve_fact_val(ctx, it->args[0], res_sym, sp, sv, scnt);
    const char *vy = ext_resolve_fact_val(ctx, it->args[1], res_sym, sp, sv, scnt);
    if (str_eq_c(it->atom_name, "below") || str_eq_c(it->atom_name, "Stigi.Facts.below")) {
      (void)emit3(ctx->fs, "baseBelow", col_sym(vx), col_sym(vy), col_sym(sc), ctx->diag);
    } else if (str_eq_c(it->atom_name, "lessOrEqual") ||
               str_eq_c(it->atom_name, "Stigi.Facts.lessOrEqual")) {
      (void)emit3(ctx->fs, "baseLeq", col_sym(vx), col_sym(vy), col_sym(sc), ctx->diag);
    }
  }
}

static void ext_emit_owed_subst(ExtCtx *ctx, const char *op, const StigiFactGroup *fg,
                                const char *sc, const char *code, const char *res_sym,
                                const StigiParam *sp, const char *const *sv, uint32_t scnt) {
  for (uint32_t i = 0; i < fg->count; i++) {
    const StigiFactItem *it = &fg->items[i];
    if (it->kind != STIGI_FACT_ATOM || it->arg_count != 2) continue;
    const char *vx = ext_resolve_fact_val(ctx, it->args[0], res_sym, sp, sv, scnt);
    const char *vy = ext_resolve_fact_val(ctx, it->args[1], res_sym, sp, sv, scnt);
    if (str_eq_c(it->atom_name, "below") || str_eq_c(it->atom_name, "Stigi.Facts.below")) {
      (void)emit5(ctx->fs, "belowOwed", col_sym(op), col_sym(vx), col_sym(vy),
                  col_sym(sc), col_sym(code), ctx->diag);
    } else if (str_eq_c(it->atom_name, "lessOrEqual") ||
               str_eq_c(it->atom_name, "Stigi.Facts.lessOrEqual")) {
      (void)emit5(ctx->fs, "leqOwed", col_sym(op), col_sym(vx), col_sym(vy),
                  col_sym(sc), col_sym(code), ctx->diag);
    }
  }
}

static ExtVal ext_expr(ExtCtx *ctx, const StigiExpr *e, const char *hint_ty);

static ExtVal ext_expr(ExtCtx *ctx, const StigiExpr *e, const char *hint_ty) {
  if (e == NULL) return ext_new_val(ctx, "Unit");

  /* Exhaustive switch over StigiExprKind with NO default arm (S37) */
  switch (e->kind) {
    case STIGI_EXPR_LOCAL:
    case STIGI_EXPR_GLOBAL: {
      ExtVar *ev = ext_find_var(ctx, e->u.ident.name);
      const char *pt = ext_new_pt(ctx);
      const char *op = ext_new_op(ctx);
      (void)emit4(ctx->fs, "operation", col_sym(op), col_sym("Ref"), col_sym(pt),
                  col_sym(ctx->cur_sc), ctx->diag);
      (void)emit2(ctx->fs, "operationOf", col_sym(op), col_sym(ctx->inst_sym), ctx->diag);
      (void)emit2(ctx->fs, "emitted", col_sym(op), col_sym("Ref"), ctx->diag);
      if (ev != NULL) {
        (void)emit2(ctx->fs, "binderRead", col_sym(ev->val_sym), col_sym(pt), ctx->diag);
        ExtVal out = {ev->val_sym, ev->type_sym != NULL ? ev->type_sym : "U64"};
        return out;
      }
      ExtVal out = ext_new_val(ctx, hint_ty != NULL ? hint_ty : "U64");
      /* Nullary enum constructor -> builtBy (Fix #12) */
      if (e->u.ident.name.len > 0 && e->u.ident.name.ptr[0] >= 'A' &&
          e->u.ident.name.ptr[0] <= 'Z') {
        const char *cs = fs_strdup(ctx->fs, e->u.ident.name.ptr, e->u.ident.name.len);
        (void)emit2(ctx->fs, "builtBy", col_sym(out.sym), col_sym(cs), ctx->diag);
      }
      return out;
    }
    case STIGI_EXPR_LIT_INT: {
      const char *pt = ext_new_pt(ctx);
      const char *op = ext_new_op(ctx);
      const char *ty = (hint_ty != NULL && (strcmp(hint_ty, "U8") == 0 ||
                                            strcmp(hint_ty, "U32") == 0 ||
                                            strcmp(hint_ty, "U64") == 0 ||
                                            strcmp(hint_ty, "I32") == 0 ||
                                            strcmp(hint_ty, "I64") == 0))
                           ? hint_ty
                           : "U64";
      ExtVal v = ext_new_val(ctx, ty);
      (void)emit4(ctx->fs, "operation", col_sym(op), col_sym("Lit"), col_sym(pt),
                  col_sym(ctx->cur_sc), ctx->diag);
      (void)emit2(ctx->fs, "operationOf", col_sym(op), col_sym(ctx->inst_sym), ctx->diag);
      (void)emit2(ctx->fs, "emitted", col_sym(op), col_sym("Lit"), ctx->diag);
      (void)emit2(ctx->fs, "lit", col_sym(v.sym), col_num(clamp_u64_to_i64(e->u.lit_int.value)),
                  ctx->diag);
      return v;
    }
    case STIGI_EXPR_LIT_CHAR:
    case STIGI_EXPR_LIT_STR:
    case STIGI_EXPR_LIT_BOOL:
    case STIGI_EXPR_LIT_UNIT: {
      const char *pt = ext_new_pt(ctx);
      const char *op = ext_new_op(ctx);
      const char *ty = (e->kind == STIGI_EXPR_LIT_BOOL) ? "Bool"
                       : (e->kind == STIGI_EXPR_LIT_CHAR) ? "U8" : "Unit";
      ExtVal v = ext_new_val(ctx, ty);
      (void)emit4(ctx->fs, "operation", col_sym(op), col_sym("Lit"), col_sym(pt),
                  col_sym(ctx->cur_sc), ctx->diag);
      (void)emit2(ctx->fs, "operationOf", col_sym(op), col_sym(ctx->inst_sym), ctx->diag);
      (void)emit2(ctx->fs, "emitted", col_sym(op), col_sym("Lit"), ctx->diag);
      return v;
    }
    case STIGI_EXPR_CALL: {
      /* Array element read: localArray(index) */
      if (e->u.call.callee->kind == STIGI_EXPR_LOCAL && e->u.call.arg_count == 1) {
        ExtVar *ev = ext_find_var(ctx, e->u.call.callee->u.ident.name);
        if (ev != NULL && ev->len_sym != NULL) {
          ExtVal iv = ext_expr(ctx, e->u.call.args[0], "U64");
          const char *pt = ext_new_pt(ctx);
          const char *op = ext_new_op(ctx);
          ExtVal rv = ext_new_val(ctx, hint_ty != NULL ? hint_ty : "U64");
          (void)emit4(ctx->fs, "operation", col_sym(op), col_sym("IndexRead"), col_sym(pt),
                      col_sym(ctx->cur_sc), ctx->diag);
          (void)emit2(ctx->fs, "operationOf", col_sym(op), col_sym(ctx->inst_sym), ctx->diag);
          (void)emit4(ctx->fs, "indexRead", col_sym(op), col_sym(iv.sym), col_sym(ev->len_sym),
                      col_sym(ctx->cur_sc), ctx->diag);
          (void)emit2(ctx->fs, "result", col_sym(op), col_sym(rv.sym), ctx->diag);
          (void)emit2(ctx->fs, "emitted", col_sym(op), col_sym("IndexRead"), ctx->diag);
          (void)emit2(ctx->fs, "emitted", col_sym(op), col_sym("Result"), ctx->diag);
          (void)emit2(ctx->fs, "binderRead", col_sym(ev->val_sym), col_sym(pt), ctx->diag);
          return rv;
        }
      }

      const char *arg_vals[32];
      uint32_t argc = e->u.call.arg_count < 32 ? e->u.call.arg_count : 32;
      for (uint32_t i = 0; i < argc; i++) {
        arg_vals[i] = ext_expr(ctx, e->u.call.args[i], NULL).sym;
      }
      const char *pt = ext_new_pt(ctx);
      const char *op = ext_new_op(ctx);
      const char *ret_ty = hint_ty != NULL ? hint_ty : "U64";
      const char *callee_inst = "unknown#0";
      int32_t tgt = -1;
      if (e->u.call.callee->kind == STIGI_EXPR_LOCAL ||
          e->u.call.callee->kind == STIGI_EXPR_GLOBAL) {
        StigiStr cn = e->u.call.callee->u.ident.name;
        tgt = stigi_image_resolve_name(ctx->img, ctx->uidx, cn.ptr, cn.len);
        if (tgt >= 0) {
          callee_inst = fs_fmt(ctx->fs, "%s#0", ctx->img->units[tgt].name);
          const StigiDecl *cd = ctx->img->units[tgt].ast->decl;
          if (cd->kind == STIGI_DECL_DEF && cd->u.def_decl.head.ret_type != NULL) {
            ret_ty = fs_strdup(ctx->fs, cd->u.def_decl.head.ret_type->name.ptr,
                               cd->u.def_decl.head.ret_type->name.len);
          }
        } else {
          callee_inst = fs_fmt(ctx->fs, "%.*s#0", (int)cn.len, cn.ptr);
        }
      }
      ExtVal rv = ext_new_val(ctx, ret_ty);
      const char *call_kind = argc > 0 ? "Call" : "Call0";
      const char *cg_kind = (tgt >= 0 && (uint32_t)tgt == ctx->uidx) ? "Tail" : "Normal";
      (void)emit4(ctx->fs, "operation", col_sym(op), col_sym(call_kind), col_sym(pt),
                  col_sym(ctx->cur_sc), ctx->diag);
      (void)emit2(ctx->fs, "operationOf", col_sym(op), col_sym(ctx->inst_sym), ctx->diag);
      (void)emit3(ctx->fs, "callOf", col_sym(op), col_sym(callee_inst), col_sym(cg_kind), ctx->diag);
      (void)emit2(ctx->fs, "result", col_sym(op), col_sym(rv.sym), ctx->diag);
      (void)emit2(ctx->fs, "emitted", col_sym(op), col_sym("CallOf"), ctx->diag);
      (void)emit2(ctx->fs, "emitted", col_sym(op), col_sym("Result"), ctx->diag);
      for (uint32_t i = 0; i < argc; i++) {
        (void)emit3(ctx->fs, "operand", col_sym(op), col_num((int64_t)i), col_sym(arg_vals[i]),
                    ctx->diag);
        (void)emit3(ctx->fs, "callArg", col_sym(op), col_num((int64_t)i), col_sym(arg_vals[i]),
                    ctx->diag);
        (void)emit3(ctx->fs, "argVia", col_sym(op), col_num((int64_t)i), col_sym(arg_vals[i]),
                    ctx->diag);
      }
      if (argc > 0) {
        (void)emit2(ctx->fs, "emitted", col_sym(op), col_sym("Operand"), ctx->diag);
        (void)emit2(ctx->fs, "emitted", col_sym(op), col_sym("ArgVia"), ctx->diag);
      }
      /* Fix #13: callee preconditions, exit credits, capability needs/providers */
      if (tgt >= 0) {
        const StigiDecl *cd = ctx->img->units[tgt].ast->decl;
        if (cd->kind == STIGI_DECL_DEF || cd->kind == STIGI_DECL_LEMMA_DEF) {
          const StigiDefHead *ch = &cd->u.def_decl.head;
          uint32_t scnt = argc < ch->param_count ? argc : ch->param_count;
          for (uint32_t g = 0; g < ch->pre_group_count; g++) {
            if (ch->pre_groups[g].is_fact) {
              ext_emit_owed_subst(ctx, op, &ch->pre_groups[g].facts, ctx->cur_sc,
                                  "precondition-unproved", rv.sym, ch->params, arg_vals, scnt);
            } else {
              for (uint32_t c = 0; c < ch->pre_groups[g].cap_count; c++) {
                StigiStr cap_n = ch->pre_groups[g].caps[c].cap_type->name;
                const char *cap_s = fs_strdup(ctx->fs, cap_n.ptr, cap_n.len);
                (void)emit2(ctx->fs, "needsCapability", col_sym(op), col_sym(cap_s), ctx->diag);
                for (uint32_t p = 0; p < ctx->cap_count; p++) {
                  if (strcmp(ctx->cap_units[p], cap_s) == 0) {
                    (void)emit2(ctx->fs, "provider", col_sym(op), col_sym(ctx->cap_provs[p]),
                                ctx->diag);
                    break;
                  }
                }
              }
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
      } else if (e->u.call.callee->kind == STIGI_EXPR_LOCAL &&
                 e->u.call.callee->u.ident.name.len > 0 &&
                 e->u.call.callee->u.ident.name.ptr[0] >= 'A' &&
                 e->u.call.callee->u.ident.name.ptr[0] <= 'Z') {
        const char *cs = fs_strdup(ctx->fs, e->u.call.callee->u.ident.name.ptr,
                                   e->u.call.callee->u.ident.name.len);
        (void)emit2(ctx->fs, "builtBy", col_sym(rv.sym), col_sym(cs), ctx->diag);
      }
      return rv;
    }
    case STIGI_EXPR_BINOP: {
      /* Fix #10: propagate actual operand type (U8/U32/U64/I32/I64) instead of hardcoding U64 */
      ExtVal lv = ext_expr(ctx, e->u.binop.lhs, hint_ty);
      ExtVal rv = ext_expr(ctx, e->u.binop.rhs, lv.ty);
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
        (void)emit4(ctx->fs, "operation", col_sym(op), col_sym(kind_s), col_sym(pt),
                    col_sym(ctx->cur_sc), ctx->diag);
        (void)emit2(ctx->fs, "operationOf", col_sym(op), col_sym(ctx->inst_sym), ctx->diag);
        (void)emit5(ctx->fs, rel_s, col_sym(op), col_sym(res.sym), col_sym(lv.sym),
                    col_sym(rv.sym), col_sym(ctx->cur_sc), ctx->diag);
        (void)emit3(ctx->fs, "operand", col_sym(op), col_num(0), col_sym(lv.sym), ctx->diag);
        (void)emit3(ctx->fs, "operand", col_sym(op), col_num(1), col_sym(rv.sym), ctx->diag);
        (void)emit2(ctx->fs, "result", col_sym(op), col_sym(res.sym), ctx->diag);
        (void)emit2(ctx->fs, "emitted", col_sym(op), col_sym(row_s), ctx->diag);
        (void)emit2(ctx->fs, "emitted", col_sym(op), col_sym("Operand"), ctx->diag);
        (void)emit2(ctx->fs, "emitted", col_sym(op), col_sym("Result"), ctx->diag);
      } else {
        (void)emit4(ctx->fs, "operation", col_sym(op), col_sym("Ref"), col_sym(pt),
                    col_sym(ctx->cur_sc), ctx->diag);
        (void)emit2(ctx->fs, "operationOf", col_sym(op), col_sym(ctx->inst_sym), ctx->diag);
        (void)emit2(ctx->fs, "emitted", col_sym(op), col_sym("Ref"), ctx->diag);
      }
      return res;
    }
    case STIGI_EXPR_UNOP: {
      ExtVal opnd = ext_expr(ctx, e->u.unop.operand, hint_ty);
      const char *pt = ext_new_pt(ctx);
      const char *op = ext_new_op(ctx);
      if (e->u.unop.op == STIGI_OP_NEG) {
        /* Fix #10: negation is subtractOp(0 - x) */
        const char *ty = opnd.ty != NULL ? opnd.ty : "I64";
        ExtVal zero = ext_new_val(ctx, ty);
        (void)emit2(ctx->fs, "lit", col_sym(zero.sym), col_num(0), ctx->diag);
        ExtVal res = ext_new_val(ctx, ty);
        (void)emit4(ctx->fs, "operation", col_sym(op), col_sym("Sub"), col_sym(pt),
                    col_sym(ctx->cur_sc), ctx->diag);
        (void)emit2(ctx->fs, "operationOf", col_sym(op), col_sym(ctx->inst_sym), ctx->diag);
        (void)emit5(ctx->fs, "subtractOp", col_sym(op), col_sym(res.sym), col_sym(zero.sym),
                    col_sym(opnd.sym), col_sym(ctx->cur_sc), ctx->diag);
        (void)emit3(ctx->fs, "operand", col_sym(op), col_num(0), col_sym(zero.sym), ctx->diag);
        (void)emit3(ctx->fs, "operand", col_sym(op), col_num(1), col_sym(opnd.sym), ctx->diag);
        (void)emit2(ctx->fs, "result", col_sym(op), col_sym(res.sym), ctx->diag);
        (void)emit2(ctx->fs, "emitted", col_sym(op), col_sym("SubtractOp"), ctx->diag);
        (void)emit2(ctx->fs, "emitted", col_sym(op), col_sym("Operand"), ctx->diag);
        (void)emit2(ctx->fs, "emitted", col_sym(op), col_sym("Result"), ctx->diag);
        return res;
      }
      (void)emit4(ctx->fs, "operation", col_sym(op), col_sym("UnOp"), col_sym(pt),
                  col_sym(ctx->cur_sc), ctx->diag);
      (void)emit2(ctx->fs, "operationOf", col_sym(op), col_sym(ctx->inst_sym), ctx->diag);
      (void)emit2(ctx->fs, "emitted", col_sym(op), col_sym("UnOp"), ctx->diag);
      return ext_new_val(ctx, "Bool");
    }
    case STIGI_EXPR_IF: {
      /* Fix #12: handle <, <=, >, >=, ==, != AND general Bool conditions; emit phi at join */
      const char *pre_sc = ctx->cur_sc;
      const char *ts = ext_new_sc(ctx, pre_sc);
      const char *fs_sc = ext_new_sc(ctx, pre_sc);
      const char *cond_pt = ext_new_pt(ctx);
      const char *cond_op = ext_new_op(ctx);

      bool is_cmp = (e->u.if_expr.cond->kind == STIGI_EXPR_BINOP &&
                     (e->u.if_expr.cond->u.binop.op == STIGI_OP_LT ||
                      e->u.if_expr.cond->u.binop.op == STIGI_OP_LE ||
                      e->u.if_expr.cond->u.binop.op == STIGI_OP_GT ||
                      e->u.if_expr.cond->u.binop.op == STIGI_OP_GE ||
                      e->u.if_expr.cond->u.binop.op == STIGI_OP_EQ ||
                      e->u.if_expr.cond->u.binop.op == STIGI_OP_NE));
      if (is_cmp) {
        StigiOp cop = e->u.if_expr.cond->u.binop.op;
        ExtVal lx = ext_expr(ctx, e->u.if_expr.cond->u.binop.lhs, NULL);
        ExtVal ry = ext_expr(ctx, e->u.if_expr.cond->u.binop.rhs, lx.ty);
        const char *a = lx.sym, *b = ry.sym;
        const char *cmp_s = "Equal";
        if (cop == STIGI_OP_LT) {
          cmp_s = "Less";
          (void)emit3(ctx->fs, "baseBelow", col_sym(a), col_sym(b), col_sym(ts), ctx->diag);
          (void)emit3(ctx->fs, "baseLeq", col_sym(b), col_sym(a), col_sym(fs_sc), ctx->diag);
        } else if (cop == STIGI_OP_LE) {
          cmp_s = "LessOrEqual";
          (void)emit3(ctx->fs, "baseLeq", col_sym(a), col_sym(b), col_sym(ts), ctx->diag);
          (void)emit3(ctx->fs, "baseBelow", col_sym(b), col_sym(a), col_sym(fs_sc), ctx->diag);
        } else if (cop == STIGI_OP_GT) {
          cmp_s = "Less";
          a = ry.sym; b = lx.sym;
          (void)emit3(ctx->fs, "baseBelow", col_sym(a), col_sym(b), col_sym(ts), ctx->diag);
          (void)emit3(ctx->fs, "baseLeq", col_sym(b), col_sym(a), col_sym(fs_sc), ctx->diag);
        } else if (cop == STIGI_OP_GE) {
          cmp_s = "LessOrEqual";
          a = ry.sym; b = lx.sym;
          (void)emit3(ctx->fs, "baseLeq", col_sym(a), col_sym(b), col_sym(ts), ctx->diag);
          (void)emit3(ctx->fs, "baseBelow", col_sym(b), col_sym(a), col_sym(fs_sc), ctx->diag);
        } else if (cop == STIGI_OP_NE) {
          cmp_s = "NotEqual";
        }
        (void)emit4(ctx->fs, "operation", col_sym(cond_op), col_sym("Cond"), col_sym(cond_pt),
                    col_sym(pre_sc), ctx->diag);
        (void)emit2(ctx->fs, "operationOf", col_sym(cond_op), col_sym(ctx->inst_sym), ctx->diag);
        (void)emit6(ctx->fs, "condition", col_sym(cond_op), col_sym(a), col_sym(cmp_s),
                    col_sym(b), col_sym(ts), col_sym(fs_sc), ctx->diag);
        (void)emit2(ctx->fs, "emitted", col_sym(cond_op), col_sym("Condition"), ctx->diag);
        (void)emit2(ctx->fs, "emitted", col_sym(cond_op), col_sym("TrueEdge"), ctx->diag);
        (void)emit2(ctx->fs, "emitted", col_sym(cond_op), col_sym("FalseEdge"), ctx->diag);
      } else {
        (void)ext_expr(ctx, e->u.if_expr.cond, "Bool");
        (void)emit4(ctx->fs, "operation", col_sym(cond_op), col_sym("CondBool"), col_sym(cond_pt),
                    col_sym(pre_sc), ctx->diag);
        (void)emit2(ctx->fs, "operationOf", col_sym(cond_op), col_sym(ctx->inst_sym), ctx->diag);
        (void)emit2(ctx->fs, "emitted", col_sym(cond_op), col_sym("TrueEdge"), ctx->diag);
        (void)emit2(ctx->fs, "emitted", col_sym(cond_op), col_sym("FalseEdge"), ctx->diag);
      }

      ctx->cur_sc = ts;
      ctx->cur_pt = cond_pt;
      const char *t_start = ext_new_pt(ctx);
      (void)emit2(ctx->fs, "trueEdge", col_sym(cond_op), col_sym(t_start), ctx->diag);
      ExtVal tv = ext_expr(ctx, e->u.if_expr.then_br, hint_ty);
      const char *t_end = ctx->cur_pt;

      ctx->cur_sc = fs_sc;
      ctx->cur_pt = cond_pt;
      const char *f_start = ext_new_pt(ctx);
      (void)emit2(ctx->fs, "falseEdge", col_sym(cond_op), col_sym(f_start), ctx->diag);
      ExtVal fv = ext_expr(ctx, e->u.if_expr.else_br, tv.ty);
      const char *f_end = ctx->cur_pt;

      const char *join_pt = fs_fmt(ctx->fs, "%s:p%u", ctx->inst_sym, ctx->next_pt++);
      (void)emit2(ctx->fs, "next", col_sym(t_end), col_sym(join_pt), ctx->diag);
      (void)emit2(ctx->fs, "next", col_sym(f_end), col_sym(join_pt), ctx->diag);
      ctx->cur_pt = join_pt;
      ctx->cur_sc = pre_sc;

      ExtVal merged = ext_new_val(ctx, tv.ty);
      (void)emit6(ctx->fs, "phi", col_sym(merged.sym), col_sym(tv.sym), col_sym(ts),
                  col_sym(fv.sym), col_sym(fs_sc), col_sym(pre_sc), ctx->diag);
      return merged;
    }
    case STIGI_EXPR_WHILE: {
      /* Fix #15: ALWAYS emit loopSite, loopOf, and backEdge even when decreases == NULL */
      const char *head_pt = ext_new_pt(ctx);
      const char *head_op = ext_new_op(ctx);
      const char *body_sc = ext_new_sc(ctx, ctx->cur_sc);
      const char *site = fs_fmt(ctx->fs, "%s:loop%s", ctx->inst_sym, head_pt);
      (void)emit4(ctx->fs, "operation", col_sym(head_op), col_sym("LoopHead"), col_sym(head_pt),
                  col_sym(ctx->cur_sc), ctx->diag);
      (void)emit2(ctx->fs, "operationOf", col_sym(head_op), col_sym(ctx->inst_sym), ctx->diag);
      (void)emit2(ctx->fs, "emitted", col_sym(head_op), col_sym("BackEdge"), ctx->diag);
      (void)emit2(ctx->fs, "loopSite", col_sym(site), col_sym(head_pt), ctx->diag);
      (void)emit2(ctx->fs, "loopOf", col_sym(site), col_sym(ctx->inst_sym), ctx->diag);
      (void)ext_expr(ctx, e->u.while_expr.cond, "Bool");
      const char *saved_sc = ctx->cur_sc;
      ctx->cur_sc = body_sc;
      (void)ext_expr(ctx, e->u.while_expr.body, NULL);
      const char *back_pt = ctx->cur_pt;
      (void)emit2(ctx->fs, "backEdge", col_sym(back_pt), col_sym(head_pt), ctx->diag);
      (void)emit2(ctx->fs, "next", col_sym(back_pt), col_sym(head_pt), ctx->diag);
      if (e->u.while_expr.decreases != NULL) {
        const char *mv = ext_resolve_fact_val(ctx, e->u.while_expr.decreases, NULL, NULL, NULL, 0);
        (void)emit2(ctx->fs, "decreasesAt", col_sym(site), col_sym(mv), ctx->diag);
      }
      ctx->cur_sc = saved_sc;
      return ext_new_val(ctx, "Unit");
    }
    case STIGI_EXPR_MATCH: {
      /* Fix #12: emit matchArm + phi across arms */
      ExtVal sv = ext_expr(ctx, e->u.match_expr.scrutinee, NULL);
      const char *pre_sc = ctx->cur_sc;
      const char *pt = ext_new_pt(ctx);
      const char *op = ext_new_op(ctx);
      (void)emit4(ctx->fs, "operation", col_sym(op), col_sym("Match"), col_sym(pt),
                  col_sym(pre_sc), ctx->diag);
      (void)emit2(ctx->fs, "operationOf", col_sym(op), col_sym(ctx->inst_sym), ctx->diag);
      (void)emit2(ctx->fs, "emitted", col_sym(op), col_sym("Match"), ctx->diag);
      ExtVal acc = ext_new_val(ctx, hint_ty != NULL ? hint_ty : "U64");
      const char *prev_sc = pre_sc;
      for (uint32_t i = 0; i < e->u.match_expr.arm_count; i++) {
        const char *arm_sc = ext_new_sc(ctx, pre_sc);
        ctx->cur_sc = arm_sc;
        const char *arm_pt = ext_new_pt(ctx);
        const char *arm_op = ext_new_op(ctx);
        const char *cname = fs_strdup(ctx->fs, e->u.match_expr.arms[i].pat.name.ptr,
                                      e->u.match_expr.arms[i].pat.name.len);
        (void)emit4(ctx->fs, "operation", col_sym(arm_op), col_sym("MatchArm"), col_sym(arm_pt),
                    col_sym(arm_sc), ctx->diag);
        (void)emit2(ctx->fs, "operationOf", col_sym(arm_op), col_sym(ctx->inst_sym), ctx->diag);
        (void)emit3(ctx->fs, "matchArm", col_sym(arm_op), col_sym(sv.sym), col_sym(cname),
                    ctx->diag);
        (void)emit2(ctx->fs, "emitted", col_sym(arm_op), col_sym("MatchArm"), ctx->diag);
        ExtVal av = ext_expr(ctx, e->u.match_expr.arms[i].body, hint_ty);
        if (i == 0) {
          acc = av;
          prev_sc = arm_sc;
        } else {
          ExtVal merged = ext_new_val(ctx, acc.ty);
          (void)emit6(ctx->fs, "phi", col_sym(merged.sym), col_sym(acc.sym), col_sym(prev_sc),
                      col_sym(av.sym), col_sym(arm_sc), col_sym(pre_sc), ctx->diag);
          acc = merged;
          prev_sc = pre_sc;
        }
      }
      ctx->cur_sc = pre_sc;
      return acc;
    }
    case STIGI_EXPR_VAL: {
      const char *ty = e->u.val_stmt.type != NULL
                           ? fs_strdup(ctx->fs, e->u.val_stmt.type->name.ptr,
                                       e->u.val_stmt.type->name.len)
                           : NULL;
      ExtVal iv = ext_expr(ctx, e->u.val_stmt.init, ty);
      const char *pt = ext_new_pt(ctx);
      const char *op = ext_new_op(ctx);
      (void)emit4(ctx->fs, "operation", col_sym(op), col_sym("Val"), col_sym(pt),
                  col_sym(ctx->cur_sc), ctx->diag);
      (void)emit2(ctx->fs, "operationOf", col_sym(op), col_sym(ctx->inst_sym), ctx->diag);
      (void)emit2(ctx->fs, "emitted", col_sym(op), col_sym("Val"), ctx->diag);
      if (e->u.val_stmt.pat.kind == STIGI_PAT_VAR && ctx->var_count < 128) {
        ExtVar *v = &ctx->vars[ctx->var_count++];
        memset(v, 0, sizeof *v);
        v->name = e->u.val_stmt.pat.name;
        v->val_sym = iv.sym;
        v->type_sym = ty != NULL ? ty : iv.ty;
      }
      return iv;
    }
    case STIGI_EXPR_VAR: {
      /* Fix #11: emit belowOwed/leqOwed (assertion-unproved) BEFORE crediting var x@<..> */
      const char *ty = e->u.var_stmt.type != NULL
                           ? fs_strdup(ctx->fs, e->u.var_stmt.type->name.ptr,
                                       e->u.var_stmt.type->name.len)
                           : NULL;
      ExtVal iv = ext_expr(ctx, e->u.var_stmt.init, ty);
      const char *pt = ext_new_pt(ctx);
      const char *op = ext_new_op(ctx);
      (void)emit4(ctx->fs, "operation", col_sym(op), col_sym("Var"), col_sym(pt),
                  col_sym(ctx->cur_sc), ctx->diag);
      (void)emit2(ctx->fs, "operationOf", col_sym(op), col_sym(ctx->inst_sym), ctx->diag);
      (void)emit2(ctx->fs, "emitted", col_sym(op), col_sym("Var"), ctx->diag);
      if (ctx->var_count < 128) {
        ExtVar *v = &ctx->vars[ctx->var_count++];
        memset(v, 0, sizeof *v);
        v->name = e->u.var_stmt.name;
        v->val_sym = iv.sym;
        v->type_sym = ty != NULL ? ty : iv.ty;
        v->has_inv = e->u.var_stmt.has_inv;
        v->inv = e->u.var_stmt.inv;
      }
      if (e->u.var_stmt.has_inv) {
        if (fact_group_has_safety(&e->u.var_stmt.inv)) {
          (void)emit2(ctx->fs, "statesFact", col_sym(ctx->unit_sym), col_sym("Invariant"),
                      ctx->diag);
        }
        /* Check invariant in pre-scope, credit only in post-scope (no circular self-proof!) */
        ext_emit_owed_subst(ctx, op, &e->u.var_stmt.inv, ctx->cur_sc, "assertion-unproved",
                            iv.sym, NULL, NULL, 0);
        const char *post_sc = ext_new_sc(ctx, ctx->cur_sc);
        ext_emit_credits_subst(ctx, &e->u.var_stmt.inv, post_sc, iv.sym, NULL, NULL, 0);
        ctx->cur_sc = post_sc;
      }
      return iv;
    }
    case STIGI_EXPR_ASSIGN: {
      /* Fix #11: allocate a FRESH SSA val_sym on every ASSIGN and check var invariant */
      ExtVar *ev = ext_find_var(ctx, e->u.assign_stmt.name);
      ExtVal vv = ext_expr(ctx, e->u.assign_stmt.value, ev != NULL ? ev->type_sym : NULL);
      const char *pt = ext_new_pt(ctx);
      const char *op = ext_new_op(ctx);
      (void)emit4(ctx->fs, "operation", col_sym(op), col_sym("Assign"), col_sym(pt),
                  col_sym(ctx->cur_sc), ctx->diag);
      (void)emit2(ctx->fs, "operationOf", col_sym(op), col_sym(ctx->inst_sym), ctx->diag);
      (void)emit2(ctx->fs, "emitted", col_sym(op), col_sym("Assign"), ctx->diag);
      if (ev != NULL) {
        ExtVal fresh = ext_new_val(ctx, ev->type_sym != NULL ? ev->type_sym : vv.ty);
        (void)emit3(ctx->fs, "baseLeq", col_sym(vv.sym), col_sym(fresh.sym),
                    col_sym(ctx->cur_sc), ctx->diag);
        (void)emit3(ctx->fs, "baseLeq", col_sym(fresh.sym), col_sym(vv.sym),
                    col_sym(ctx->cur_sc), ctx->diag);
        ev->val_sym = fresh.sym;
        if (ev->has_inv) {
          ext_emit_owed_subst(ctx, op, &ev->inv, ctx->cur_sc, "assertion-unproved",
                              fresh.sym, NULL, NULL, 0);
          const char *post_sc = ext_new_sc(ctx, ctx->cur_sc);
          ext_emit_credits_subst(ctx, &ev->inv, post_sc, fresh.sym, NULL, NULL, 0);
          ctx->cur_sc = post_sc;
        }
        return fresh;
      }
      return vv;
    }
    case STIGI_EXPR_INDEX_READ: {
      ExtVal iv = ext_expr(ctx, e->u.index_expr.index, "U64");
      const char *pt = ext_new_pt(ctx);
      const char *op = ext_new_op(ctx);
      ExtVal rv = ext_new_val(ctx, hint_ty != NULL ? hint_ty : "U64");
      const char *len_sym = ext_new_val(ctx, "U64").sym;
      if (e->u.index_expr.target->kind == STIGI_EXPR_LOCAL) {
        ExtVar *ev = ext_find_var(ctx, e->u.index_expr.target->u.ident.name);
        if (ev != NULL && ev->len_sym != NULL) len_sym = ev->len_sym;
      }
      (void)emit4(ctx->fs, "operation", col_sym(op), col_sym("IndexRead"), col_sym(pt),
                  col_sym(ctx->cur_sc), ctx->diag);
      (void)emit2(ctx->fs, "operationOf", col_sym(op), col_sym(ctx->inst_sym), ctx->diag);
      (void)emit4(ctx->fs, "indexRead", col_sym(op), col_sym(iv.sym), col_sym(len_sym),
                  col_sym(ctx->cur_sc), ctx->diag);
      (void)emit2(ctx->fs, "result", col_sym(op), col_sym(rv.sym), ctx->diag);
      (void)emit2(ctx->fs, "emitted", col_sym(op), col_sym("IndexRead"), ctx->diag);
      (void)emit2(ctx->fs, "emitted", col_sym(op), col_sym("Result"), ctx->diag);
      return rv;
    }
    case STIGI_EXPR_INDEX_WRITE: {
      ExtVal iv = ext_expr(ctx, e->u.index_expr.index, "U64");
      (void)ext_expr(ctx, e->u.index_expr.value, NULL);
      const char *pt = ext_new_pt(ctx);
      const char *op = ext_new_op(ctx);
      const char *obj_sym = ext_new_val(ctx, "Array").sym;
      const char *len_sym = ext_new_val(ctx, "U64").sym;
      if (e->u.index_expr.target->kind == STIGI_EXPR_LOCAL) {
        ExtVar *ev = ext_find_var(ctx, e->u.index_expr.target->u.ident.name);
        if (ev != NULL) {
          obj_sym = ev->val_sym;
          if (ev->len_sym != NULL) len_sym = ev->len_sym;
        }
      }
      (void)emit4(ctx->fs, "operation", col_sym(op), col_sym("IndexWrite"), col_sym(pt),
                  col_sym(ctx->cur_sc), ctx->diag);
      (void)emit2(ctx->fs, "operationOf", col_sym(op), col_sym(ctx->inst_sym), ctx->diag);
      (void)emit4(ctx->fs, "indexWrite", col_sym(op), col_sym(iv.sym), col_sym(len_sym),
                  col_sym(ctx->cur_sc), ctx->diag);
      (void)emit2(ctx->fs, "overwrite", col_sym(pt), col_sym(obj_sym), ctx->diag);
      (void)emit2(ctx->fs, "writeVia", col_sym(op), col_sym(obj_sym), ctx->diag);
      (void)emit2(ctx->fs, "emitted", col_sym(op), col_sym("IndexWrite"), ctx->diag);
      (void)emit2(ctx->fs, "emitted", col_sym(op), col_sym("Overwrite"), ctx->diag);
      (void)emit2(ctx->fs, "emitted", col_sym(op), col_sym("WriteVia"), ctx->diag);
      return ext_new_val(ctx, "Unit");
    }
    case STIGI_EXPR_HANDLE: {
      const char *pt = ext_new_pt(ctx);
      const char *op = ext_new_op(ctx);
      (void)emit4(ctx->fs, "operation", col_sym(op), col_sym("Handle"), col_sym(pt),
                  col_sym(ctx->cur_sc), ctx->diag);
      (void)emit2(ctx->fs, "operationOf", col_sym(op), col_sym(ctx->inst_sym), ctx->diag);
      (void)emit2(ctx->fs, "emitted", col_sym(op), col_sym("Handle"), ctx->diag);
      if (e->u.handle_stmt.body != NULL) {
        return ext_expr(ctx, e->u.handle_stmt.body, hint_ty);
      }
      return ext_new_val(ctx, "Unit");
    }
    case STIGI_EXPR_ASSERT: {
      ExtVal rv = e->u.assert_expr.expr != NULL ? ext_expr(ctx, e->u.assert_expr.expr, hint_ty)
                                                : ext_new_val(ctx, "Unit");
      const char *pt = ext_new_pt(ctx);
      const char *op = ext_new_op(ctx);
      (void)emit4(ctx->fs, "operation", col_sym(op), col_sym("Assert"), col_sym(pt),
                  col_sym(ctx->cur_sc), ctx->diag);
      (void)emit2(ctx->fs, "operationOf", col_sym(op), col_sym(ctx->inst_sym), ctx->diag);
      (void)emit2(ctx->fs, "emitted", col_sym(op), col_sym("Assert"), ctx->diag);
      if (fact_group_has_safety(&e->u.assert_expr.facts)) {
        (void)emit2(ctx->fs, "statesFact", col_sym(ctx->unit_sym), col_sym("Assertion"), ctx->diag);
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
      (void)emit4(ctx->fs, "operation", col_sym(op), col_sym("Deny"), col_sym(pt),
                  col_sym(ctx->cur_sc), ctx->diag);
      (void)emit2(ctx->fs, "operationOf", col_sym(op), col_sym(ctx->inst_sym), ctx->diag);
      (void)emit2(ctx->fs, "emitted", col_sym(op), col_sym("Deny"), ctx->diag);
      return ext_new_val(ctx, "Unit");
    }
    case STIGI_EXPR_BLOCK: {
      ExtVal last = ext_new_val(ctx, "Unit");
      for (uint32_t i = 0; i < e->u.block.count; i++) {
        const char *h = (i + 1 == e->u.block.count) ? hint_ty : NULL;
        last = ext_expr(ctx, e->u.block.stmts[i], h);
      }
      return last;
    }
    case STIGI_EXPR_LOCAL_DECL:
      return ext_new_val(ctx, "Unit");
  }
  return ext_new_val(ctx, "Unit");
}

static const char *decl_kind_sym(StigiDeclKind k) {
  switch (k) {
    case STIGI_DECL_ENUM: return "Enum";
    case STIGI_DECL_TYPE: return "Type";
    case STIGI_DECL_DEF: return "Def";
    case STIGI_DECL_LEMMA_DEF: return "Def";
    case STIGI_DECL_VAL: return "Val";
    case STIGI_DECL_CAPABILITY: return "Capability";
    case STIGI_DECL_HANDLER: return "Handler";
    case STIGI_DECL_EXTERN: return "Extern";
    case STIGI_DECL_RELATION: return "Relation";
    case STIGI_DECL_RULE: return "Rule";
    case STIGI_DECL_TEST: return "Test";
    case STIGI_DECL_NEG_TEST: return "NegTest";
  }
  return "Def";
}

static Status extract_one_unit_inner(const StigiImage *img, uint32_t unit_idx,
                                     StigiFactSet *fs, Diag *diag) {
  const StigiImageUnit *u = &img->units[unit_idx];
  const StigiDecl *d = u->ast->decl;
  const char *u_sym = fs_strdup(fs, u->name, u->name_len);
  if (u_sym == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");

  Status st = emit2(fs, "unit", col_sym(u_sym), col_sym(decl_kind_sym(d->kind)), diag);
  if (st != STIGI_OK) return st;
  st = emit2(fs, "unitName", col_sym(u_sym), col_sym(u_sym), diag);
  if (st != STIGI_OK) return st;
  if (u->is_root || u->reachable) {
    st = emit1(fs, "root", col_sym(u_sym), diag);
    if (st != STIGI_OK) return st;
  }

  if (d->kind == STIGI_DECL_EXTERN && !d->u.extern_decl.is_val &&
      d->u.extern_decl.body == NULL) {
    const char *inst_sym = fs_fmt(fs, "%s#0", u_sym);
    st = emit2(fs, "instanceOf", col_sym(inst_sym), col_sym(u_sym), diag);
    if (st != STIGI_OK) return st;
    return emit1(fs, "bodylessExtern", col_sym(inst_sym), diag);
  }

  if (d->kind == STIGI_DECL_CAPABILITY) {
    for (uint32_t i = 0; i < d->u.cap_decl.state_count; i++) {
      const char *ss = fs_strdup(fs, d->u.cap_decl.states[i].ptr, d->u.cap_decl.states[i].len);
      st = emit2(fs, "stateOf", col_sym(u_sym), col_sym(ss), diag);
      if (st != STIGI_OK) return st;
    }
    if (d->u.cap_decl.start_state.len > 0) {
      const char *ss = fs_strdup(fs, d->u.cap_decl.start_state.ptr, d->u.cap_decl.start_state.len);
      st = emit2(fs, "startState", col_sym(u_sym), col_sym(ss), diag);
      if (st != STIGI_OK) return st;
    }
    for (uint32_t i = 0; i < d->u.cap_decl.stop_state_count; i++) {
      const char *ss = fs_strdup(fs, d->u.cap_decl.stop_states[i].ptr,
                                 d->u.cap_decl.stop_states[i].len);
      st = emit2(fs, "stopState", col_sym(u_sym), col_sym(ss), diag);
      if (st != STIGI_OK) return st;
    }
    for (uint32_t i = 0; i < d->u.cap_decl.member_count; i++) {
      const StigiDecl *m = d->u.cap_decl.members[i];
      if (m->kind == STIGI_DECL_DEF) {
        const char *op_s = fs_fmt(fs, "%s.%.*s", u_sym, (int)m->name.len, m->name.ptr);
        st = emit2(fs, "capabilityOperation", col_sym(u_sym), col_sym(op_s), diag);
        if (st != STIGI_OK) return st;
      }
    }
    return STIGI_OK;
  }

  if (d->kind != STIGI_DECL_DEF && d->kind != STIGI_DECL_LEMMA_DEF) {
    return STIGI_OK;
  }

  const char *inst_sym = fs_fmt(fs, "%s#0", u_sym);
  st = emit2(fs, "instanceOf", col_sym(inst_sym), col_sym(u_sym), diag);
  if (st != STIGI_OK) return st;
  st = emit1(fs, "defHasBody", col_sym(u_sym), diag);
  if (st != STIGI_OK) return st;
  if (d->kind == STIGI_DECL_LEMMA_DEF) {
    st = emit1(fs, "lemmaUnit", col_sym(u_sym), diag);
    if (st != STIGI_OK) return st;
    if (d->u.def_decl.body != NULL) {
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

  const char *s0 = ext_new_sc(&ctx, NULL);
  ctx.cur_sc = s0;
  const char *p0 = ext_new_pt(&ctx);
  (void)emit1(fs, "entry", col_sym(p0), diag);

  const StigiDefHead *h = &d->u.def_decl.head;
  for (uint32_t i = 0; i < h->param_count && ctx.var_count < 128; i++) {
    const StigiParam *pr = &h->params[i];
    const char *ty_s = fs_strdup(fs, pr->type->name.ptr, pr->type->name.len);
    const char *vs = fs_fmt(fs, "%s:p:%.*s", inst_sym, (int)pr->name.len, pr->name.ptr);
    (void)emit1(fs, "value", col_sym(vs), diag);
    (void)emit2(fs, "valueType", col_sym(vs), col_sym(ty_s), diag);
    if (strcmp(ty_s, "U64") == 0) (void)emit1(fs, "u64", col_sym(vs), diag);
    (void)emit3(fs, "param", col_sym(inst_sym), col_num((int64_t)i), col_sym(vs), diag);
    ExtVar *v = &ctx.vars[ctx.var_count++];
    memset(v, 0, sizeof *v);
    v->name = pr->name;
    v->val_sym = vs;
    v->type_sym = ty_s;
  }
  for (uint32_t i = 0; i < h->param_count && i < ctx.var_count; i++) {
    const StigiType *pty = h->params[i].type;
    if (pty->kind == STIGI_TYPE_ARRAY && pty->len_arg_count > 0) {
      ctx.vars[i].len_sym = ext_resolve_fact_val(&ctx, pty->len_args[0], NULL, NULL, NULL, 0);
    }
  }

  for (uint32_t i = 0; i < h->pre_group_count; i++) {
    if (h->pre_groups[i].is_fact) {
      if (fact_group_has_safety(&h->pre_groups[i].facts)) {
        (void)emit2(fs, "statesFact", col_sym(u_sym), col_sym("Entry"), diag);
      }
      ext_emit_credits_subst(&ctx, &h->pre_groups[i].facts, s0, NULL, NULL, NULL, 0);
    } else {
      for (uint32_t c = 0; c < h->pre_groups[i].cap_count; c++) {
        StigiStr cn = h->pre_groups[i].caps[c].cap_type->name;
        const char *cs = fs_strdup(fs, cn.ptr, cn.len);
        (void)emit2(fs, "rowOf", col_sym(u_sym), col_sym(cs), diag);
        if (ctx.cap_count < 32) {
          const char *prov = fs_fmt(fs, "%s:cap%u", inst_sym, ctx.cap_count);
          ctx.cap_units[ctx.cap_count] = cs;
          ctx.cap_provs[ctx.cap_count] = prov;
          (void)emit2(fs, "capabilityOf", col_sym(prov), col_sym(cs), diag);
          ctx.cap_count++;
        }
      }
    }
  }
  for (uint32_t i = 0; i < h->post_val_fact_count; i++) {
    if (fact_group_has_safety(&h->post_val_facts[i])) {
      (void)emit2(fs, "statesFact", col_sym(u_sym), col_sym("Entry"), diag);
    }
    ext_emit_credits_subst(&ctx, &h->post_val_facts[i], s0, NULL, NULL, NULL, 0);
  }
  if (h->decreases != NULL) {
    const char *site = fs_fmt(fs, "%s:def", inst_sym);
    const char *mv = ext_resolve_fact_val(&ctx, h->decreases, NULL, NULL, NULL, 0);
    (void)emit2(fs, "decreasesAt", col_sym(site), col_sym(mv), diag);
    (void)emit2(fs, "defSite", col_sym(site), col_sym(inst_sym), diag);
  }

  const char *ret_hint = h->ret_type != NULL
                             ? fs_strdup(fs, h->ret_type->name.ptr, h->ret_type->name.len)
                             : NULL;
  ExtVal ret_v = ext_expr(&ctx, d->u.def_decl.body, ret_hint);

  for (uint32_t i = 0; i < h->exit_fact_count; i++) {
    if (fact_group_has_safety(&h->exit_facts[i])) {
      (void)emit2(fs, "statesFact", col_sym(u_sym), col_sym("Exit"), diag);
    }
    const char *ex_op = ext_new_op(&ctx);
    ext_emit_owed_subst(&ctx, ex_op, &h->exit_facts[i], ctx.cur_sc, "exit-unproved",
                        ret_v.sym, NULL, NULL, 0);
  }

  (void)emit2(fs, "returnOf", col_sym(ctx.cur_pt), col_sym(ret_v.sym), diag);
  (void)emit1(fs, "exit", col_sym(ctx.cur_pt), diag);
  return STIGI_OK;
}

Status stigi_extract_unit(const StigiImage *img, uint32_t unit_idx,
                          StigiFactSet *fs, Diag *diag) {
  if (img == NULL || fs == NULL || unit_idx >= img->unit_count) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_extract_unit: invalid arguments");
  }
  Status st = emit_kernel_tables(fs, diag);
  if (st != STIGI_OK) return st;
  return extract_one_unit_inner(img, unit_idx, fs, diag);
}

Status stigi_extract_image(const StigiImage *img, StigiFactSet *fs, Diag *diag) {
  if (img == NULL || fs == NULL) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_extract_image: invalid arguments");
  }
  Status st = emit_kernel_tables(fs, diag);
  if (st != STIGI_OK) return st;
  for (uint32_t i = 0; i < img->unit_count; i++) {
    st = extract_one_unit_inner(img, i, fs, diag);
    if (st != STIGI_OK) return st;
  }
  /* Fix #8: emit dependency edges */
  for (uint32_t e = 0; e < img->edge_count; e++) {
    uint32_t f = img->edges[e].from;
    uint32_t t = img->edges[e].to;
    if (f < img->unit_count && t < img->unit_count) {
      const char *fsym = fs_strdup(fs, img->units[f].name, img->units[f].name_len);
      const char *tsym = fs_strdup(fs, img->units[t].name, img->units[t].name_len);
      st = emit2(fs, "edge", col_sym(fsym), col_sym(tsym), diag);
      if (st != STIGI_OK) return st;
    }
  }
  return STIGI_OK;
}

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
    int64_t vals[STIGI_MAX_FACT_COLS];
    for (int c = 0; c < r->ncols; c++) {
      if (r->cols[c].is_num) {
        vals[c] = r->cols[c].num;
      } else {
        uint32_t sym_id = 0;
        Status st = stigi_dl_intern(dl, r->cols[c].sym, strlen(r->cols[c].sym), &sym_id, diag);
        if (st != STIGI_OK) return st;
        vals[c] = (int64_t)sym_id;
      }
    }
    Status st = stigi_dl_add(dl, rel_id, vals, r->ncols, diag);
    if (st != STIGI_OK) return st;
  }
  return STIGI_OK;
}

static int cmp_fact_row(const void *pa, const void *pb) {
  const StigiFactRow *a = *(const StigiFactRow *const *)pa;
  const StigiFactRow *b = *(const StigiFactRow *const *)pb;
  int c = strcmp(a->rel, b->rel);
  if (c != 0) return c;
  if (a->ncols != b->ncols) return a->ncols < b->ncols ? -1 : 1;
  for (int i = 0; i < a->ncols; i++) {
    if (a->cols[i].is_num != b->cols[i].is_num) return a->cols[i].is_num ? -1 : 1;
    if (a->cols[i].is_num) {
      if (a->cols[i].num != b->cols[i].num) return a->cols[i].num < b->cols[i].num ? -1 : 1;
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
    while (ncap < sb->len + n + 1U) ncap *= 2U;
    char *nb = realloc(sb->buf, ncap);
    if (nb == NULL) { sb->oom = true; return; }
    sb->buf = nb;
    sb->cap = ncap;
  }
  memcpy(sb->buf + sb->len, s, n);
  sb->len += n;
  sb->buf[sb->len] = '\0';
}

/* Fix #16: dynamic buffer sizing in stigi_facts_print (zero fixed-buffer over-read) */
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
        char nbuf[32];
        int nn = snprintf(nbuf, sizeof nbuf, "%lld", (long long)r->cols[c].num);
        if (nn > 0) sb_append(&sb, nbuf, (size_t)nn);
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

/* Fix #9: batch FILE* open/close per relation (sort rows by rel, open each file ONCE) */
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
          (void)snprintf(path, sizeof path, "%s/%.*s.facts", dir, (int)rlen, start);
          FILE *f = fopen(path, "w");
          if (f == NULL) {
            return diag_set(diag, STIGI_ERR_IO, -1, "cannot create %s", path);
          }
          (void)fclose(f);
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
      if (cur_f != NULL) { (void)fclose(cur_f); cur_f = NULL; }
      char path[512];
      (void)snprintf(path, sizeof path, "%s/%s.facts", dir, r->rel);
      cur_f = fopen(path, "w");
      if (cur_f == NULL) {
        st = diag_set(diag, STIGI_ERR_IO, -1, "cannot open %s", path);
        break;
      }
      cur_rel = r->rel;
    }
    for (int c = 0; c < r->ncols; c++) {
      if (c > 0) fputc('\t', cur_f);
      if (r->cols[c].is_num) fprintf(cur_f, "%lld", (long long)r->cols[c].num);
      else fputs(r->cols[c].sym, cur_f);
    }
    fputc('\n', cur_f);
  }
  if (cur_f != NULL) (void)fclose(cur_f);
  free(sorted);
  return st;
}
