#include "eval.h"

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  StigiStr name;
  StigiVal val;
  bool is_handler_var;
  StigiVal *handler_var_ptr; /* when non-NULL, reads/writes mutate handler state directly */
} EvalBinding;

typedef struct {
  EvalBinding *items;
  uint32_t count;
  uint32_t cap;
} EvalEnv;

typedef struct {
  StigiStr binder;
  int32_t cap_unit_idx;
  int32_t handler_unit_idx;
  StigiStr *param_names;
  StigiVal *param_vals;
  uint32_t param_count;
  StigiStr *var_names;
  StigiVal *var_vals;
  uint32_t var_count;
} EvalHandlerFrame;

typedef struct {
  const StigiImage *img;
  Arena *arena;
  uint64_t fuel_left;
  uint64_t fuel_max;
  uint32_t call_depth;
  EvalHandlerFrame *handlers;
  uint32_t handler_count;
  uint32_t handler_cap;
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

static StigiStr case_short_name(StigiStr s) {
  if (s.ptr == NULL) return s;
  for (size_t i = s.len; i > 0; i--) {
    if (s.ptr[i - 1] == '.') {
      StigiStr r;
      r.ptr = s.ptr + i;
      r.len = s.len - i;
      return r;
    }
  }
  return s;
}

/* ---- Exact integer width & comparison helpers ---- */

static StigiIntWidth width_from_ast_type(const StigiImage *img, uint32_t uidx,
                                         const StigiType *ty, int depth) {
  if (ty == NULL || depth > 16) return STIGI_INT_UNTYPED;
  if (str_eq_lit(ty->name, "U8")) return STIGI_INT_U8;
  if (str_eq_lit(ty->name, "U32")) return STIGI_INT_U32;
  if (str_eq_lit(ty->name, "U64")) return STIGI_INT_U64;
  if (str_eq_lit(ty->name, "I32")) return STIGI_INT_I32;
  if (str_eq_lit(ty->name, "I64")) return STIGI_INT_I64;
  if (img != NULL && uidx < img->unit_count) {
    int32_t tgt = stigi_image_resolve_name(img, uidx, ty->name.ptr, ty->name.len);
    if (tgt >= 0 && img->units[tgt].ast->decl->kind == STIGI_DECL_TYPE) {
      return width_from_ast_type(img, (uint32_t)tgt,
                                 img->units[tgt].ast->decl->u.type_decl.rhs, depth + 1);
    }
  }
  return STIGI_INT_UNTYPED;
}

static bool coerce_int_to_width(EvalCtx *ctx, StigiVal *v, StigiIntWidth w) {
  if (v->kind != STIGI_VAL_INT || w == STIGI_INT_UNTYPED) return true;
  if (w == STIGI_INT_U8 || w == STIGI_INT_U32 || w == STIGI_INT_U64) {
    uint64_t uv = 0;
    if (v->is_unsigned) {
      uv = v->u.u;
    } else {
      if (v->u.i < 0) {
        eval_fail(ctx, "eval-error", "negative value out of unsigned range");
        return false;
      }
      uv = (uint64_t)v->u.i;
    }
    if ((w == STIGI_INT_U8 && uv > 255ULL) ||
        (w == STIGI_INT_U32 && uv > 4294967295ULL)) {
      eval_fail(ctx, "eval-error", "unsigned integer overflow for target width");
      return false;
    }
    v->is_unsigned = true;
    v->int_width = w;
    v->u.u = uv;
    return true;
  }
  /* Signed I32 / I64 */
  int64_t iv = 0;
  if (v->is_unsigned) {
    if (v->u.u > (uint64_t)INT64_MAX) {
      eval_fail(ctx, "eval-error", "integer exceeds signed I64 range");
      return false;
    }
    iv = (int64_t)v->u.u;
  } else {
    iv = v->u.i;
  }
  if (w == STIGI_INT_I32 && (iv < -2147483648LL || iv > 2147483647LL)) {
    eval_fail(ctx, "eval-error", "signed integer overflow for I32");
    return false;
  }
  v->is_unsigned = false;
  v->int_width = w;
  v->u.i = iv;
  return true;
}

static StigiIntWidth combine_widths(const StigiVal *a, const StigiVal *b) {
  if (a->int_width != STIGI_INT_UNTYPED) return a->int_width;
  if (b->int_width != STIGI_INT_UNTYPED) return b->int_width;
  return (a->is_unsigned && b->is_unsigned) ? STIGI_INT_U64 : STIGI_INT_I64;
}

/* Exact mathematical comparison across [INT64_MIN, UINT64_MAX] */
static int cmp_int_vals(const StigiVal *a, const StigiVal *b) {
  if (a->is_unsigned && b->is_unsigned) {
    return a->u.u < b->u.u ? -1 : (a->u.u > b->u.u ? 1 : 0);
  }
  if (!a->is_unsigned && !b->is_unsigned) {
    return a->u.i < b->u.i ? -1 : (a->u.i > b->u.i ? 1 : 0);
  }
  if (!a->is_unsigned) {
    if (a->u.i < 0) return -1;
    uint64_t au = (uint64_t)a->u.i;
    return au < b->u.u ? -1 : (au > b->u.u ? 1 : 0);
  }
  if (b->u.i < 0) return 1;
  uint64_t bu = (uint64_t)b->u.i;
  return a->u.u < bu ? -1 : (a->u.u > bu ? 1 : 0);
}

static bool val_equal(const StigiVal *a, const StigiVal *b) {
  if (a->kind == STIGI_VAL_STR && b->kind == STIGI_VAL_ARRAY) {
    if (a->u.s.len != b->u.arr.len) return false;
    for (uint32_t i = 0; i < b->u.arr.len; i++) {
      if (b->u.arr.elems[i].kind != STIGI_VAL_INT) return false;
      StigiVal ch = { STIGI_VAL_INT, true, STIGI_INT_U8, { .u = (uint8_t)a->u.s.ptr[i] } };
      if (cmp_int_vals(&ch, &b->u.arr.elems[i]) != 0) return false;
    }
    return true;
  }
  if (a->kind == STIGI_VAL_ARRAY && b->kind == STIGI_VAL_STR) {
    return val_equal(b, a);
  }
  if (a->kind != b->kind) return false;
  switch (a->kind) {
    case STIGI_VAL_UNIT:
    case STIGI_VAL_TEST_OK:
      return true;
    case STIGI_VAL_BOOL:
      return a->u.b == b->u.b;
    case STIGI_VAL_INT:
      return cmp_int_vals(a, b) == 0;
    case STIGI_VAL_STR:
      return str_eq_s(a->u.s, b->u.s);
    case STIGI_VAL_ENUM:
      if (a->u.en.enum_unit_idx >= 0 && b->u.en.enum_unit_idx >= 0 &&
          a->u.en.enum_unit_idx != b->u.en.enum_unit_idx) {
        return false;
      }
      if (!str_eq_s(case_short_name(a->u.en.case_name), case_short_name(b->u.en.case_name)) ||
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

/* ---- Dynamic environment helpers ---- */

static void env_init(EvalEnv *env) {
  env->items = NULL;
  env->count = 0;
  env->cap = 0;
}

static void env_free(EvalEnv *env) {
  free(env->items);
  env->items = NULL;
  env->count = env->cap = 0;
}

static bool env_push(EvalCtx *ctx, EvalEnv *env, StigiStr name, StigiVal val,
                     StigiVal *handler_var_ptr) {
  if (name.len == 0 || (name.len == 1 && name.ptr[0] == '_')) return true;
  if (env->count == env->cap) {
    uint32_t ncap = env->cap == 0 ? 32U : env->cap * 2U;
    if (ncap <= env->cap) { eval_fail(ctx, "eval-error", "env overflow"); return false; }
    EvalBinding *ni = realloc(env->items, (size_t)ncap * sizeof *ni);
    if (ni == NULL) { eval_fail(ctx, "eval-error", "out of memory"); return false; }
    env->items = ni;
    env->cap = ncap;
  }
  EvalBinding *b = &env->items[env->count++];
  b->name = name;
  b->val = val;
  b->is_handler_var = (handler_var_ptr != NULL);
  b->handler_var_ptr = handler_var_ptr;
  return true;
}

static EvalBinding *find_env(EvalEnv *env, StigiStr name) {
  for (uint32_t i = env->count; i > 0; i--) {
    if (str_eq_s(env->items[i - 1].name, name)) return &env->items[i - 1];
  }
  return NULL;
}

/* ---- String literal escape decoder ---- */

static uint8_t eval_hex_nibble(char c) {
  if (c >= '0' && c <= '9') return (uint8_t)(c - '0');
  if (c >= 'a' && c <= 'f') return (uint8_t)(c - 'a' + 10);
  if (c >= 'A' && c <= 'F') return (uint8_t)(c - 'A' + 10);
  return 0;
}

static StigiStr decode_str_lit(EvalCtx *ctx, StigiStr raw) {
  StigiStr empty = {"", 0};
  if (raw.ptr == NULL || raw.len < 2 || raw.ptr[0] != '"' || raw.ptr[raw.len - 1] != '"') {
    return raw;
  }
  size_t max_len = raw.len - 2U;
  char *buf = arena_alloc(ctx->arena, max_len + 1U);
  if (buf == NULL) { eval_fail(ctx, "eval-error", "out of memory"); return empty; }
  size_t out = 0;
  size_t i = 1;
  size_t end = raw.len - 1U;
  while (i < end) {
    if (raw.ptr[i] == '\\' && i + 1 < end) {
      char c = raw.ptr[i + 1];
      i += 2;
      switch (c) {
        case 'n': buf[out++] = '\n'; break;
        case 't': buf[out++] = '\t'; break;
        case '\\': buf[out++] = '\\'; break;
        case '"': buf[out++] = '"'; break;
        case '\'': buf[out++] = '\''; break;
        case '0': buf[out++] = '\0'; break;
        case 'x':
          if (i + 2 <= end) {
            uint8_t hi = eval_hex_nibble(raw.ptr[i]);
            uint8_t lo = eval_hex_nibble(raw.ptr[i + 1]);
            buf[out++] = (char)((hi << 4) | lo);
            i += 2;
          }
          break;
        default: buf[out++] = c; break;
      }
    } else {
      buf[out++] = raw.ptr[i++];
    }
  }
  buf[out] = '\0';
  StigiStr res = { buf, out };
  return res;
}

/* ---- Arithmetic helpers ---- */

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

static StigiVal eval_expr(EvalCtx *ctx, uint32_t uidx, const StigiExpr *e, EvalEnv *env);

static bool is_eval_marker_or_region_cap(const StigiImage *img, uint32_t uidx, const StigiType *ty) {
  if (ty == NULL) return true;
  if (str_eq_lit(ty->name, "Stigi.Mut") || str_eq_lit(ty->name, "Stigi.Region") ||
      str_eq_lit(ty->name, "Stigi.Test") || str_eq_lit(ty->name, "Stigi.NegTest")) {
    return true;
  }
  int32_t tgt = stigi_image_resolve_name(img, uidx, ty->name.ptr, ty->name.len);
  if (tgt >= 0 && img->units[tgt].ast->decl->kind == STIGI_DECL_CAPABILITY) {
    const StigiDecl *cd = img->units[tgt].ast->decl;
    bool has_op = false;
    for (uint32_t i = 0; i < cd->u.cap_decl.member_count; i++) {
      if (cd->u.cap_decl.members[i]->kind == STIGI_DECL_DEF) { has_op = true; break; }
    }
    return !has_op;
  }
  return false;
}

static bool eval_has_handler_for_cap(const EvalCtx *ctx, int32_t cap_unit_idx) {
  for (uint32_t i = ctx->handler_count; i > 0; i--) {
    if (ctx->handlers[i - 1].cap_unit_idx == cap_unit_idx) return true;
  }
  return false;
}

static StigiVal eval_call_handler_op(EvalCtx *ctx, EvalHandlerFrame *hf,
                                     StigiStr op_name, const StigiVal *args, uint32_t argc) {
  StigiVal z;
  memset(&z, 0, sizeof z);
  if (ctx->failed || !burn_fuel(ctx)) return z;
  if (ctx->call_depth >= 4096) {
    eval_fail(ctx, "evaluation-fuel", "maximum call depth exceeded");
    return z;
  }
  const StigiDecl *hd = ctx->img->units[hf->handler_unit_idx].ast->decl;
  const StigiDecl *op_def = NULL;
  for (uint32_t i = 0; i < hd->u.handler_decl.def_count; i++) {
    if (str_eq_s(hd->u.handler_decl.defs[i]->name, op_name)) {
      op_def = hd->u.handler_decl.defs[i];
      break;
    }
  }
  if (op_def == NULL) {
    eval_fail(ctx, "eval-error", "handler '%s' does not implement '%.*s'",
              ctx->img->units[hf->handler_unit_idx].name, (int)op_name.len, op_name.ptr);
    return z;
  }
  const StigiDefHead *h = &op_def->u.def_decl.head;
  if (argc != h->param_count) {
    eval_fail(ctx, "eval-error", "handler op '%.*s' arity mismatch",
              (int)op_name.len, op_name.ptr);
    return z;
  }
  EvalEnv frame;
  env_init(&frame);
  for (uint32_t i = 0; i < hf->param_count; i++) {
    if (!env_push(ctx, &frame, hf->param_names[i], hf->param_vals[i], NULL)) {
      env_free(&frame);
      return z;
    }
  }
  for (uint32_t i = 0; i < hf->var_count; i++) {
    if (!env_push(ctx, &frame, hf->var_names[i], hf->var_vals[i], &hf->var_vals[i])) {
      env_free(&frame);
      return z;
    }
  }
  for (uint32_t i = 0; i < h->param_count; i++) {
    StigiVal av = args[i];
    StigiIntWidth pw = width_from_ast_type(ctx->img, (uint32_t)hf->handler_unit_idx,
                                           h->params[i].type, 0);
    if (!coerce_int_to_width(ctx, &av, pw) ||
        !env_push(ctx, &frame, h->params[i].name, av, NULL)) {
      env_free(&frame);
      return z;
    }
  }
  ctx->call_depth++;
  StigiVal out = eval_expr(ctx, (uint32_t)hf->handler_unit_idx, op_def->u.def_decl.body, &frame);
  ctx->call_depth--;
  env_free(&frame);
  if (!ctx->failed) {
    StigiIntWidth rw = width_from_ast_type(ctx->img, (uint32_t)hf->handler_unit_idx,
                                           h->ret_type, 0);
    (void)coerce_int_to_width(ctx, &out, rw);
  }
  return out;
}

static StigiVal eval_call_unit(EvalCtx *ctx, uint32_t tgt_idx,
                               const StigiVal *args, uint32_t arg_count) {
  StigiVal z;
  memset(&z, 0, sizeof z);
  if (ctx->failed || !burn_fuel(ctx)) return z;
  if (ctx->call_depth >= 4096) {
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

  /* Verify every runtime capability required by callee is either pure/Region/Mut or handled */
  for (uint32_t g = 0; g < h->pre_group_count; g++) {
    if (h->pre_groups[g].is_fact) continue;
    for (uint32_t c = 0; c < h->pre_groups[g].cap_count; c++) {
      const StigiType *ct = h->pre_groups[g].caps[c].cap_type;
      if (is_eval_marker_or_region_cap(ctx->img, tgt_idx, ct)) continue;
      int32_t cidx = stigi_image_resolve_name(ctx->img, tgt_idx, ct->name.ptr, ct->name.len);
      if (cidx < 0 || !eval_has_handler_for_cap(ctx, cidx)) {
        eval_fail(ctx, "not-compile-time",
                  "cannot evaluate '%s' requiring unhandled capability '%.*s'",
                  ctx->img->units[tgt_idx].name, (int)ct->name.len, ct->name.ptr);
        return z;
      }
    }
  }

  if (arg_count != h->param_count) {
    eval_fail(ctx, "eval-error", "arity mismatch calling '%s'", ctx->img->units[tgt_idx].name);
    return z;
  }
  EvalEnv frame;
  env_init(&frame);
  for (uint32_t i = 0; i < h->param_count; i++) {
    StigiVal av = args[i];
    StigiIntWidth pw = width_from_ast_type(ctx->img, tgt_idx, h->params[i].type, 0);
    if (!coerce_int_to_width(ctx, &av, pw) ||
        !env_push(ctx, &frame, h->params[i].name, av, NULL)) {
      env_free(&frame);
      return z;
    }
  }
  ctx->call_depth++;
  StigiVal out = eval_expr(ctx, tgt_idx, body, &frame);
  ctx->call_depth--;
  env_free(&frame);
  if (!ctx->failed && d->kind != STIGI_DECL_LEMMA_DEF) {
    StigiIntWidth rw = width_from_ast_type(ctx->img, tgt_idx, h->ret_type, 0);
    (void)coerce_int_to_width(ctx, &out, rw);
  }
  return out;
}

static StigiVal eval_index_read_val(EvalCtx *ctx, const StigiVal *target, const StigiVal *idx) {
  StigiVal z;
  memset(&z, 0, sizeof z);
  if (idx->kind != STIGI_VAL_INT) {
    eval_fail(ctx, "eval-error", "array index is not an integer");
    return z;
  }
  uint64_t u_idx = idx->is_unsigned ? idx->u.u : (idx->u.i >= 0 ? (uint64_t)idx->u.i : UINT64_MAX);
  if (target->kind == STIGI_VAL_ARRAY) {
    if (u_idx >= target->u.arr.len) {
      eval_fail(ctx, "eval-error", "array index read out of bounds");
      return z;
    }
    return target->u.arr.elems[u_idx];
  }
  if (target->kind == STIGI_VAL_STR) {
    if (u_idx >= target->u.s.len) {
      eval_fail(ctx, "eval-error", "string index read out of bounds");
      return z;
    }
    z.kind = STIGI_VAL_INT;
    z.is_unsigned = true;
    z.int_width = STIGI_INT_U8;
    z.u.u = (uint8_t)target->u.s.ptr[u_idx];
    return z;
  }
  eval_fail(ctx, "eval-error", "cannot index non-array value");
  return z;
}

static StigiVal eval_expr(EvalCtx *ctx, uint32_t uidx, const StigiExpr *e, EvalEnv *env) {
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
      if (e->u.lit_int.overflow) {
        eval_fail(ctx, "eval-error", "integer literal exceeds U64 range");
        return z;
      }
      z.kind = STIGI_VAL_INT;
      z.is_unsigned = true;
      z.int_width = STIGI_INT_UNTYPED;
      z.u.u = e->u.lit_int.value;
      return z;
    case STIGI_EXPR_LIT_CHAR:
      z.kind = STIGI_VAL_INT;
      z.is_unsigned = true;
      z.int_width = STIGI_INT_U8;
      z.u.u = (uint64_t)e->u.lit_char.value;
      return z;
    case STIGI_EXPR_LIT_STR:
      z.kind = STIGI_VAL_STR;
      z.u.s = decode_str_lit(ctx, e->u.lit_str.raw);
      return z;
    case STIGI_EXPR_LOCAL:
    case STIGI_EXPR_GLOBAL: {
      EvalBinding *b = find_env(env, e->u.ident.name);
      if (b != NULL) return b->is_handler_var ? *b->handler_var_ptr : b->val;
      int32_t tgt = stigi_image_resolve_name(ctx->img, uidx,
                                             e->u.ident.name.ptr, e->u.ident.name.len);
      if (tgt >= 0) {
        const StigiDecl *d = ctx->img->units[tgt].ast->decl;
        if (d->kind == STIGI_DECL_VAL) {
          if (!burn_fuel(ctx)) return z;
          if (ctx->call_depth >= 4096) {
            eval_fail(ctx, "evaluation-fuel", "maximum val evaluation depth exceeded");
            return z;
          }
          EvalEnv sub;
          env_init(&sub);
          ctx->call_depth++;
          StigiVal rv = eval_expr(ctx, (uint32_t)tgt, d->u.val_decl.body, &sub);
          ctx->call_depth--;
          env_free(&sub);
          if (!ctx->failed) {
            StigiIntWidth vw = width_from_ast_type(ctx->img, (uint32_t)tgt, d->u.val_decl.type, 0);
            (void)coerce_int_to_width(ctx, &rv, vw);
          }
          return rv;
        }
        if (d->kind == STIGI_DECL_EXTERN && d->u.extern_decl.is_val) {
          eval_fail(ctx, "not-compile-time", "cannot read extern val '%s' at compile time",
                    ctx->img->units[tgt].name);
          return z;
        }
      }
      /* Nullary enum constructor (qualified or unqualified) */
      uint32_t cidx = 0;
      bool amb = false;
      int32_t eidx = stigi_image_resolve_enum_case(ctx->img, uidx, e->u.ident.name.ptr,
                                                   e->u.ident.name.len, &cidx, &amb);
      if (eidx >= 0) {
        z.kind = STIGI_VAL_ENUM;
        z.u.en.enum_unit_idx = eidx;
        z.u.en.case_name = ctx->img->units[eidx].ast->decl->u.enum_decl.cases[cidx].name;
        return z;
      }
      if (e->u.ident.name.len > 0 && e->u.ident.name.ptr[0] >= 'A' &&
          e->u.ident.name.ptr[0] <= 'Z') {
        z.kind = STIGI_VAL_ENUM;
        z.u.en.enum_unit_idx = -1;
        z.u.en.case_name = case_short_name(e->u.ident.name);
        return z;
      }
      eval_fail(ctx, "eval-error", "unbound identifier '%.*s'",
                (int)e->u.ident.name.len, e->u.ident.name.ptr);
      return z;
    }
    case STIGI_EXPR_UNOP: {
      StigiVal v = eval_expr(ctx, uidx, e->u.unop.operand, env);
      if (ctx->failed) return z;
      if (e->u.unop.op == STIGI_OP_NOT && v.kind == STIGI_VAL_BOOL) {
        v.u.b = !v.u.b;
        return v;
      }
      if (e->u.unop.op == STIGI_OP_NEG && v.kind == STIGI_VAL_INT) {
        if (v.int_width == STIGI_INT_U8 || v.int_width == STIGI_INT_U32 ||
            v.int_width == STIGI_INT_U64) {
          eval_fail(ctx, "eval-error", "cannot negate unsigned integer");
          return z;
        }
        if (v.is_unsigned) {
          if (v.u.u > 9223372036854775808ULL) {
            eval_fail(ctx, "eval-error", "integer negation overflow");
            return z;
          }
          v.is_unsigned = false;
          v.int_width = STIGI_INT_UNTYPED;
          v.u.i = (v.u.u == 9223372036854775808ULL) ? INT64_MIN : -(int64_t)v.u.u;
          return v;
        }
        if (v.u.i == INT64_MIN || (v.int_width == STIGI_INT_I32 && v.u.i == -2147483648LL)) {
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
        StigiVal lv = eval_expr(ctx, uidx, e->u.binop.lhs, env);
        if (ctx->failed) return z;
        if (lv.kind != STIGI_VAL_BOOL) {
          eval_fail(ctx, "eval-error", "'&&' left operand is not Bool");
          return z;
        }
        if (!lv.u.b) return lv;
        StigiVal rv = eval_expr(ctx, uidx, e->u.binop.rhs, env);
        if (!ctx->failed && rv.kind != STIGI_VAL_BOOL) {
          eval_fail(ctx, "eval-error", "'&&' right operand is not Bool");
          return z;
        }
        return rv;
      }
      if (op == STIGI_OP_OR) {
        StigiVal lv = eval_expr(ctx, uidx, e->u.binop.lhs, env);
        if (ctx->failed) return z;
        if (lv.kind != STIGI_VAL_BOOL) {
          eval_fail(ctx, "eval-error", "'||' left operand is not Bool");
          return z;
        }
        if (lv.u.b) return lv;
        StigiVal rv = eval_expr(ctx, uidx, e->u.binop.rhs, env);
        if (!ctx->failed && rv.kind != STIGI_VAL_BOOL) {
          eval_fail(ctx, "eval-error", "'||' right operand is not Bool");
          return z;
        }
        return rv;
      }
      StigiVal lv = eval_expr(ctx, uidx, e->u.binop.lhs, env);
      if (ctx->failed) return z;
      StigiVal rv = eval_expr(ctx, uidx, e->u.binop.rhs, env);
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
      if (op == STIGI_OP_LT || op == STIGI_OP_LE || op == STIGI_OP_GT || op == STIGI_OP_GE) {
        int c = cmp_int_vals(&lv, &rv);
        z.kind = STIGI_VAL_BOOL;
        if (op == STIGI_OP_LT) z.u.b = (c < 0);
        else if (op == STIGI_OP_LE) z.u.b = (c <= 0);
        else if (op == STIGI_OP_GT) z.u.b = (c > 0);
        else z.u.b = (c >= 0);
        return z;
      }

      StigiIntWidth rw = combine_widths(&lv, &rv);
      if (!coerce_int_to_width(ctx, &lv, rw) || !coerce_int_to_width(ctx, &rv, rw)) return z;

      if (lv.is_unsigned) {
        uint64_t a = lv.u.u, b = rv.u.u;
        uint64_t res_u = 0;
        switch (op) {
          case STIGI_OP_ADD:
            if (a > UINT64_MAX - b) { eval_fail(ctx, "eval-error", "unsigned add overflow"); return z; }
            res_u = a + b;
            break;
          case STIGI_OP_SUB:
            if (a < b) { eval_fail(ctx, "eval-error", "unsigned sub underflow"); return z; }
            res_u = a - b;
            break;
          case STIGI_OP_MUL:
            if (b != 0 && a > UINT64_MAX / b) {
              eval_fail(ctx, "eval-error", "unsigned mul overflow");
              return z;
            }
            res_u = a * b;
            break;
          case STIGI_OP_DIV:
            if (b == 0) { eval_fail(ctx, "eval-error", "division by zero"); return z; }
            res_u = a / b;
            break;
          case STIGI_OP_MOD:
            if (b == 0) { eval_fail(ctx, "eval-error", "remainder by zero"); return z; }
            res_u = a % b;
            break;
          default:
            eval_fail(ctx, "eval-error", "unsupported operator");
            return z;
        }
        z.kind = STIGI_VAL_INT;
        z.is_unsigned = true;
        z.int_width = rw;
        z.u.u = res_u;
        (void)coerce_int_to_width(ctx, &z, rw);
        return z;
      }

      int64_t a = lv.u.i, b = rv.u.i, r = 0;
      switch (op) {
        case STIGI_OP_ADD:
          if (!checked_add_i64(a, b, &r)) { eval_fail(ctx, "eval-error", "signed add overflow"); return z; }
          break;
        case STIGI_OP_SUB:
          if (!checked_sub_i64(a, b, &r)) { eval_fail(ctx, "eval-error", "signed sub overflow"); return z; }
          break;
        case STIGI_OP_MUL:
          if (!checked_mul_i64(a, b, &r)) { eval_fail(ctx, "eval-error", "signed mul overflow"); return z; }
          break;
        case STIGI_OP_DIV:
          if (b == 0 || (a == INT64_MIN && b == -1)) {
            eval_fail(ctx, "eval-error", "division error");
            return z;
          }
          r = a / b;
          break;
        case STIGI_OP_MOD:
          if (b == 0 || (a == INT64_MIN && b == -1)) {
            eval_fail(ctx, "eval-error", "remainder error");
            return z;
          }
          r = a % b;
          break;
        default:
          eval_fail(ctx, "eval-error", "unsupported operator");
          return z;
      }
      z.kind = STIGI_VAL_INT;
      z.is_unsigned = false;
      z.int_width = rw;
      z.u.i = r;
      (void)coerce_int_to_width(ctx, &z, rw);
      return z;
    }
    case STIGI_EXPR_IF: {
      StigiVal cv = eval_expr(ctx, uidx, e->u.if_expr.cond, env);
      if (ctx->failed) return z;
      if (cv.kind != STIGI_VAL_BOOL) {
        eval_fail(ctx, "eval-error", "'if' condition is not Bool");
        return z;
      }
      return eval_expr(ctx, uidx, cv.u.b ? e->u.if_expr.then_br : e->u.if_expr.else_br, env);
    }
    case STIGI_EXPR_WHILE: {
      for (;;) {
        StigiVal cv = eval_expr(ctx, uidx, e->u.while_expr.cond, env);
        if (ctx->failed) return z;
        if (cv.kind != STIGI_VAL_BOOL) {
          eval_fail(ctx, "eval-error", "'while' condition is not Bool");
          return z;
        }
        if (!cv.u.b) break;
        if (!burn_fuel(ctx)) return z;
        (void)eval_expr(ctx, uidx, e->u.while_expr.body, env);
        if (ctx->failed) return z;
      }
      z.kind = STIGI_VAL_UNIT;
      return z;
    }
    case STIGI_EXPR_VAL: {
      StigiVal iv = eval_expr(ctx, uidx, e->u.val_stmt.init, env);
      if (ctx->failed) return z;
      StigiIntWidth w = width_from_ast_type(ctx->img, uidx, e->u.val_stmt.type, 0);
      if (!coerce_int_to_width(ctx, &iv, w)) return z;
      if (e->u.val_stmt.pat.kind == STIGI_PAT_VAR) {
        (void)env_push(ctx, env, e->u.val_stmt.pat.name, iv, NULL);
      } else if (e->u.val_stmt.pat.kind == STIGI_PAT_CTOR && iv.kind == STIGI_VAL_ENUM) {
        for (uint32_t i = 0; i < e->u.val_stmt.pat.field_count && i < iv.u.en.field_count; i++) {
          (void)env_push(ctx, env, e->u.val_stmt.pat.fields[i], iv.u.en.fields[i], NULL);
        }
      }
      z.kind = STIGI_VAL_UNIT;
      return z;
    }
    case STIGI_EXPR_VAR: {
      StigiVal iv = eval_expr(ctx, uidx, e->u.var_stmt.init, env);
      if (ctx->failed) return z;
      StigiIntWidth w = width_from_ast_type(ctx->img, uidx, e->u.var_stmt.type, 0);
      if (!coerce_int_to_width(ctx, &iv, w)) return z;
      (void)env_push(ctx, env, e->u.var_stmt.name, iv, NULL);
      z.kind = STIGI_VAL_UNIT;
      return z;
    }
    case STIGI_EXPR_ASSIGN: {
      StigiVal vv = eval_expr(ctx, uidx, e->u.assign_stmt.value, env);
      if (ctx->failed) return z;
      EvalBinding *b = find_env(env, e->u.assign_stmt.name);
      if (b == NULL) {
        eval_fail(ctx, "eval-error", "assignment to unknown var");
        return z;
      }
      StigiVal *dst = b->is_handler_var ? b->handler_var_ptr : &b->val;
      if (dst->kind == STIGI_VAL_INT && dst->int_width != STIGI_INT_UNTYPED) {
        if (!coerce_int_to_width(ctx, &vv, dst->int_width)) return z;
      }
      *dst = vv;
      z.kind = STIGI_VAL_UNIT;
      return z;
    }
    case STIGI_EXPR_INDEX_READ: {
      StigiVal tv = eval_expr(ctx, uidx, e->u.index_expr.target, env);
      if (ctx->failed) return z;
      StigiVal iv = eval_expr(ctx, uidx, e->u.index_expr.index, env);
      if (ctx->failed) return z;
      return eval_index_read_val(ctx, &tv, &iv);
    }
    case STIGI_EXPR_INDEX_WRITE: {
      if (e->u.index_expr.target->kind != STIGI_EXPR_LOCAL) {
        eval_fail(ctx, "eval-error", "index write target must be a local array");
        return z;
      }
      EvalBinding *b = find_env(env, e->u.index_expr.target->u.ident.name);
      StigiVal idx = eval_expr(ctx, uidx, e->u.index_expr.index, env);
      StigiVal val = eval_expr(ctx, uidx, e->u.index_expr.value, env);
      if (ctx->failed) return z;
      StigiVal *arrv = b != NULL ? (b->is_handler_var ? b->handler_var_ptr : &b->val) : NULL;
      uint64_t u_idx = idx.is_unsigned ? idx.u.u : (idx.u.i >= 0 ? (uint64_t)idx.u.i : UINT64_MAX);
      if (arrv == NULL || arrv->kind != STIGI_VAL_ARRAY || idx.kind != STIGI_VAL_INT ||
          u_idx >= arrv->u.arr.len) {
        eval_fail(ctx, "eval-error", "array index write out of bounds");
        return z;
      }
      arrv->u.arr.elems[u_idx] = val;
      z.kind = STIGI_VAL_UNIT;
      return z;
    }
    case STIGI_EXPR_MATCH: {
      StigiVal sv = eval_expr(ctx, uidx, e->u.match_expr.scrutinee, env);
      if (ctx->failed) return z;
      for (uint32_t i = 0; i < e->u.match_expr.arm_count; i++) {
        const StigiPat *pat = &e->u.match_expr.arms[i].pat;
        uint32_t saved = env->count;
        bool matched = false;
        if (pat->kind == STIGI_PAT_WILD) {
          matched = true;
        } else if (pat->kind == STIGI_PAT_BOOL && sv.kind == STIGI_VAL_BOOL &&
                   sv.u.b == pat->bool_val) {
          matched = true;
        } else if (pat->kind == STIGI_PAT_INT && sv.kind == STIGI_VAL_INT &&
                   (sv.is_unsigned ? (sv.u.u == pat->int_val)
                                   : (sv.u.i >= 0 && (uint64_t)sv.u.i == pat->int_val))) {
          matched = true;
        } else if (pat->kind == STIGI_PAT_VAR) {
          matched = true;
          (void)env_push(ctx, env, pat->name, sv, NULL);
        } else if (pat->kind == STIGI_PAT_CTOR && sv.kind == STIGI_VAL_ENUM &&
                   str_eq_s(case_short_name(pat->name), case_short_name(sv.u.en.case_name))) {
          matched = true;
          for (uint32_t f = 0; f < pat->field_count && f < sv.u.en.field_count; f++) {
            (void)env_push(ctx, env, pat->fields[f], sv.u.en.fields[f], NULL);
          }
        }
        if (matched) {
          StigiVal r = eval_expr(ctx, uidx, e->u.match_expr.arms[i].body, env);
          env->count = saved;
          return r;
        }
        env->count = saved;
      }
      eval_fail(ctx, "eval-error", "no match arm matched");
      return z;
    }
    case STIGI_EXPR_CALL: {
      uint32_t argc = e->u.call.arg_count;
      StigiVal *args = argc > 0 ? malloc((size_t)argc * sizeof *args) : NULL;
      if (argc > 0 && args == NULL) { eval_fail(ctx, "eval-error", "out of memory"); return z; }
      for (uint32_t i = 0; i < argc; i++) {
        args[i] = eval_expr(ctx, uidx, e->u.call.args[i], env);
        if (ctx->failed) { free(args); return z; }
      }
      if (e->u.call.callee->kind == STIGI_EXPR_LOCAL ||
          e->u.call.callee->kind == STIGI_EXPR_GLOBAL) {
        StigiStr cn = e->u.call.callee->u.ident.name;
        /* Local array / string read */
        EvalBinding *lb = find_env(env, cn);
        if (lb != NULL && argc == 1) {
          StigiVal tv = lb->is_handler_var ? *lb->handler_var_ptr : lb->val;
          if (tv.kind == STIGI_VAL_ARRAY || tv.kind == STIGI_VAL_STR) {
            StigiVal out = eval_index_read_val(ctx, &tv, &args[0]);
            free(args);
            return out;
          }
        }
        /* Capability operation call via active handler: binder.op(...) or Cap.op(...) */
        const char *dot = memchr(cn.ptr, '.', cn.len);
        if (dot != NULL) {
          StigiStr prefix = { cn.ptr, (size_t)(dot - cn.ptr) };
          StigiStr op_name = { dot + 1, cn.len - prefix.len - 1U };
          for (uint32_t hi = ctx->handler_count; hi > 0; hi--) {
            EvalHandlerFrame *hf = &ctx->handlers[hi - 1];
            if (hf->binder.len > 0 && str_eq_s(hf->binder, prefix)) {
              StigiVal out = eval_call_handler_op(ctx, hf, op_name, args, argc);
              free(args);
              return out;
            }
          }
          /* Also check unnamed handler match by capability name */
          int32_t cidx = stigi_image_resolve_name(ctx->img, uidx, prefix.ptr, prefix.len);
          if (cidx >= 0) {
            for (uint32_t hi = ctx->handler_count; hi > 0; hi--) {
              if (ctx->handlers[hi - 1].cap_unit_idx == cidx) {
                StigiVal out = eval_call_handler_op(ctx, &ctx->handlers[hi - 1], op_name, args, argc);
                free(args);
                return out;
              }
            }
          }
        }
        /* Stigi.Test.expect(got, want, reason) */
        if (str_eq_lit(cn, "Stigi.Test.expect")) {
          if (argc != 3) {
            free(args);
            eval_fail(ctx, "eval-error", "Stigi.Test.expect requires 3 arguments");
            return z;
          }
          if (args[2].kind != STIGI_VAL_STR) {
            free(args);
            eval_fail(ctx, "eval-error", "Stigi.Test.expect reason must be a string literal");
            return z;
          }
          if (!val_equal(&args[0], &args[1])) {
            eval_fail(ctx, "test-failed", "Stigi.Test.expect failed: %.*s",
                      (int)args[2].u.s.len, args[2].u.s.ptr != NULL ? args[2].u.s.ptr : "");
            free(args);
            return z;
          }
          free(args);
          z.kind = STIGI_VAL_TEST_OK;
          return z;
        }
        /* Builtin pure integer & array operations */
        if (str_eq_lit(cn, "Array.tryFill") && argc == 2 && args[0].kind == STIGI_VAL_INT) {
          uint64_t n = args[0].is_unsigned ? args[0].u.u
                                           : (args[0].u.i >= 0 ? (uint64_t)args[0].u.i : UINT64_MAX);
          if (n > 1048576ULL) {
            z.kind = STIGI_VAL_ENUM;
            z.u.en.enum_unit_idx = -1;
            z.u.en.case_name = (StigiStr){"FillFailed", 10};
            free(args);
            return z;
          }
          StigiVal arr;
          memset(&arr, 0, sizeof arr);
          arr.kind = STIGI_VAL_ARRAY;
          arr.u.arr.len = (uint32_t)n;
          if (n > 0) {
            arr.u.arr.elems = arena_alloc(ctx->arena, (size_t)n * sizeof *arr.u.arr.elems);
            if (arr.u.arr.elems == NULL) { free(args); eval_fail(ctx, "eval-error", "OOM"); return z; }
            for (uint32_t k = 0; k < (uint32_t)n; k++) arr.u.arr.elems[k] = args[1];
          }
          z.kind = STIGI_VAL_ENUM;
          z.u.en.enum_unit_idx = -1;
          z.u.en.case_name = (StigiStr){"Filled", 6};
          z.u.en.field_count = 1;
          z.u.en.fields = arena_alloc(ctx->arena, sizeof *z.u.en.fields);
          if (z.u.en.fields == NULL) { free(args); eval_fail(ctx, "eval-error", "OOM"); return z; }
          z.u.en.fields[0] = arr;
          free(args);
          return z;
        }
        if (str_eq_lit(cn, "Array.tryGrow") && argc == 3 &&
            args[0].kind == STIGI_VAL_ARRAY && args[1].kind == STIGI_VAL_INT) {
          uint64_t n = args[1].is_unsigned ? args[1].u.u
                                           : (args[1].u.i >= 0 ? (uint64_t)args[1].u.i : UINT64_MAX);
          if (n > 1048576ULL || n < args[0].u.arr.len) {
            z.kind = STIGI_VAL_ENUM;
            z.u.en.enum_unit_idx = -1;
            z.u.en.case_name = (StigiStr){"GrowFailed", 10};
            z.u.en.field_count = 1;
            z.u.en.fields = arena_alloc(ctx->arena, sizeof *z.u.en.fields);
            if (z.u.en.fields != NULL) z.u.en.fields[0] = args[0];
            free(args);
            return z;
          }
          StigiVal arr;
          memset(&arr, 0, sizeof arr);
          arr.kind = STIGI_VAL_ARRAY;
          arr.u.arr.len = (uint32_t)n;
          if (n > 0) {
            arr.u.arr.elems = arena_alloc(ctx->arena, (size_t)n * sizeof *arr.u.arr.elems);
            if (arr.u.arr.elems == NULL) { free(args); eval_fail(ctx, "eval-error", "OOM"); return z; }
            for (uint32_t k = 0; k < args[0].u.arr.len; k++) arr.u.arr.elems[k] = args[0].u.arr.elems[k];
            for (uint32_t k = args[0].u.arr.len; k < (uint32_t)n; k++) arr.u.arr.elems[k] = args[2];
          }
          z.kind = STIGI_VAL_ENUM;
          z.u.en.enum_unit_idx = -1;
          z.u.en.case_name = (StigiStr){"Grown", 5};
          z.u.en.field_count = 1;
          z.u.en.fields = arena_alloc(ctx->arena, sizeof *z.u.en.fields);
          if (z.u.en.fields != NULL) z.u.en.fields[0] = arr;
          free(args);
          return z;
        }
        if (str_eq_lit(cn, "U64.ofU8") && argc == 1 && args[0].kind == STIGI_VAL_INT) {
          z = args[0]; z.is_unsigned = true; z.int_width = STIGI_INT_U64;
          (void)coerce_int_to_width(ctx, &z, STIGI_INT_U64);
          free(args); return z;
        }
        if (str_eq_lit(cn, "U64.ofI64") && argc == 1 && args[0].kind == STIGI_VAL_INT) {
          z = args[0];
          (void)coerce_int_to_width(ctx, &z, STIGI_INT_U64);
          free(args); return z;
        }
        if (str_eq_lit(cn, "U8.ofU64") && argc == 1 && args[0].kind == STIGI_VAL_INT) {
          z = args[0];
          (void)coerce_int_to_width(ctx, &z, STIGI_INT_U8);
          free(args); return z;
        }
        if (str_eq_lit(cn, "U32.wrappingAdd") && argc == 2 &&
            args[0].kind == STIGI_VAL_INT && args[1].kind == STIGI_VAL_INT) {
          z.kind = STIGI_VAL_INT; z.is_unsigned = true; z.int_width = STIGI_INT_U32;
          z.u.u = (uint32_t)(args[0].u.u + args[1].u.u);
          free(args); return z;
        }
        if (str_eq_lit(cn, "U32.rotateRight") && argc == 2 &&
            args[0].kind == STIGI_VAL_INT && args[1].kind == STIGI_VAL_INT) {
          uint32_t x = (uint32_t)args[0].u.u;
          uint32_t r = (uint32_t)(args[1].u.u & 31U);
          z.kind = STIGI_VAL_INT; z.is_unsigned = true; z.int_width = STIGI_INT_U32;
          z.u.u = (r == 0) ? x : ((x >> r) | (x << (32U - r)));
          free(args); return z;
        }
        if (str_eq_lit(cn, "U32.bitwiseXor") && argc == 2 &&
            args[0].kind == STIGI_VAL_INT && args[1].kind == STIGI_VAL_INT) {
          z.kind = STIGI_VAL_INT; z.is_unsigned = true; z.int_width = STIGI_INT_U32;
          z.u.u = (uint32_t)(args[0].u.u ^ args[1].u.u);
          free(args); return z;
        }
        if (str_eq_lit(cn, "U64.bitwiseAnd") && argc == 2 &&
            args[0].kind == STIGI_VAL_INT && args[1].kind == STIGI_VAL_INT) {
          z.kind = STIGI_VAL_INT; z.is_unsigned = true; z.int_width = STIGI_INT_U64;
          z.u.u = args[0].u.u & args[1].u.u;
          free(args); return z;
        }
        if (str_eq_lit(cn, "U64.shiftLeft") && argc == 2 &&
            args[0].kind == STIGI_VAL_INT && args[1].kind == STIGI_VAL_INT) {
          if (args[1].u.u >= 64ULL) { free(args); eval_fail(ctx, "eval-error", "shift >= 64"); return z; }
          z.kind = STIGI_VAL_INT; z.is_unsigned = true; z.int_width = STIGI_INT_U64;
          z.u.u = args[0].u.u << args[1].u.u;
          free(args); return z;
        }
        if (str_eq_lit(cn, "U64.shiftRight") && argc == 2 &&
            args[0].kind == STIGI_VAL_INT && args[1].kind == STIGI_VAL_INT) {
          if (args[1].u.u >= 64ULL) { free(args); eval_fail(ctx, "eval-error", "shift >= 64"); return z; }
          z.kind = STIGI_VAL_INT; z.is_unsigned = true; z.int_width = STIGI_INT_U64;
          z.u.u = args[0].u.u >> args[1].u.u;
          free(args); return z;
        }

        int32_t tgt = stigi_image_resolve_name(ctx->img, uidx, cn.ptr, cn.len);
        if (tgt >= 0) {
          StigiVal out = eval_call_unit(ctx, (uint32_t)tgt, args, argc);
          free(args);
          return out;
        }

        /* Enum constructor (qualified Enum.Case or unqualified Case) */
        uint32_t cidx = 0;
        bool amb = false;
        int32_t eidx = stigi_image_resolve_enum_case(ctx->img, uidx, cn.ptr, cn.len, &cidx, &amb);
        if (eidx >= 0 || (cn.len > 0 && cn.ptr[0] >= 'A' && cn.ptr[0] <= 'Z')) {
          z.kind = STIGI_VAL_ENUM;
          z.u.en.enum_unit_idx = eidx;
          z.u.en.case_name = (eidx >= 0)
                                 ? ctx->img->units[eidx].ast->decl->u.enum_decl.cases[cidx].name
                                 : case_short_name(cn);
          z.u.en.field_count = argc;
          if (argc > 0) {
            z.u.en.fields = arena_alloc(ctx->arena, (size_t)argc * sizeof *z.u.en.fields);
            if (z.u.en.fields == NULL) { free(args); eval_fail(ctx, "eval-error", "OOM"); return z; }
            memcpy(z.u.en.fields, args, (size_t)argc * sizeof *args);
          }
          free(args);
          return z;
        }
      }
      free(args);
      eval_fail(ctx, "eval-error", "unresolved call target");
      return z;
    }
    case STIGI_EXPR_HANDLE: {
      int32_t hidx = stigi_image_resolve_name(ctx->img, uidx,
                                              e->u.handle_stmt.handler_name.ptr,
                                              e->u.handle_stmt.handler_name.len);
      bool pushed = false;
      if (hidx >= 0 && ctx->img->units[hidx].ast->decl->kind == STIGI_DECL_HANDLER) {
        const StigiDecl *hd = ctx->img->units[hidx].ast->decl;
        if (ctx->handler_count == ctx->handler_cap) {
          uint32_t ncap = ctx->handler_cap == 0 ? 8U : ctx->handler_cap * 2U;
          EvalHandlerFrame *nh = realloc(ctx->handlers, (size_t)ncap * sizeof *nh);
          if (nh == NULL) { eval_fail(ctx, "eval-error", "out of memory"); return z; }
          ctx->handlers = nh;
          ctx->handler_cap = ncap;
        }
        EvalHandlerFrame hf;
        memset(&hf, 0, sizeof hf);
        hf.binder = e->u.handle_stmt.binder;
        hf.handler_unit_idx = hidx;
        hf.cap_unit_idx = stigi_image_resolve_name(ctx->img, (uint32_t)hidx,
                                                   hd->u.handler_decl.for_cap.ptr,
                                                   hd->u.handler_decl.for_cap.len);
        uint32_t pc = hd->u.handler_decl.val_param_count;
        if (pc > 0) {
          hf.param_names = arena_alloc(ctx->arena, (size_t)pc * sizeof *hf.param_names);
          hf.param_vals = arena_alloc(ctx->arena, (size_t)pc * sizeof *hf.param_vals);
          if (hf.param_names == NULL || hf.param_vals == NULL) {
            eval_fail(ctx, "eval-error", "OOM");
            return z;
          }
          hf.param_count = pc;
          for (uint32_t i = 0; i < pc && i < e->u.handle_stmt.arg_count; i++) {
            hf.param_names[i] = hd->u.handler_decl.val_params[i].name;
            hf.param_vals[i] = eval_expr(ctx, uidx, e->u.handle_stmt.args[i], env);
            if (ctx->failed) return z;
          }
        }
        uint32_t vc = hd->u.handler_decl.var_count;
        if (vc > 0) {
          hf.var_names = arena_alloc(ctx->arena, (size_t)vc * sizeof *hf.var_names);
          hf.var_vals = arena_alloc(ctx->arena, (size_t)vc * sizeof *hf.var_vals);
          if (hf.var_names == NULL || hf.var_vals == NULL) {
            eval_fail(ctx, "eval-error", "OOM");
            return z;
          }
          hf.var_count = vc;
          EvalEnv hinit;
          env_init(&hinit);
          for (uint32_t i = 0; i < pc; i++) {
            (void)env_push(ctx, &hinit, hf.param_names[i], hf.param_vals[i], NULL);
          }
          for (uint32_t i = 0; i < vc; i++) {
            const StigiExpr *vs = hd->u.handler_decl.vars[i];
            hf.var_names[i] = vs->u.var_stmt.name;
            hf.var_vals[i] = eval_expr(ctx, (uint32_t)hidx, vs->u.var_stmt.init, &hinit);
            if (ctx->failed) { env_free(&hinit); return z; }
            StigiIntWidth vw = width_from_ast_type(ctx->img, (uint32_t)hidx, vs->u.var_stmt.type, 0);
            (void)coerce_int_to_width(ctx, &hf.var_vals[i], vw);
            (void)env_push(ctx, &hinit, hf.var_names[i], hf.var_vals[i], NULL);
          }
          env_free(&hinit);
        }
        ctx->handlers[ctx->handler_count++] = hf;
        pushed = true;
      }
      if (e->u.handle_stmt.body != NULL) {
        StigiVal out = eval_expr(ctx, uidx, e->u.handle_stmt.body, env);
        if (pushed && ctx->handler_count > 0) ctx->handler_count--;
        return out;
      }
      z.kind = STIGI_VAL_UNIT;
      return z;
    }
    case STIGI_EXPR_BLOCK: {
      uint32_t saved_env = env->count;
      uint32_t saved_h = ctx->handler_count;
      StigiVal last;
      memset(&last, 0, sizeof last);
      last.kind = STIGI_VAL_UNIT;
      for (uint32_t i = 0; i < e->u.block.count; i++) {
        last = eval_expr(ctx, uidx, e->u.block.stmts[i], env);
        if (ctx->failed) {
          env->count = saved_env;
          ctx->handler_count = saved_h;
          return z;
        }
      }
      env->count = saved_env;
      ctx->handler_count = saved_h;
      return last;
    }
    case STIGI_EXPR_ASSERT:
      if (e->u.assert_expr.expr != NULL) {
        return eval_expr(ctx, uidx, e->u.assert_expr.expr, env);
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
  EvalEnv env;
  env_init(&env);

  if (d->kind == STIGI_DECL_VAL || d->kind == STIGI_DECL_TEST) {
    ctx.res.value = eval_expr(&ctx, unit_idx, d->u.val_decl.body, &env);
    if (!ctx.failed && d->kind == STIGI_DECL_VAL) {
      StigiIntWidth w = width_from_ast_type(img, unit_idx, d->u.val_decl.type, 0);
      (void)coerce_int_to_width(&ctx, &ctx.res.value, w);
    }
  } else if (d->kind == STIGI_DECL_DEF && d->u.def_decl.head.param_count == 0) {
    ctx.res.value = eval_call_unit(&ctx, unit_idx, NULL, 0);
  } else if (d->kind == STIGI_DECL_EXTERN && d->u.extern_decl.body == NULL) {
    eval_fail(&ctx, "not-compile-time", "cannot evaluate body-less extern '%s'",
              img->units[unit_idx].name);
  } else {
    eval_fail(&ctx, "eval-error", "unit '%s' is not directly evaluable",
              img->units[unit_idx].name);
  }

  env_free(&env);
  free(ctx.handlers);
  ctx.res.fuel_used = ctx.fuel_max - ctx.fuel_left;
  return ctx.res;
}

StigiEvalResult stigi_eval_test(const StigiImage *img, uint32_t unit_idx,
                                uint64_t fuel_max, Arena *arena) {
  StigiEvalResult r = stigi_eval_unit(img, unit_idx, fuel_max, arena);
  if (r.status == STIGI_OK && r.value.kind != STIGI_VAL_TEST_OK) {
    r.status = STIGI_ERR_INVALID;
    r.diag_code = "test-failed";
    (void)snprintf(r.msg, sizeof r.msg, "Test did not produce Stigi.Test.Result via Stigi.Test.expect");
  }
  return r;
}
