#include "eval.h"

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  StigiStr name;
  StigiVal val;
} EvalBinding;

typedef struct {
  const StigiImage *img;
  Arena *arena;
  uint64_t fuel_left;
  uint64_t fuel_max;
  uint32_t call_depth;
  StigiEvalResult res;
  bool failed;
} EvalCtx;

static void eval_fail(EvalCtx *ctx, const char *code, const char *fmt, ...) {
  if (ctx->failed) return;
  ctx->failed = true;
  ctx->res.status = STIGI_ERR_INVALID;
  ctx->res.diag_code = code;
  va_list ap;
  va_start(ap, fmt);
  (void)vsnprintf(ctx->res.msg, sizeof ctx->res.msg, fmt, ap);
  va_end(ap);
}

static bool burn_fuel(EvalCtx *ctx) {
  if (ctx->fuel_left == 0) {
    eval_fail(ctx, "evaluation-fuel", "evaluation exceeded fuel budget (%llu)",
              (unsigned long long)ctx->fuel_max);
    return false;
  }
  ctx->fuel_left--;
  return true;
}

static bool str_eq_lit(StigiStr s, const char *lit) {
  size_t n = strlen(lit);
  return s.len == n && memcmp(s.ptr, lit, n) == 0;
}

static bool str_eq_s(StigiStr a, StigiStr b) {
  return a.len == b.len && (a.len == 0 || memcmp(a.ptr, b.ptr, a.len) == 0);
}

static bool val_equal(const StigiVal *a, const StigiVal *b) {
  if (a->kind != b->kind) return false;
  switch (a->kind) {
    case STIGI_VAL_UNIT:
    case STIGI_VAL_TEST_OK:
      return true;
    case STIGI_VAL_BOOL:
      return a->u.b == b->u.b;
    case STIGI_VAL_INT:
      if (a->is_unsigned && b->is_unsigned) return a->u.u == b->u.u;
      if (!a->is_unsigned && !b->is_unsigned) return a->u.i == b->u.i;
      if (a->is_unsigned) return b->u.i >= 0 && a->u.u == (uint64_t)b->u.i;
      return a->u.i >= 0 && (uint64_t)a->u.i == b->u.u;
    case STIGI_VAL_STR:
      return str_eq_s(a->u.s, b->u.s);
    case STIGI_VAL_ENUM:
      if (!str_eq_s(a->u.en.case_name, b->u.en.case_name) ||
          a->u.en.field_count != b->u.en.field_count) {
        return false;
      }
      for (uint32_t i = 0; i < a->u.en.field_count; i++) {
        if (!val_equal(&a->u.en.fields[i], &b->u.en.fields[i])) return false;
      }
      return true;
    case STIGI_VAL_ARRAY:
      if (a->u.arr.len != b->u.arr.len) return false;
      for (uint32_t i = 0; i < a->u.arr.len; i++) {
        if (!val_equal(&a->u.arr.elems[i], &b->u.arr.elems[i])) return false;
      }
      return true;
  }
  return false;
}

static EvalBinding *find_env(EvalBinding *env, uint32_t env_count, StigiStr name) {
  for (uint32_t i = env_count; i > 0; i--) {
    if (str_eq_s(env[i - 1].name, name)) return &env[i - 1];
  }
  return NULL;
}

static StigiVal eval_expr(EvalCtx *ctx, uint32_t uidx, const StigiExpr *e,
                          EvalBinding *env, uint32_t *env_count, uint32_t env_cap);

static StigiVal eval_call_unit(EvalCtx *ctx, uint32_t tgt_idx,
                               const StigiVal *args, uint32_t arg_count) {
  StigiVal z;
  memset(&z, 0, sizeof z);
  if (ctx->failed || !burn_fuel(ctx)) return z;
  if (ctx->call_depth >= 128) {
    eval_fail(ctx, "evaluation-fuel", "maximum call depth exceeded");
    return z;
  }
  const StigiDecl *d = ctx->img->units[tgt_idx].ast->decl;
  if (d->kind == STIGI_DECL_EXTERN && d->u.extern_decl.body == NULL) {
    eval_fail(ctx, "not-compile-time", "cannot evaluate body-less extern '%s' at compile time",
              ctx->img->units[tgt_idx].name);
    return z;
  }
  if (d->kind != STIGI_DECL_DEF && d->kind != STIGI_DECL_LEMMA_DEF &&
      d->kind != STIGI_DECL_EXTERN) {
    eval_fail(ctx, "eval-error", "unit '%s' is not callable", ctx->img->units[tgt_idx].name);
    return z;
  }
  const StigiDefHead *h = (d->kind == STIGI_DECL_EXTERN) ? &d->u.extern_decl.head
                                                         : &d->u.def_decl.head;
  const StigiExpr *body = (d->kind == STIGI_DECL_EXTERN) ? d->u.extern_decl.body
                                                         : d->u.def_decl.body;
  if (arg_count != h->param_count) {
    eval_fail(ctx, "eval-error", "arity mismatch calling '%s'", ctx->img->units[tgt_idx].name);
    return z;
  }
  EvalBinding frame[128];
  uint32_t fcnt = 0;
  for (uint32_t i = 0; i < h->param_count && fcnt < 128; i++) {
    frame[fcnt].name = h->params[i].name;
    frame[fcnt].val = args[i];
    fcnt++;
  }
  ctx->call_depth++;
  StigiVal out = eval_expr(ctx, tgt_idx, body, frame, &fcnt, 128);
  ctx->call_depth--;
  return out;
}

static bool checked_add_i64(int64_t a, int64_t b, int64_t *out) {
  if ((b > 0 && a > INT64_MAX - b) || (b < 0 && a < INT64_MIN - b)) return false;
  *out = a + b;
  return true;
}

static bool checked_sub_i64(int64_t a, int64_t b, int64_t *out) {
  if ((b < 0 && a > INT64_MAX + b) || (b > 0 && a < INT64_MIN + b)) return false;
  *out = a - b;
  return true;
}

static bool checked_mul_i64(int64_t a, int64_t b, int64_t *out) {
  if (a == 0 || b == 0) { *out = 0; return true; }
  if (a == -1 && b == INT64_MIN) return false;
  if (b == -1 && a == INT64_MIN) return false;
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

static StigiVal eval_expr(EvalCtx *ctx, uint32_t uidx, const StigiExpr *e,
                          EvalBinding *env, uint32_t *env_count, uint32_t env_cap) {
  StigiVal z;
  memset(&z, 0, sizeof z);
  if (ctx->failed || e == NULL) return z;

  switch (e->kind) {
    case STIGI_EXPR_LIT_UNIT:
      z.kind = STIGI_VAL_UNIT;
      return z;
    case STIGI_EXPR_LIT_BOOL:
      z.kind = STIGI_VAL_BOOL;
      z.u.b = e->u.lit_bool.value;
      return z;
    case STIGI_EXPR_LIT_INT:
      z.kind = STIGI_VAL_INT;
      z.is_unsigned = true;
      z.u.u = e->u.lit_int.value;
      return z;
    case STIGI_EXPR_LIT_CHAR:
      z.kind = STIGI_VAL_INT;
      z.is_unsigned = true;
      z.u.u = (uint64_t)e->u.lit_char.value;
      return z;
    case STIGI_EXPR_LIT_STR:
      z.kind = STIGI_VAL_STR;
      z.u.s = e->u.lit_str.raw;
      return z;
    case STIGI_EXPR_LOCAL:
    case STIGI_EXPR_GLOBAL: {
      EvalBinding *b = find_env(env, *env_count, e->u.ident.name);
      if (b != NULL) return b->val;
      int32_t tgt = stigi_image_resolve_name(ctx->img, uidx,
                                             e->u.ident.name.ptr, e->u.ident.name.len);
      if (tgt >= 0) {
        const StigiDecl *d = ctx->img->units[tgt].ast->decl;
        if (d->kind == STIGI_DECL_VAL) {
          if (!burn_fuel(ctx)) return z;
          if (ctx->call_depth >= 128) {
            eval_fail(ctx, "evaluation-fuel", "maximum val evaluation depth exceeded");
            return z;
          }
          EvalBinding sub[64];
          uint32_t scnt = 0;
          ctx->call_depth++;
          StigiVal rv = eval_expr(ctx, (uint32_t)tgt, d->u.val_decl.body, sub, &scnt, 64);
          ctx->call_depth--;
          return rv;
        }
        if (d->kind == STIGI_DECL_EXTERN && d->u.extern_decl.is_val) {
          eval_fail(ctx, "not-compile-time", "cannot read extern val '%s' at compile time",
                    ctx->img->units[tgt].name);
          return z;
        }
      }
      /* Nullary enum constructor */
      if (e->u.ident.name.len > 0 && e->u.ident.name.ptr[0] >= 'A' &&
          e->u.ident.name.ptr[0] <= 'Z') {
        z.kind = STIGI_VAL_ENUM;
        z.u.en.case_name = e->u.ident.name;
        return z;
      }
      eval_fail(ctx, "eval-error", "unbound identifier '%.*s'",
                (int)e->u.ident.name.len, e->u.ident.name.ptr);
      return z;
    }
    case STIGI_EXPR_UNOP: {
      StigiVal v = eval_expr(ctx, uidx, e->u.unop.operand, env, env_count, env_cap);
      if (ctx->failed) return z;
      if (e->u.unop.op == STIGI_OP_NOT && v.kind == STIGI_VAL_BOOL) {
        v.u.b = !v.u.b;
        return v;
      }
      if (e->u.unop.op == STIGI_OP_NEG && v.kind == STIGI_VAL_INT) {
        if (v.is_unsigned) {
          if (v.u.u > 9223372036854775808ULL) {
            eval_fail(ctx, "eval-error", "integer negation overflow");
            return z;
          }
          v.is_unsigned = false;
          v.u.i = (v.u.u == 9223372036854775808ULL) ? INT64_MIN : -(int64_t)v.u.u;
          return v;
        }
        if (v.u.i == INT64_MIN) {
          eval_fail(ctx, "eval-error", "integer negation overflow");
          return z;
        }
        v.u.i = -v.u.i;
        return v;
      }
      eval_fail(ctx, "eval-error", "invalid unary operand");
      return z;
    }
    case STIGI_EXPR_BINOP: {
      StigiOp op = e->u.binop.op;
      if (op == STIGI_OP_AND) {
        StigiVal lv = eval_expr(ctx, uidx, e->u.binop.lhs, env, env_count, env_cap);
        if (ctx->failed) return z;
        if (!lv.u.b) return lv;
        return eval_expr(ctx, uidx, e->u.binop.rhs, env, env_count, env_cap);
      }
      if (op == STIGI_OP_OR) {
        StigiVal lv = eval_expr(ctx, uidx, e->u.binop.lhs, env, env_count, env_cap);
        if (ctx->failed) return z;
        if (lv.u.b) return lv;
        return eval_expr(ctx, uidx, e->u.binop.rhs, env, env_count, env_cap);
      }
      StigiVal lv = eval_expr(ctx, uidx, e->u.binop.lhs, env, env_count, env_cap);
      if (ctx->failed) return z;
      StigiVal rv = eval_expr(ctx, uidx, e->u.binop.rhs, env, env_count, env_cap);
      if (ctx->failed) return z;
      if (op == STIGI_OP_EQ || op == STIGI_OP_NE) {
        z.kind = STIGI_VAL_BOOL;
        z.u.b = (op == STIGI_OP_EQ) ? val_equal(&lv, &rv) : !val_equal(&lv, &rv);
        return z;
      }
      if (lv.kind != STIGI_VAL_INT || rv.kind != STIGI_VAL_INT) {
        eval_fail(ctx, "eval-error", "non-integer arithmetic operand");
        return z;
      }
      if (lv.is_unsigned && rv.is_unsigned) {
        uint64_t a = lv.u.u, b = rv.u.u;
        switch (op) {
          case STIGI_OP_ADD:
            if (a > UINT64_MAX - b) { eval_fail(ctx, "eval-error", "u64 add overflow"); return z; }
            z.kind = STIGI_VAL_INT; z.is_unsigned = true; z.u.u = a + b; return z;
          case STIGI_OP_SUB:
            if (a < b) { eval_fail(ctx, "eval-error", "u64 sub underflow"); return z; }
            z.kind = STIGI_VAL_INT; z.is_unsigned = true; z.u.u = a - b; return z;
          case STIGI_OP_MUL:
            if (b != 0 && a > UINT64_MAX / b) {
              eval_fail(ctx, "eval-error", "u64 mul overflow");
              return z;
            }
            z.kind = STIGI_VAL_INT; z.is_unsigned = true; z.u.u = a * b; return z;
          case STIGI_OP_DIV:
            if (b == 0) { eval_fail(ctx, "eval-error", "division by zero"); return z; }
            z.kind = STIGI_VAL_INT; z.is_unsigned = true; z.u.u = a / b; return z;
          case STIGI_OP_MOD:
            if (b == 0) { eval_fail(ctx, "eval-error", "remainder by zero"); return z; }
            z.kind = STIGI_VAL_INT; z.is_unsigned = true; z.u.u = a % b; return z;
          case STIGI_OP_LT: z.kind = STIGI_VAL_BOOL; z.u.b = (a < b); return z;
          case STIGI_OP_LE: z.kind = STIGI_VAL_BOOL; z.u.b = (a <= b); return z;
          case STIGI_OP_GT: z.kind = STIGI_VAL_BOOL; z.u.b = (a > b); return z;
          case STIGI_OP_GE: z.kind = STIGI_VAL_BOOL; z.u.b = (a >= b); return z;
          default:
            eval_fail(ctx, "eval-error", "unsupported operator");
            return z;
        }
      }
      int64_t a = lv.is_unsigned ? (int64_t)lv.u.u : lv.u.i;
      int64_t b = rv.is_unsigned ? (int64_t)rv.u.u : rv.u.i;
      int64_t r = 0;
      switch (op) {
        case STIGI_OP_ADD:
          if (!checked_add_i64(a, b, &r)) { eval_fail(ctx, "eval-error", "add overflow"); return z; }
          z.kind = STIGI_VAL_INT; z.u.i = r; return z;
        case STIGI_OP_SUB:
          if (!checked_sub_i64(a, b, &r)) { eval_fail(ctx, "eval-error", "sub overflow"); return z; }
          z.kind = STIGI_VAL_INT; z.u.i = r; return z;
        case STIGI_OP_MUL:
          if (!checked_mul_i64(a, b, &r)) { eval_fail(ctx, "eval-error", "mul overflow"); return z; }
          z.kind = STIGI_VAL_INT; z.u.i = r; return z;
        case STIGI_OP_DIV:
          if (b == 0 || (a == INT64_MIN && b == -1)) {
            eval_fail(ctx, "eval-error", "division error");
            return z;
          }
          z.kind = STIGI_VAL_INT; z.u.i = a / b; return z;
        case STIGI_OP_MOD:
          if (b == 0 || (a == INT64_MIN && b == -1)) {
            eval_fail(ctx, "eval-error", "remainder error");
            return z;
          }
          z.kind = STIGI_VAL_INT; z.u.i = a % b; return z;
        case STIGI_OP_LT: z.kind = STIGI_VAL_BOOL; z.u.b = (a < b); return z;
        case STIGI_OP_LE: z.kind = STIGI_VAL_BOOL; z.u.b = (a <= b); return z;
        case STIGI_OP_GT: z.kind = STIGI_VAL_BOOL; z.u.b = (a > b); return z;
        case STIGI_OP_GE: z.kind = STIGI_VAL_BOOL; z.u.b = (a >= b); return z;
        default:
          eval_fail(ctx, "eval-error", "unsupported operator");
          return z;
      }
    }
    case STIGI_EXPR_IF: {
      StigiVal cv = eval_expr(ctx, uidx, e->u.if_expr.cond, env, env_count, env_cap);
      if (ctx->failed) return z;
      if (cv.kind != STIGI_VAL_BOOL) {
        eval_fail(ctx, "eval-error", "'if' condition is not Bool");
        return z;
      }
      return eval_expr(ctx, uidx, cv.u.b ? e->u.if_expr.then_br : e->u.if_expr.else_br,
                       env, env_count, env_cap);
    }
    case STIGI_EXPR_WHILE: {
      for (;;) {
        StigiVal cv = eval_expr(ctx, uidx, e->u.while_expr.cond, env, env_count, env_cap);
        if (ctx->failed) return z;
        if (cv.kind != STIGI_VAL_BOOL) {
          eval_fail(ctx, "eval-error", "'while' condition is not Bool");
          return z;
        }
        if (!cv.u.b) break;
        if (!burn_fuel(ctx)) return z;
        (void)eval_expr(ctx, uidx, e->u.while_expr.body, env, env_count, env_cap);
        if (ctx->failed) return z;
      }
      z.kind = STIGI_VAL_UNIT;
      return z;
    }
    case STIGI_EXPR_VAL: {
      StigiVal iv = eval_expr(ctx, uidx, e->u.val_stmt.init, env, env_count, env_cap);
      if (ctx->failed) return z;
      if (e->u.val_stmt.pat.kind == STIGI_PAT_VAR && *env_count < env_cap) {
        env[*env_count].name = e->u.val_stmt.pat.name;
        env[*env_count].val = iv;
        (*env_count)++;
      } else if (e->u.val_stmt.pat.kind == STIGI_PAT_CTOR && iv.kind == STIGI_VAL_ENUM) {
        for (uint32_t i = 0; i < e->u.val_stmt.pat.field_count && i < iv.u.en.field_count; i++) {
          if (*env_count < env_cap) {
            env[*env_count].name = e->u.val_stmt.pat.fields[i];
            env[*env_count].val = iv.u.en.fields[i];
            (*env_count)++;
          }
        }
      }
      z.kind = STIGI_VAL_UNIT;
      return z;
    }
    case STIGI_EXPR_VAR: {
      StigiVal iv = eval_expr(ctx, uidx, e->u.var_stmt.init, env, env_count, env_cap);
      if (ctx->failed) return z;
      if (*env_count < env_cap) {
        env[*env_count].name = e->u.var_stmt.name;
        env[*env_count].val = iv;
        (*env_count)++;
      }
      z.kind = STIGI_VAL_UNIT;
      return z;
    }
    case STIGI_EXPR_ASSIGN: {
      StigiVal vv = eval_expr(ctx, uidx, e->u.assign_stmt.value, env, env_count, env_cap);
      if (ctx->failed) return z;
      EvalBinding *b = find_env(env, *env_count, e->u.assign_stmt.name);
      if (b == NULL) {
        eval_fail(ctx, "eval-error", "assignment to unknown var");
        return z;
      }
      b->val = vv;
      z.kind = STIGI_VAL_UNIT;
      return z;
    }
    case STIGI_EXPR_INDEX_WRITE: {
      if (e->u.index_expr.target->kind != STIGI_EXPR_LOCAL) {
        eval_fail(ctx, "eval-error", "index write target must be a local array");
        return z;
      }
      EvalBinding *b = find_env(env, *env_count, e->u.index_expr.target->u.ident.name);
      StigiVal idx = eval_expr(ctx, uidx, e->u.index_expr.index, env, env_count, env_cap);
      StigiVal val = eval_expr(ctx, uidx, e->u.index_expr.value, env, env_count, env_cap);
      if (ctx->failed) return z;
      uint64_t u_idx = idx.is_unsigned ? idx.u.u : (idx.u.i >= 0 ? (uint64_t)idx.u.i : UINT64_MAX);
      if (b == NULL || b->val.kind != STIGI_VAL_ARRAY || idx.kind != STIGI_VAL_INT ||
          u_idx >= b->val.u.arr.len) {
        eval_fail(ctx, "eval-error", "array index write out of bounds");
        return z;
      }
      b->val.u.arr.elems[u_idx] = val;
      z.kind = STIGI_VAL_UNIT;
      return z;
    }
    case STIGI_EXPR_MATCH: {
      StigiVal sv = eval_expr(ctx, uidx, e->u.match_expr.scrutinee, env, env_count, env_cap);
      if (ctx->failed) return z;
      for (uint32_t i = 0; i < e->u.match_expr.arm_count; i++) {
        const StigiPat *pat = &e->u.match_expr.arms[i].pat;
        uint32_t saved = *env_count;
        bool matched = false;
        if (pat->kind == STIGI_PAT_WILD) {
          matched = true;
        } else if (pat->kind == STIGI_PAT_INT && sv.kind == STIGI_VAL_INT &&
                   (sv.is_unsigned ? (sv.u.u == pat->int_val)
                                   : (sv.u.i >= 0 && (uint64_t)sv.u.i == pat->int_val))) {
          matched = true;
        } else if (pat->kind == STIGI_PAT_VAR) {
          matched = true;
          if (*env_count < env_cap) {
            env[*env_count].name = pat->name;
            env[*env_count].val = sv;
            (*env_count)++;
          }
        } else if (pat->kind == STIGI_PAT_CTOR && sv.kind == STIGI_VAL_ENUM &&
                   str_eq_s(pat->name, sv.u.en.case_name)) {
          matched = true;
          for (uint32_t f = 0; f < pat->field_count && f < sv.u.en.field_count; f++) {
            if (*env_count < env_cap) {
              env[*env_count].name = pat->fields[f];
              env[*env_count].val = sv.u.en.fields[f];
              (*env_count)++;
            }
          }
        }
        if (matched) {
          StigiVal r = eval_expr(ctx, uidx, e->u.match_expr.arms[i].body, env, env_count, env_cap);
          *env_count = saved;
          return r;
        }
        *env_count = saved;
      }
      eval_fail(ctx, "eval-error", "no match arm matched");
      return z;
    }
    case STIGI_EXPR_CALL: {
      StigiVal args[32];
      uint32_t argc = e->u.call.arg_count;
      if (argc > 32) {
        eval_fail(ctx, "eval-error", "too many call arguments");
        return z;
      }
      for (uint32_t i = 0; i < argc; i++) {
        args[i] = eval_expr(ctx, uidx, e->u.call.args[i], env, env_count, env_cap);
        if (ctx->failed) return z;
      }
      if (e->u.call.callee->kind == STIGI_EXPR_LOCAL ||
          e->u.call.callee->kind == STIGI_EXPR_GLOBAL) {
        StigiStr cn = e->u.call.callee->u.ident.name;
        /* Local array read */
        EvalBinding *lb = find_env(env, *env_count, cn);
        if (lb != NULL && lb->val.kind == STIGI_VAL_ARRAY && argc == 1 &&
            args[0].kind == STIGI_VAL_INT) {
          uint64_t u_idx = args[0].is_unsigned ? args[0].u.u
                                               : (args[0].u.i >= 0 ? (uint64_t)args[0].u.i : UINT64_MAX);
          if (u_idx >= lb->val.u.arr.len) {
            eval_fail(ctx, "eval-error", "array index read out of bounds");
            return z;
          }
          return lb->val.u.arr.elems[u_idx];
        }
        /* Stigi.Test.expect(got, want, reason) */
        if (str_eq_lit(cn, "Stigi.Test.expect")) {
          if (argc != 3) {
            eval_fail(ctx, "eval-error", "Stigi.Test.expect requires 3 arguments");
            return z;
          }
          if (!val_equal(&args[0], &args[1])) {
            eval_fail(ctx, "test-failed", "Stigi.Test.expect failed: %.*s",
                      (int)args[2].u.s.len, args[2].u.s.ptr != NULL ? args[2].u.s.ptr : "");
            return z;
          }
          z.kind = STIGI_VAL_TEST_OK;
          return z;
        }
        int32_t tgt = stigi_image_resolve_name(ctx->img, uidx, cn.ptr, cn.len);
        if (tgt >= 0) {
          return eval_call_unit(ctx, (uint32_t)tgt, args, argc);
        }
        /* Enum constructor */
        if (cn.len > 0 && cn.ptr[0] >= 'A' && cn.ptr[0] <= 'Z') {
          z.kind = STIGI_VAL_ENUM;
          z.u.en.case_name = cn;
          z.u.en.field_count = argc;
          if (argc > 0) {
            z.u.en.fields = arena_alloc(ctx->arena, (size_t)argc * sizeof *z.u.en.fields);
            if (z.u.en.fields == NULL) {
              eval_fail(ctx, "eval-error", "out of memory");
              return z;
            }
            memcpy(z.u.en.fields, args, (size_t)argc * sizeof *args);
          }
          return z;
        }
      }
      eval_fail(ctx, "eval-error", "unresolved call target");
      return z;
    }
    case STIGI_EXPR_BLOCK: {
      uint32_t saved = *env_count;
      StigiVal last;
      memset(&last, 0, sizeof last);
      last.kind = STIGI_VAL_UNIT;
      for (uint32_t i = 0; i < e->u.block.count; i++) {
        last = eval_expr(ctx, uidx, e->u.block.stmts[i], env, env_count, env_cap);
        if (ctx->failed) { *env_count = saved; return z; }
      }
      *env_count = saved;
      return last;
    }
    case STIGI_EXPR_ASSERT:
      if (e->u.assert_expr.expr != NULL) {
        return eval_expr(ctx, uidx, e->u.assert_expr.expr, env, env_count, env_cap);
      }
      z.kind = STIGI_VAL_UNIT;
      return z;
    case STIGI_EXPR_HANDLE:
      if (e->u.handle_stmt.body != NULL) {
        return eval_expr(ctx, uidx, e->u.handle_stmt.body, env, env_count, env_cap);
      }
      z.kind = STIGI_VAL_UNIT;
      return z;
    default:
      z.kind = STIGI_VAL_UNIT;
      return z;
  }
}

StigiEvalResult stigi_eval_unit(const StigiImage *img, uint32_t unit_idx,
                                uint64_t fuel_max, Arena *arena) {
  EvalCtx ctx;
  memset(&ctx, 0, sizeof ctx);
  ctx.img = img;
  ctx.arena = arena;
  ctx.fuel_max = fuel_max > 0 ? fuel_max : STIGI_DEFAULT_FUEL;
  ctx.fuel_left = ctx.fuel_max;
  ctx.res.status = STIGI_OK;

  if (img == NULL || arena == NULL || unit_idx >= img->unit_count) {
    eval_fail(&ctx, "eval-error", "invalid arguments");
    return ctx.res;
  }

  const StigiDecl *d = img->units[unit_idx].ast->decl;
  EvalBinding env[128];
  uint32_t env_count = 0;

  if (d->kind == STIGI_DECL_VAL || d->kind == STIGI_DECL_TEST) {
    ctx.res.value = eval_expr(&ctx, unit_idx, d->u.val_decl.body, env, &env_count, 128);
  } else if (d->kind == STIGI_DECL_DEF && d->u.def_decl.head.param_count == 0) {
    ctx.res.value = eval_call_unit(&ctx, unit_idx, NULL, 0);
  } else if (d->kind == STIGI_DECL_EXTERN && d->u.extern_decl.body == NULL) {
    eval_fail(&ctx, "not-compile-time", "cannot evaluate body-less extern '%s'",
              img->units[unit_idx].name);
  } else {
    eval_fail(&ctx, "eval-error", "unit '%s' is not directly evaluable",
              img->units[unit_idx].name);
  }

  ctx.res.fuel_used = ctx.fuel_max - ctx.fuel_left;
  return ctx.res;
}

StigiEvalResult stigi_eval_test(const StigiImage *img, uint32_t unit_idx,
                                uint64_t fuel_max, Arena *arena) {
  return stigi_eval_unit(img, unit_idx, fuel_max, arena);
}
