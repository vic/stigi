#include "emit.h"
#include "datalog.h"
#include "eval.h"
#include "extract.h"
#include "kanren.h"
#include "types.h"

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char *buf;
  size_t len;
  size_t cap;
  bool oom;
} EmitBuf;

static void eb_grow(EmitBuf *b, size_t add) {
  if (b->oom) return;
  if (add > SIZE_MAX - b->len - 1U) {
    b->oom = true;
    return;
  }
  if (b->len + add + 1U <= b->cap) return;
  size_t ncap = b->cap == 0 ? 4096U : b->cap;
  while (ncap < b->len + add + 1U) {
    if (ncap > SIZE_MAX / 2U) {
      ncap = b->len + add + 1U;
      break;
    }
    ncap *= 2U;
  }
  char *nb = realloc(b->buf, ncap);
  if (nb == NULL) {
    b->oom = true;
    return;
  }
  b->buf = nb;
  b->cap = ncap;
}

static void eb_mem(EmitBuf *b, const char *s, size_t n) {
  if (n == 0 || b->oom) return;
  eb_grow(b, n);
  if (b->oom) return;
  memcpy(b->buf + b->len, s, n);
  b->len += n;
  b->buf[b->len] = '\0';
}

static void eb_cstr(EmitBuf *b, const char *s) {
  if (s != NULL) eb_mem(b, s, strlen(s));
}

static void eb_str(EmitBuf *b, StigiStr s) {
  if (s.ptr != NULL && s.len > 0) eb_mem(b, s.ptr, s.len);
}

/* Two-pass exact formatting (zero truncation regardless of length) */
#if defined(__GNUC__) || defined(__clang__)
__attribute__((format(printf, 2, 3)))
#endif
static void eb_fmt(EmitBuf *b, const char *fmt, ...) {
  if (b->oom) return;
  va_list ap, ap2;
  va_start(ap, fmt);
  va_copy(ap2, ap);
  int n = vsnprintf(NULL, 0, fmt, ap);
  va_end(ap);
  if (n <= 0) {
    va_end(ap2);
    return;
  }
  eb_grow(b, (size_t)n);
  if (b->oom) {
    va_end(ap2);
    return;
  }
  (void)vsnprintf(b->buf + b->len, (size_t)n + 1U, fmt, ap2);
  va_end(ap2);
  b->len += (size_t)n;
}

static bool is_c_reserved_ident(const char *s, size_t n) {
  static const char *const k_kw[] = {
      "auto", "break", "case", "char", "const", "continue", "default", "do",
      "double", "else", "enum", "extern", "float", "for", "goto", "if",
      "inline", "int", "long", "register", "restrict", "return", "short",
      "signed", "sizeof", "static", "struct", "switch", "typedef", "union",
      "unsigned", "void", "volatile", "while", "_Alignas", "_Alignof",
      "_Atomic", "_Bool", "_Complex", "_Generic", "_Imaginary", "_Noreturn",
      "_Static_assert", "_Thread_local", "bool", "true", "false", "NULL", "main"
  };
  for (size_t i = 0; i < sizeof(k_kw) / sizeof(k_kw[0]); i++) {
    if (strlen(k_kw[i]) == n && memcmp(k_kw[i], s, n) == 0) return true;
  }
  return false;
}

static void eb_c_ident(EmitBuf *b, StigiStr s) {
  if (s.ptr == NULL || s.len == 0) return;
  if (is_c_reserved_ident(s.ptr, s.len)) eb_cstr(b, "sv_");
  eb_mem(b, s.ptr, s.len);
}

static void eb_mangle_name(EmitBuf *b, const char *name, size_t len) {
  eb_cstr(b, "Stigi_");
  for (size_t i = 0; i < len; i++) {
    char c = name[i] == '.' ? '_' : name[i];
    eb_mem(b, &c, 1);
  }
}

static void eb_unit_path(EmitBuf *b, const char *name, size_t len) {
  for (size_t i = 0; i < len; i++) {
    char c = name[i] == '.' ? '/' : name[i];
    eb_mem(b, &c, 1);
  }
  eb_cstr(b, ".sc");
}

static void eb_line_dir(EmitBuf *b, uint32_t line, const char *unit_name, size_t unit_len) {
  eb_fmt(b, "#line %u \"", line > 0 ? line : 1U);
  eb_unit_path(b, unit_name, unit_len);
  eb_cstr(b, "\"\n");
}

static StigiStr extern_c_sym(StigiStr full_name) {
  for (size_t i = full_name.len; i > 0; i--) {
    if (full_name.ptr[i - 1U] == '.') {
      return (StigiStr){full_name.ptr + i, full_name.len - i};
    }
  }
  return full_name;
}

static bool str_eq_c(StigiStr s, const char *lit) {
  size_t n = strlen(lit);
  return s.len == n && memcmp(s.ptr, lit, n) == 0;
}

static bool str_eq_ss(StigiStr a, StigiStr b) {
  return a.len == b.len && (a.len == 0 || memcmp(a.ptr, b.ptr, a.len) == 0);
}

static uint64_t emit_align_up(uint64_t n, uint64_t align) {
  if (align <= 1ULL) return n;
  uint64_t rem = n % align;
  return rem == 0ULL ? n : n + (align - rem);
}

typedef enum {
  ETY_UNKNOWN = 0,
  ETY_U8,
  ETY_U32,
  ETY_U64,
  ETY_I32,
  ETY_I64,
  ETY_BOOL,
  ETY_UNIT,
  ETY_NEVER
} EmitScalarTy;

static EmitScalarTy ast_type_to_scalar(const StigiImage *img, uint32_t from_idx,
                                       const StigiType *ty, int depth) {
  if (ty == NULL || depth > 16) return ETY_UNKNOWN;
  if (ty->kind == STIGI_TYPE_ARRAY || ty->kind == STIGI_TYPE_CELL || ty->kind == STIGI_TYPE_PTR) {
    return ETY_UNKNOWN;
  }
  if (str_eq_c(ty->name, "U8")) return ETY_U8;
  if (str_eq_c(ty->name, "U32")) return ETY_U32;
  if (str_eq_c(ty->name, "U64")) return ETY_U64;
  if (str_eq_c(ty->name, "I32") || str_eq_c(ty->name, "Stigi.Test.Result")) return ETY_I32;
  if (str_eq_c(ty->name, "I64")) return ETY_I64;
  if (str_eq_c(ty->name, "Bool")) return ETY_BOOL;
  if (str_eq_c(ty->name, "Unit")) return ETY_UNIT;
  if (str_eq_c(ty->name, "Never")) return ETY_NEVER;

  int32_t tgt = stigi_image_resolve_name(img, from_idx, ty->name.ptr, ty->name.len);
  if (tgt >= 0) {
    const StigiDecl *d = img->units[tgt].ast->decl;
    if (d->kind == STIGI_DECL_TYPE) {
      return ast_type_to_scalar(img, (uint32_t)tgt, d->u.type_decl.rhs, depth + 1);
    }
  }
  return ETY_UNKNOWN;
}

static bool ast_type_is_void(const StigiImage *img, uint32_t from_idx, const StigiType *ty) {
  EmitScalarTy st = ast_type_to_scalar(img, from_idx, ty, 0);
  return st == ETY_UNIT || st == ETY_NEVER;
}

static const StigiType *resolve_typedef_type(const StigiImage *img, uint32_t *from_idx,
                                             const StigiType *ty, int depth) {
  if (ty == NULL || depth > 16) return ty;
  if (ty->kind != STIGI_TYPE_NAMED) return ty;
  int32_t tgt = stigi_image_resolve_name(img, *from_idx, ty->name.ptr, ty->name.len);
  if (tgt >= 0) {
    const StigiDecl *d = img->units[tgt].ast->decl;
    if (d->kind == STIGI_DECL_TYPE) {
      *from_idx = (uint32_t)tgt;
      return resolve_typedef_type(img, from_idx, d->u.type_decl.rhs, depth + 1);
    }
  }
  return ty;
}

static StigiLayout ast_type_layout(const StigiImage *img, uint32_t from_idx,
                                   const StigiType *ty, int depth) {
  if (ty == NULL || depth > 16) return (StigiLayout){0, 1};
  if (ty->kind == STIGI_TYPE_ARRAY || ty->kind == STIGI_TYPE_CELL || ty->kind == STIGI_TYPE_PTR) {
    return (StigiLayout){8, 8};
  }
  if (str_eq_c(ty->name, "U8") || str_eq_c(ty->name, "Bool")) return (StigiLayout){1, 1};
  if (str_eq_c(ty->name, "U32") || str_eq_c(ty->name, "I32") ||
      str_eq_c(ty->name, "Stigi.Test.Result")) {
    return (StigiLayout){4, 4};
  }
  if (str_eq_c(ty->name, "U64") || str_eq_c(ty->name, "I64")) return (StigiLayout){8, 8};
  if (str_eq_c(ty->name, "Unit") || str_eq_c(ty->name, "Never")) return (StigiLayout){0, 1};

  int32_t tgt = stigi_image_resolve_name(img, from_idx, ty->name.ptr, ty->name.len);
  if (tgt >= 0) {
    const StigiDecl *d = img->units[tgt].ast->decl;
    if (d->kind == STIGI_DECL_TYPE) {
      return ast_type_layout(img, (uint32_t)tgt, d->u.type_decl.rhs, depth + 1);
    }
    if (d->kind == STIGI_DECL_ENUM) {
      return stigi_enum_layout_info(img, (uint32_t)tgt, NULL, 0).layout;
    }
  }
  return (StigiLayout){8, 8};
}

static void emit_c_type(EmitBuf *b, const StigiImage *img, uint32_t from_idx,
                        const StigiType *ty, int depth) {
  if (ty == NULL || depth > 16) {
    eb_cstr(b, "uint64_t");
    return;
  }
  if (str_eq_c(ty->name, "U8")) { eb_cstr(b, "uint8_t"); return; }
  if (str_eq_c(ty->name, "U32")) { eb_cstr(b, "uint32_t"); return; }
  if (str_eq_c(ty->name, "U64")) { eb_cstr(b, "uint64_t"); return; }
  if (str_eq_c(ty->name, "I32")) { eb_cstr(b, "int32_t"); return; }
  if (str_eq_c(ty->name, "I64")) { eb_cstr(b, "int64_t"); return; }
  if (str_eq_c(ty->name, "Bool")) { eb_cstr(b, "bool"); return; }
  if (str_eq_c(ty->name, "Unit") || str_eq_c(ty->name, "Never")) { eb_cstr(b, "void"); return; }
  if (str_eq_c(ty->name, "Stigi.Test.Result")) { eb_cstr(b, "int32_t"); return; }

  if (ty->kind == STIGI_TYPE_ARRAY || ty->kind == STIGI_TYPE_CELL || ty->kind == STIGI_TYPE_PTR) {
    if (ty->type_arg_count > 0) {
      emit_c_type(b, img, from_idx, ty->type_args[0], depth + 1);
    } else {
      eb_cstr(b, "uint8_t");
    }
    eb_cstr(b, " *");
    return;
  }

  int32_t tgt = stigi_image_resolve_name(img, from_idx, ty->name.ptr, ty->name.len);
  if (tgt >= 0) {
    const StigiDecl *d = img->units[tgt].ast->decl;
    if (d->kind == STIGI_DECL_TYPE) {
      emit_c_type(b, img, (uint32_t)tgt, d->u.type_decl.rhs, depth + 1);
      return;
    }
    if (d->kind == STIGI_DECL_ENUM) {
      eb_mangle_name(b, img->units[tgt].name, img->units[tgt].name_len);
      return;
    }
  }
  eb_cstr(b, "uint64_t");
}

static void emit_scalar_c_type(EmitBuf *b, EmitScalarTy st) {
  switch (st) {
    case ETY_U8: eb_cstr(b, "uint8_t"); break;
    case ETY_U32: eb_cstr(b, "uint32_t"); break;
    case ETY_U64: eb_cstr(b, "uint64_t"); break;
    case ETY_I32: eb_cstr(b, "int32_t"); break;
    case ETY_I64: eb_cstr(b, "int64_t"); break;
    case ETY_BOOL: eb_cstr(b, "bool"); break;
    case ETY_UNIT:
    case ETY_NEVER: eb_cstr(b, "void"); break;
    case ETY_UNKNOWN: eb_cstr(b, "uint64_t"); break;
  }
}

static void emit_typed_int_lit(EmitBuf *b, uint64_t val, bool is_neg, EmitScalarTy hint) {
  if (is_neg) {
    switch (hint) {
      case ETY_I32:
        if (val == 2147483648ULL) {
          eb_cstr(b, "(-INT32_C(2147483647) - INT32_C(1))");
        } else {
          eb_fmt(b, "(-INT32_C(%llu))", (unsigned long long)val);
        }
        return;
      case ETY_I64:
      case ETY_UNKNOWN:
        if (val == 9223372036854775808ULL) {
          eb_cstr(b, "(-INT64_C(9223372036854775807) - INT64_C(1))");
        } else {
          eb_fmt(b, "(-INT64_C(%llu))", (unsigned long long)val);
        }
        return;
      case ETY_U8:
        eb_fmt(b, "(uint8_t)(-%uU)", (unsigned)val);
        return;
      case ETY_U32:
        eb_fmt(b, "(-UINT32_C(%u))", (unsigned)val);
        return;
      case ETY_U64:
      case ETY_BOOL:
      case ETY_UNIT:
      case ETY_NEVER:
        eb_fmt(b, "(-UINT64_C(%llu))", (unsigned long long)val);
        return;
    }
  }
  switch (hint) {
    case ETY_U8:
      eb_fmt(b, "(uint8_t)%uU", (unsigned)val);
      break;
    case ETY_U32:
      eb_fmt(b, "UINT32_C(%u)", (unsigned)val);
      break;
    case ETY_I32:
      if (val <= 2147483647ULL) {
        eb_fmt(b, "INT32_C(%llu)", (unsigned long long)val);
      } else {
        eb_fmt(b, "(int32_t)UINT32_C(%llu)", (unsigned long long)val);
      }
      break;
    case ETY_I64:
      if (val <= 9223372036854775807ULL) {
        eb_fmt(b, "INT64_C(%llu)", (unsigned long long)val);
      } else {
        eb_fmt(b, "(int64_t)UINT64_C(%llu)", (unsigned long long)val);
      }
      break;
    case ETY_U64:
    case ETY_BOOL:
    case ETY_UNIT:
    case ETY_NEVER:
    case ETY_UNKNOWN:
      eb_fmt(b, "UINT64_C(%llu)", (unsigned long long)val);
      break;
  }
}

static const char *c_op_str(StigiOp op) {
  switch (op) {
    case STIGI_OP_ADD: return "+";
    case STIGI_OP_SUB: return "-";
    case STIGI_OP_MUL: return "*";
    case STIGI_OP_DIV: return "/";
    case STIGI_OP_MOD: return "%";
    case STIGI_OP_LT: return "<";
    case STIGI_OP_LE: return "<=";
    case STIGI_OP_GT: return ">";
    case STIGI_OP_GE: return ">=";
    case STIGI_OP_EQ: return "==";
    case STIGI_OP_NE: return "!=";
    case STIGI_OP_AND: return "&&";
    case STIGI_OP_OR: return "||";
    case STIGI_OP_NOT: return "!";
    case STIGI_OP_NEG: return "-";
    case STIGI_OP_PIPE: return "|>";
  }
  return "+";
}

/* ---- Datalog maskIndex decision set ---- */

typedef struct {
  bool *access_masked;
  uint32_t access_count;
  uint32_t access_cap;
  bool is_pure_def;
} UnitMaskInfo;

static void free_unit_masks(UnitMaskInfo *info, uint32_t count) {
  if (info == NULL) return;
  for (uint32_t i = 0; i < count; i++) {
    free(info[i].access_masked);
  }
  free(info);
}

/* S134 e: the emitter evaluates no Datalog. It reads the decision rows that the door's one evaluation of each
 * unit cached under the verdict key: a masked flag per element access row and the four relation counts. */
static Status compute_datalog_masks(const StigiDoor *door, UnitMaskInfo **out_info,
                                    uint32_t *out_total_masks, StigiEmitStats *stats,
                                    Diag *diag) {
  const StigiImage *img = &door->image;
  *out_info = NULL;
  *out_total_masks = 0;
  if (img->unit_count == 0) return STIGI_OK;

  UnitMaskInfo *info = calloc(img->unit_count, sizeof *info);
  if (info == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "compute_datalog_masks: OOM");

  uint32_t total_masked = 0;
  for (uint32_t i = 0; i < img->unit_count; i++) {
    if (!img->units[i].reachable) continue;
    const StigiImageUnit *u = &img->units[i];
    const StigiDecl *d = u->ast->decl;
    if (d->kind != STIGI_DECL_DEF) continue;

    const StigiDefHead *h = &d->u.def_decl.head;
    bool has_cap = false;
    for (uint32_t g = 0; g < h->pre_group_count; g++) {
      if (!h->pre_groups[g].is_fact && h->pre_groups[g].cap_count > 0) has_cap = true;
    }
    if (!has_cap) {
      info[i].is_pure_def = true;
      if (stats != NULL) stats->pure_defs++;
    }

    uint8_t key[32];
    Status st = stigi_door_unit_key(door, i, key, diag);
    if (st != STIGI_OK) {
      free_unit_masks(info, img->unit_count);
      return st;
    }
    const StigiUnitDecision *dec = stigi_decisions_find(&door->decisions, key);
    if (dec == NULL) {
      free_unit_masks(info, img->unit_count);
      return diag_set(diag, STIGI_ERR_INVALID, -1,
                      "no decision rows for unit '%s': the door has not evaluated it under this rule program",
                      u->name);
    }
    if (!dec->tracked) continue;

    UnitMaskInfo *ui = &info[i];
    if (dec->access_count > 0) {
      ui->access_masked = malloc((size_t)dec->access_count * sizeof *ui->access_masked);
      if (ui->access_masked == NULL) {
        free_unit_masks(info, img->unit_count);
        return diag_set(diag, STIGI_ERR_OOM, -1, "access_masked OOM");
      }
      ui->access_cap = dec->access_count;
      for (uint32_t a = 0; a < dec->access_count; a++) {
        ui->access_masked[a] = dec->masked[a] != 0;
        if (ui->access_masked[a]) total_masked++;
      }
      ui->access_count = dec->access_count;
    }
    if (stats != NULL) {
      stats->free_edges += dec->free_edges;
      stats->free_at_exit += dec->free_at_exit;
      stats->reuses += dec->reuses;
      stats->stack_objects += dec->stack_objects;
    }
  }

  *out_info = info;
  *out_total_masks = total_masked;
  return STIGI_OK;
}

/* ---- Emitter context (dynamic vars, shadowing hygiene, handler binders) ---- */

typedef struct {
  StigiStr name;
  char *c_name;
  const StigiType *ast_type;
  const StigiExpr *len_expr;
  uint32_t type_uidx;
  EmitScalarTy scalar_ty;
  EmitScalarTy elem_scalar_ty;
  bool is_array;
  bool is_cell;
  bool is_ptr;
  bool is_handler_state_field;
  int32_t enum_uidx;
  int32_t elem_enum_uidx;
  int32_t handler_uidx; /* >= 0 if capability binder bound by `handle` */
  int32_t cap_uidx;     /* >= 0 if capability unit (from `handle` or `{Cap}` param) */
  char *handler_state_var;
  char *cap_handle_var;
} EmitVar;

typedef struct {
  uint32_t uidx;
  char *key;
  const StigiStr *tp_names;
  const StigiType **tp_vals;
  uint32_t tp_count;
} MonoInst;

typedef struct {
  MonoInst *items;
  uint32_t count;
  uint32_t cap;
} MonoList;

static void mono_list_free(MonoList *ml) {
  if (ml == NULL) return;
  for (uint32_t i = 0; i < ml->count; i++) {
    free(ml->items[i].key);
    free(ml->items[i].tp_vals);
  }
  free(ml->items);
  ml->items = NULL;
  ml->count = 0;
  ml->cap = 0;
}

/* The stigi name of a scalar C type (U64 when it is not known) */
static const char *scalar_type_name(EmitScalarTy ty) {
  switch (ty) {
    case ETY_U8: return "U8";
    case ETY_U32: return "U32";
    case ETY_I32: return "I32";
    case ETY_I64: return "I64";
    case ETY_BOOL: return "Bool";
    case ETY_UNIT: return "Unit";
    default: return "U64";
  }
}

static const StigiType *mono_subst_type(Arena *arena, const StigiStr *tp_names,
                                        const StigiType *const *tp_vals,
                                        uint32_t tp_count, const StigiType *ty) {
  if (ty == NULL || tp_count == 0 || tp_names == NULL || tp_vals == NULL) return ty;
  if (ty->kind == STIGI_TYPE_NAMED && ty->type_arg_count == 0) {
    for (uint32_t i = 0; i < tp_count; i++) {
      if (str_eq_ss(ty->name, tp_names[i]) && tp_vals[i] != NULL) return tp_vals[i];
    }
    return ty;
  }
  if (ty->type_arg_count == 0 || arena == NULL) return ty;
  /* a type parameter inside the type arguments (Array[A](n), Cell[A]): a copy with them substituted */
  StigiType **args = arena_alloc(arena, (size_t)ty->type_arg_count * sizeof *args);
  StigiType *copy = arena_alloc(arena, sizeof *copy);
  if (args == NULL || copy == NULL) return ty;
  bool changed = false;
  for (uint32_t i = 0; i < ty->type_arg_count; i++) {
    args[i] = (StigiType *)mono_subst_type(arena, tp_names, tp_vals, tp_count, ty->type_args[i]);
    changed = changed || args[i] != ty->type_args[i];
  }
  if (!changed) return ty;
  *copy = *ty;
  copy->type_args = args;
  return copy;
}

typedef struct {
  const StigiImage *img;
  uint32_t uidx;
  const StigiDecl *cur_def;
  const StigiDefHead *cur_head;
  EmitBuf *b;
  EmitBuf *pre;
  StigiEmitStats *stats;
  const UnitMaskInfo *mask_info;
  uint32_t access_seq;
  EmitVar *vars;
  uint32_t var_count;
  uint32_t var_cap;
  char **param_cnames; /* original parameter C names for tail-loop updates */
  uint32_t param_cname_count;
  uint32_t tmp_id;
  uint32_t shadow_seq;
  bool is_tail_loop;
  bool is_never_def;
  bool in_extern_dsl;
  bool fail_closed;
  char fail_msg[256];
  EmitScalarTy ret_scalar;
  const StigiType *ret_ast_type;
  uint32_t ret_type_uidx;
  const StigiStr *mono_tp_names;
  const StigiType *const *mono_tp_vals;
  uint32_t mono_tp_count;
  MonoList *mono_list;
  Arena *mono_arena;
  Arena term_arena; /* case-binder length terms (case_binder_len); zeroed = empty */
} EmitCtx;

static void emit_ctx_pop_vars(EmitCtx *ctx, uint32_t saved_count) {
  while (ctx->var_count > saved_count) {
    ctx->var_count--;
    free(ctx->vars[ctx->var_count].c_name);
    free(ctx->vars[ctx->var_count].handler_state_var);
    free(ctx->vars[ctx->var_count].cap_handle_var);
    ctx->vars[ctx->var_count].c_name = NULL;
    ctx->vars[ctx->var_count].handler_state_var = NULL;
    ctx->vars[ctx->var_count].cap_handle_var = NULL;
  }
}

static void emit_ctx_free(EmitCtx *ctx) {
  emit_ctx_pop_vars(ctx, 0);
  free(ctx->vars);
  ctx->vars = NULL;
  ctx->var_cap = 0;
  for (uint32_t i = 0; i < ctx->param_cname_count; i++) free(ctx->param_cnames[i]);
  free(ctx->param_cnames);
  ctx->param_cnames = NULL;
  ctx->param_cname_count = 0;
  arena_free(&ctx->term_arena);
}

static const EmitVar *emit_find_var(const EmitCtx *ctx, StigiStr name) {
  if (name.len == 0) return NULL;
  for (uint32_t i = ctx->var_count; i > 0; i--) {
    if (str_eq_ss(ctx->vars[i - 1].name, name)) {
      return &ctx->vars[i - 1];
    }
  }
  return NULL;
}

static const EmitVar *emit_find_cap_var(const EmitCtx *ctx, int32_t cap_uidx) {
  if (cap_uidx < 0) return NULL;
  for (uint32_t i = ctx->var_count; i > 0; i--) {
    if (ctx->vars[i - 1].cap_uidx == cap_uidx) {
      return &ctx->vars[i - 1];
    }
  }
  return NULL;
}

static bool emit_is_c_scalar_type(const char *s, size_t len) {
  return (len == 7 && memcmp(s, "uint8_t", 7) == 0) ||
         (len == 8 && memcmp(s, "uint32_t", 8) == 0) ||
         (len == 8 && memcmp(s, "uint64_t", 8) == 0) ||
         (len == 6 && memcmp(s, "int8_t", 6) == 0) ||
         (len == 7 && memcmp(s, "int32_t", 7) == 0) ||
         (len == 7 && memcmp(s, "int64_t", 7) == 0) ||
         (len == 4 && memcmp(s, "bool", 4) == 0);
}

static bool cap_has_runtime_handle(const StigiImage *img, uint32_t cap_uidx) {
  if (img == NULL || cap_uidx >= img->unit_count) return false;
  const StigiDecl *cd = img->units[cap_uidx].ast->decl;
  if (cd->kind != STIGI_DECL_CAPABILITY) return false;
  bool has_ops = false;
  for (uint32_t m = 0; m < cd->u.cap_decl.member_count; m++) {
    if (cd->u.cap_decl.members[m]->kind == STIGI_DECL_DEF) { has_ops = true; break; }
  }
  if (!has_ops) return false;
  /* Every handler counts: a capability in main's closure is handled inside that closure, so a filter on
   * `reachable` here can never change the answer (S163 review: that mutant is equivalent). */
  for (uint32_t i = 0; i < img->unit_count; i++) {
    const StigiDecl *hd = img->units[i].ast->decl;
    if (hd->kind != STIGI_DECL_HANDLER) continue;
    int32_t fc = stigi_image_resolve_name(img, i, hd->u.handler_decl.for_cap.ptr,
                                          hd->u.handler_decl.for_cap.len);
    if (fc == (int32_t)cap_uidx) return true;
  }
  return false;
}

static int32_t find_handler_tag_for_cap(const StigiImage *img, uint32_t cap_uidx, uint32_t h_uidx) {
  int32_t tag = 0;
  for (uint32_t i = 0; i < img->unit_count; i++) {
    if (!img->units[i].reachable) continue;
    const StigiDecl *hd = img->units[i].ast->decl;
    if (hd->kind != STIGI_DECL_HANDLER) continue;
    int32_t fc = stigi_image_resolve_name(img, i, hd->u.handler_decl.for_cap.ptr,
                                          hd->u.handler_decl.for_cap.len);
    if (fc == (int32_t)cap_uidx) {
      if (i == h_uidx) return tag;
      tag++;
    }
  }
  return -1;
}

static void emit_var_cname(EmitBuf *b, const EmitVar *v) {
  if (v->is_handler_state_field) {
    eb_cstr(b, "_hstate->");
  }
  if (v->c_name != NULL) {
    eb_cstr(b, v->c_name);
  } else {
    eb_c_ident(b, v->name);
  }
}

static bool ctx_next_access_masked(EmitCtx *ctx) {
  uint32_t seq = ctx->access_seq++;
  if (ctx->mask_info != NULL && seq < ctx->mask_info->access_count) {
    return ctx->mask_info->access_masked[seq];
  }
  return false;
}

static EmitVar *emit_push_var_raw(EmitCtx *ctx, StigiStr name, bool allow_shadow_suffix) {
  if (ctx->var_count == ctx->var_cap) {
    uint32_t ncap = ctx->var_cap == 0 ? 32U : ctx->var_cap * 2U;
    EmitVar *nv = realloc(ctx->vars, (size_t)ncap * sizeof *nv);
    if (nv == NULL) {
      if (ctx->b != NULL) ctx->b->oom = true;
      return NULL;
    }
    ctx->vars = nv;
    ctx->var_cap = ncap;
  }
  bool shadowed = allow_shadow_suffix && (emit_find_var(ctx, name) != NULL);
  EmitBuf cn;
  memset(&cn, 0, sizeof cn);
  eb_c_ident(&cn, name);
  if (shadowed) {
    eb_fmt(&cn, "_%u", ++ctx->shadow_seq);
  }
  EmitVar *v = &ctx->vars[ctx->var_count++];
  memset(v, 0, sizeof *v);
  v->name = name;
  v->c_name = cn.buf;
  v->enum_uidx = -1;
  v->elem_enum_uidx = -1;
  v->handler_uidx = -1;
  v->cap_uidx = -1;
  return v;
}

static EmitVar *emit_register_var(EmitCtx *ctx, StigiStr name, const StigiType *ty,
                                  uint32_t type_uidx, bool is_param) {
  EmitVar *v = emit_push_var_raw(ctx, name, !is_param);
  if (v == NULL) return NULL;
  v->ast_type = ty;
  v->type_uidx = type_uidx;
  if (ty == NULL) return v;

  uint32_t r_uidx = type_uidx;
  const StigiType *rty = resolve_typedef_type(ctx->img, &r_uidx, ty, 0);
  v->scalar_ty = ast_type_to_scalar(ctx->img, type_uidx, ty, 0);
  if (rty != NULL && (rty->kind == STIGI_TYPE_ARRAY || rty->kind == STIGI_TYPE_PTR)) {
    v->is_array = true;
    v->is_ptr = (rty->kind == STIGI_TYPE_PTR);
    if (rty->len_arg_count > 0) v->len_expr = rty->len_args[0];
    if (rty->type_arg_count > 0) {
      v->elem_scalar_ty = ast_type_to_scalar(ctx->img, r_uidx, rty->type_args[0], 0);
      uint32_t eru = r_uidx;
      const StigiType *erty = resolve_typedef_type(ctx->img, &eru, rty->type_args[0], 0);
      if (erty != NULL && erty->kind == STIGI_TYPE_NAMED) {
        int32_t eet = stigi_image_resolve_name(ctx->img, eru, erty->name.ptr, erty->name.len);
        if (eet >= 0 && ctx->img->units[eet].ast->decl->kind == STIGI_DECL_ENUM) {
          v->elem_enum_uidx = eet;
        }
      }
    }
  } else if (rty != NULL && rty->kind == STIGI_TYPE_CELL) {
    v->is_cell = true;
    if (rty->type_arg_count > 0) {
      v->elem_scalar_ty = ast_type_to_scalar(ctx->img, r_uidx, rty->type_args[0], 0);
      uint32_t eru = r_uidx;
      const StigiType *erty = resolve_typedef_type(ctx->img, &eru, rty->type_args[0], 0);
      if (erty != NULL && erty->kind == STIGI_TYPE_NAMED) {
        int32_t eet = stigi_image_resolve_name(ctx->img, eru, erty->name.ptr, erty->name.len);
        if (eet >= 0 && ctx->img->units[eet].ast->decl->kind == STIGI_DECL_ENUM) {
          v->elem_enum_uidx = eet;
        }
      }
    }
  } else if (rty != NULL && rty->kind == STIGI_TYPE_NAMED) {
    int32_t et = stigi_image_resolve_name(ctx->img, r_uidx, rty->name.ptr, rty->name.len);
    if (et >= 0 && ctx->img->units[et].ast->decl->kind == STIGI_DECL_ENUM) {
      v->enum_uidx = et;
    }
  }
  return v;
}


/* A case pattern's binders over one scrutinee. `src` is the C text of the case payload up to a field name. */
typedef struct {
  const StigiEnumCase *ec;
  const StigiStr *binders;
  uint32_t binder_count;
  uint32_t type_uidx;
  const char *src;
  uint32_t first_var; /* var_count before the pattern; its hidden length locals sit above it */
} CaseBind;

static const StigiExpr *case_len_fail(EmitCtx *ctx, const char *why) {
  ctx->fail_closed = true;
  (void)snprintf(ctx->fail_msg, sizeof ctx->fail_msg, "%s in '%s'", why, ctx->img->units[ctx->uidx].name);
  return NULL;
}

/* The name of field f under the pattern: its binder, or for a `_` binder a hidden local `_len_<field>` (no source
 * identifier starts with `_`) read from the scrutinee once per pattern. Empty name = out of memory. */
static StigiStr case_field_name(EmitCtx *ctx, const CaseBind *cb, uint32_t f) {
  StigiStr none = {NULL, 0};
  if (f < cb->binder_count && !(cb->binders[f].len == 1 && cb->binders[f].ptr[0] == '_')) return cb->binders[f];
  if (cb->src == NULL) return none;
  StigiStr field = cb->ec->fields[f].name;
  char *buf = arena_alloc(&ctx->term_arena, (size_t)field.len + 5U);
  if (buf == NULL) return none;
  memcpy(buf, "_len_", 5U);
  memcpy(buf + 5, field.ptr, field.len);
  StigiStr name = {buf, field.len + 5U};
  for (uint32_t i = cb->first_var; i < ctx->var_count; i++) {
    if (str_eq_ss(ctx->vars[i].name, name)) return name;
  }
  EmitVar *v = emit_register_var(ctx, name, cb->ec->fields[f].type, cb->type_uidx, false);
  if (v == NULL) return none;
  eb_cstr(ctx->b, "  ");
  emit_c_type(ctx->b, ctx->img, cb->type_uidx, cb->ec->fields[f].type, 0);
  eb_cstr(ctx->b, " const ");
  emit_var_cname(ctx->b, v);
  eb_fmt(ctx->b, " = %s", cb->src);
  eb_c_ident(ctx->b, field);
  eb_cstr(ctx->b, ";\n  (void)");
  emit_var_cname(ctx->b, v);
  eb_cstr(ctx->b, ";\n");
  return name;
}

/* types.md 3: a case field's length term names the case's fields over literals, top-level vals and linear ops, and a
 * pattern binds the fields under its own names, so a binder's length term is its field's term with every field name
 * replaced by that field's name under the pattern. Any other term kind, or out of memory, fails closed (NULL). */
static const StigiExpr *case_binder_len(EmitCtx *ctx, const CaseBind *cb, const StigiExpr *t) {
  if (t == NULL) return NULL;
  switch (t->kind) {
    case STIGI_EXPR_LIT_INT:
    case STIGI_EXPR_GLOBAL:
      return t;
    case STIGI_EXPR_LOCAL: {
      uint32_t f = 0;
      while (f < cb->ec->field_count && !str_eq_ss(cb->ec->fields[f].name, t->u.ident.name)) f++;
      if (f == cb->ec->field_count) return t;
      StigiStr name = case_field_name(ctx, cb, f);
      if (name.ptr == NULL) return case_len_fail(ctx, "out of memory renaming a case field length term");
      if (str_eq_ss(name, t->u.ident.name)) return t;
      StigiExpr *r = arena_alloc(&ctx->term_arena, sizeof *r);
      if (r == NULL) return case_len_fail(ctx, "out of memory renaming a case field length term");
      *r = *t;
      r->u.ident.name = name;
      return r;
    }
    case STIGI_EXPR_BINOP: {
      const StigiExpr *l = case_binder_len(ctx, cb, t->u.binop.lhs);
      const StigiExpr *rh = case_binder_len(ctx, cb, t->u.binop.rhs);
      if (l == NULL || rh == NULL) return NULL;
      if (l == t->u.binop.lhs && rh == t->u.binop.rhs) return t;
      StigiExpr *r = arena_alloc(&ctx->term_arena, sizeof *r);
      if (r == NULL) return case_len_fail(ctx, "out of memory renaming a case field length term");
      *r = *t;
      r->u.binop.lhs = (StigiExpr *)l;
      r->u.binop.rhs = (StigiExpr *)rh;
      return r;
    }
    default:
      return case_len_fail(ctx, "case field length term is not a literal, top-level val, binder or linear op");
  }
}
/* An untyped alias `val x = y` / `var x = y` of an array or cell local keeps y's identity, as the extractor's Val and
 * Var binders do (nested patterns bind `val cells = patFieldN`). Searches below `v`, so a shadowing alias never finds
 * itself. */
static void emit_copy_alias(const EmitCtx *ctx, const StigiExpr *init, EmitVar *v) {
  if (v == NULL || init == NULL || init->kind != STIGI_EXPR_LOCAL || v->is_array || v->is_cell) return;
  for (uint32_t i = (uint32_t)(v - ctx->vars); i > 0; i--) {
    const EmitVar *a = &ctx->vars[i - 1];
    if (!str_eq_ss(a->name, init->u.ident.name)) continue;
    if (a->is_array || a->is_cell) {
      v->is_array = a->is_array;
      v->is_cell = a->is_cell;
      v->is_ptr = a->is_ptr;
      v->len_expr = a->len_expr;
      v->elem_scalar_ty = a->elem_scalar_ty;
      v->elem_enum_uidx = a->elem_enum_uidx;
      v->ast_type = a->ast_type;
      v->type_uidx = a->type_uidx;
    }
    return;
  }
}

static size_t unit_ns_prefix_len(const char *name, size_t len) {
  size_t last = 0;
  for (size_t i = 0; i < len; i++) {
    if (name[i] == '.') last = i;
  }
  return last;
}

static bool find_case_in_enum(const StigiDecl *ed, StigiStr case_name, uint32_t *out_cidx) {
  if (ed == NULL || ed->kind != STIGI_DECL_ENUM) return false;
  /* Support both short `CaseName` and qualified `EnumName.CaseName` */
  StigiStr short_n = case_name;
  for (size_t i = case_name.len; i > 0; i--) {
    if (case_name.ptr[i - 1] == '.') {
      short_n.ptr = case_name.ptr + i;
      short_n.len = case_name.len - i;
      break;
    }
  }
  for (uint32_t c = 0; c < ed->u.enum_decl.case_count; c++) {
    if (str_eq_ss(ed->u.enum_decl.cases[c].name, short_n)) {
      if (out_cidx != NULL) *out_cidx = c;
      return true;
    }
  }
  return false;
}

/* Deterministic enum case lookup: qualified first, then visibility -> same ns -> unit_id -> name */
static int32_t find_enum_case_owner_det(const StigiImage *img, uint32_t from_idx,
                                        StigiStr case_name, uint32_t *out_cidx) {
  uint32_t rc = 0;
  bool amb = false;
  int32_t direct = stigi_image_resolve_enum_case(img, from_idx, case_name.ptr, case_name.len,
                                                 &rc, &amb);
  if (direct >= 0 && !amb) {
    if (out_cidx != NULL) *out_cidx = rc;
    return direct;
  }

  int32_t best_idx = -1;
  uint32_t best_cidx = 0;
  int best_rank = -1;

  const StigiImageUnit *from_u = from_idx < img->unit_count ? &img->units[from_idx] : NULL;
  size_t from_ns = from_u != NULL ? unit_ns_prefix_len(from_u->name, from_u->name_len) : 0;

  for (uint32_t i = 0; i < img->unit_count; i++) {
    const StigiImageUnit *u = &img->units[i];
    const StigiDecl *d = u->ast->decl;
    if (d->kind != STIGI_DECL_ENUM) continue;
    uint32_t cidx = 0;
    if (!find_case_in_enum(d, case_name, &cidx)) continue;

    bool vis = (from_u == NULL) || stigi_image_visible_from(img, i, from_idx);
    size_t u_ns = unit_ns_prefix_len(u->name, u->name_len);
    bool same_ns = (from_u != NULL && u_ns == from_ns &&
                    (u_ns == 0 || memcmp(u->name, from_u->name, u_ns) == 0));
    int rank = (vis ? 2 : 0) + (same_ns ? 1 : 0);

    bool better = false;
    if (best_idx < 0 || rank > best_rank) {
      better = true;
    } else if (rank == best_rank) {
      int id_cmp = memcmp(u->unit_id, img->units[best_idx].unit_id, 32);
      if (id_cmp < 0 || (id_cmp == 0 && strcmp(u->name, img->units[best_idx].name) < 0)) {
        better = true;
      }
    }
    if (better) {
      best_idx = (int32_t)i;
      best_cidx = cidx;
      best_rank = rank;
    }
  }
  if (best_idx >= 0 && out_cidx != NULL) *out_cidx = best_cidx;
  return best_idx;
}

static bool is_self_tail_call(const EmitCtx *ctx, const StigiExpr *e) {
  if (e == NULL || ctx->cur_def == NULL) return false;
  if (e->kind == STIGI_EXPR_CALL &&
      (e->u.call.callee->kind == STIGI_EXPR_LOCAL || e->u.call.callee->kind == STIGI_EXPR_GLOBAL)) {
    StigiStr cn = e->u.call.callee->u.ident.name;
    int32_t tgt = stigi_image_resolve_name(ctx->img, ctx->uidx, cn.ptr, cn.len);
    return tgt >= 0 && (uint32_t)tgt == ctx->uidx;
  }
  return false;
}

static bool has_self_tail_call(const EmitCtx *ctx, const StigiExpr *e) {
  if (e == NULL) return false;
  if (is_self_tail_call(ctx, e)) return true;
  if (e->kind == STIGI_EXPR_IF) {
    return has_self_tail_call(ctx, e->u.if_expr.then_br) ||
           has_self_tail_call(ctx, e->u.if_expr.else_br);
  }
  if (e->kind == STIGI_EXPR_BLOCK && e->u.block.count > 0) {
    return has_self_tail_call(ctx, e->u.block.stmts[e->u.block.count - 1]);
  }
  if (e->kind == STIGI_EXPR_MATCH) {
    for (uint32_t i = 0; i < e->u.match_expr.arm_count; i++) {
      if (has_self_tail_call(ctx, e->u.match_expr.arms[i].body)) return true;
    }
    return false;
  }
  if (e->kind == STIGI_EXPR_HANDLE) {
    return has_self_tail_call(ctx, e->u.handle_stmt.body);
  }
  return false;
}

static EmitScalarTy infer_expr_scalar(const EmitCtx *ctx, const StigiExpr *e) {
  if (e == NULL) return ETY_UNKNOWN;
  switch (e->kind) {
    case STIGI_EXPR_LIT_CHAR: return ETY_U8;
    case STIGI_EXPR_LIT_BOOL: return ETY_BOOL;
    case STIGI_EXPR_LIT_UNIT: return ETY_UNIT;
    case STIGI_EXPR_LOCAL:
    case STIGI_EXPR_GLOBAL: {
      const EmitVar *v = emit_find_var(ctx, e->u.ident.name);
      if (v != NULL) return v->scalar_ty;
      int32_t tgt = stigi_image_resolve_name(ctx->img, ctx->uidx,
                                             e->u.ident.name.ptr, e->u.ident.name.len);
      if (tgt >= 0) {
        const StigiDecl *d = ctx->img->units[tgt].ast->decl;
        if (d->kind == STIGI_DECL_VAL) {
          return ast_type_to_scalar(ctx->img, (uint32_t)tgt, d->u.val_decl.type, 0);
        }
        if (d->kind == STIGI_DECL_EXTERN && d->u.extern_decl.is_val) {
          return ast_type_to_scalar(ctx->img, (uint32_t)tgt, d->u.extern_decl.val_type, 0);
        }
      }
      return ETY_UNKNOWN;
    }
    case STIGI_EXPR_CALL: {
      if (e->u.call.callee->kind == STIGI_EXPR_LOCAL &&
          str_eq_c(e->u.call.callee->u.ident.name, "size") && e->u.call.type_arg_count == 1) {
        return ETY_U64;
      }
      if (e->u.call.callee->kind == STIGI_EXPR_LOCAL && e->u.call.arg_count == 1) {
        const EmitVar *av = emit_find_var(ctx, e->u.call.callee->u.ident.name);
        if (av != NULL && (av->is_array || av->is_cell)) return av->elem_scalar_ty;
      }
      if (e->u.call.callee->kind == STIGI_EXPR_LOCAL ||
          e->u.call.callee->kind == STIGI_EXPR_GLOBAL) {
        StigiStr cn = e->u.call.callee->u.ident.name;
        if (str_eq_c(cn, "Stigi.Test.expect") || str_eq_c(cn, "I32.ofU64")) return ETY_I32;
        if (str_eq_c(cn, "I64.ofU64")) return ETY_I64;
        if (str_eq_c(cn, "U8.ofU64")) return ETY_U8;
        if (str_eq_c(cn, "U32.ofU64") || str_eq_c(cn, "U32.wrappingAdd") ||
            str_eq_c(cn, "U32.wrappingSubtract") ||
            str_eq_c(cn, "U32.wrappingMultiply") || str_eq_c(cn, "U32.rotateRight") ||
            str_eq_c(cn, "U32.bitwiseXor") || str_eq_c(cn, "U32.bitwiseAnd") ||
            str_eq_c(cn, "U32.bitwiseOr") || str_eq_c(cn, "U32.shiftLeft") ||
            str_eq_c(cn, "U32.shiftRight")) {
          return ETY_U32;
        }
        if (str_eq_c(cn, "U64.ofU8") || str_eq_c(cn, "U64.ofU32") ||
            str_eq_c(cn, "U64.ofI64") || str_eq_c(cn, "U64.ofI32") ||
            str_eq_c(cn, "U64.bitwiseAnd") ||
            str_eq_c(cn, "U64.bitwiseOr") || str_eq_c(cn, "U64.bitwiseXor") ||
            str_eq_c(cn, "U64.shiftLeft") || str_eq_c(cn, "U64.shiftRight") ||
            str_eq_c(cn, "U64.wrappingAdd") || str_eq_c(cn, "U64.wrappingSubtract") ||
            str_eq_c(cn, "U64.wrappingMultiply") ||
            str_eq_c(cn, "U64.mix")) {
          return ETY_U64;
        }
        /* Capability operation call `binder.op(...)` */
        const char *dot = memchr(cn.ptr, '.', cn.len);
        if (dot != NULL) {
          StigiStr bname = {cn.ptr, (size_t)(dot - cn.ptr)};
          StigiStr opname = {dot + 1, cn.len - bname.len - 1U};
          const EmitVar *bv = emit_find_var(ctx, bname);
          if (bv != NULL && bv->handler_uidx >= 0) {
            const StigiDecl *hd = ctx->img->units[bv->handler_uidx].ast->decl;
            for (uint32_t m = 0; m < hd->u.handler_decl.def_count; m++) {
              const StigiDecl *md = hd->u.handler_decl.defs[m];
              if (md->kind == STIGI_DECL_DEF && str_eq_ss(md->name, opname)) {
                return ast_type_to_scalar(ctx->img, (uint32_t)bv->handler_uidx,
                                          md->u.def_decl.head.ret_type, 0);
              }
            }
          }
        }
        int32_t tgt = stigi_image_resolve_name(ctx->img, ctx->uidx, cn.ptr, cn.len);
        if (tgt >= 0) {
          const StigiDecl *d = ctx->img->units[tgt].ast->decl;
          if (d->kind == STIGI_DECL_DEF) {
            return ast_type_to_scalar(ctx->img, (uint32_t)tgt, d->u.def_decl.head.ret_type, 0);
          }
          if (d->kind == STIGI_DECL_EXTERN && !d->u.extern_decl.is_val) {
            return ast_type_to_scalar(ctx->img, (uint32_t)tgt, d->u.extern_decl.head.ret_type, 0);
          }
        }
      }
      return ETY_UNKNOWN;
    }
    case STIGI_EXPR_INDEX_READ: {
      if (e->u.index_expr.target->kind == STIGI_EXPR_LOCAL) {
        const EmitVar *av = emit_find_var(ctx, e->u.index_expr.target->u.ident.name);
        if (av != NULL && (av->is_array || av->is_cell)) return av->elem_scalar_ty;
      }
      return ETY_UNKNOWN;
    }
    case STIGI_EXPR_UNOP:
      if (e->u.unop.op == STIGI_OP_NOT) return ETY_BOOL;
      if (e->u.unop.op == STIGI_OP_NEG && e->u.unop.operand != NULL &&
          e->u.unop.operand->kind == STIGI_EXPR_LIT_INT) {
        return ETY_I64; /* ensures `-5 < 0` lowers as signed int64_t comparison */
      }
      return infer_expr_scalar(ctx, e->u.unop.operand);
    case STIGI_EXPR_BINOP:
      if (e->u.binop.op >= STIGI_OP_LT && e->u.binop.op <= STIGI_OP_OR) return ETY_BOOL;
      {
        EmitScalarTy lt = infer_expr_scalar(ctx, e->u.binop.lhs);
        if (lt != ETY_UNKNOWN) return lt;
        return infer_expr_scalar(ctx, e->u.binop.rhs);
      }
    case STIGI_EXPR_IF: {
      EmitScalarTy tt = infer_expr_scalar(ctx, e->u.if_expr.then_br);
      if (tt != ETY_UNKNOWN) return tt;
      return infer_expr_scalar(ctx, e->u.if_expr.else_br);
    }
    case STIGI_EXPR_MATCH:
      for (uint32_t i = 0; i < e->u.match_expr.arm_count; i++) {
        EmitScalarTy at = infer_expr_scalar(ctx, e->u.match_expr.arms[i].body);
        if (at != ETY_UNKNOWN) return at;
      }
      return ETY_UNKNOWN;
    case STIGI_EXPR_ASSERT:
      return infer_expr_scalar(ctx, e->u.assert_expr.expr);
    case STIGI_EXPR_BLOCK:
      if (e->u.block.count > 0) {
        return infer_expr_scalar(ctx, e->u.block.stmts[e->u.block.count - 1]);
      }
      return ETY_UNIT;
    case STIGI_EXPR_HANDLE:
      return infer_expr_scalar(ctx, e->u.handle_stmt.body);
    case STIGI_EXPR_WHILE:
    case STIGI_EXPR_VAL:
    case STIGI_EXPR_VAR:
    case STIGI_EXPR_ASSIGN:
    case STIGI_EXPR_INDEX_WRITE:
    case STIGI_EXPR_DENY:
    case STIGI_EXPR_LOCAL_DECL:
      return ETY_UNIT;
    default:
      return ETY_UNKNOWN;
  }
}

static int32_t infer_expr_enum_uidx(const EmitCtx *ctx, const StigiExpr *e) {
  if (e == NULL) return -1;
  if (e->kind == STIGI_EXPR_LOCAL || e->kind == STIGI_EXPR_GLOBAL) {
    if (e->kind == STIGI_EXPR_LOCAL) {
      const EmitVar *sv = emit_find_var(ctx, e->u.ident.name);
      if (sv != NULL) return sv->enum_uidx;
    }
    int32_t tgt = stigi_image_resolve_name(ctx->img, ctx->uidx,
                                           e->u.ident.name.ptr, e->u.ident.name.len);
    if (tgt >= 0) {
      const StigiDecl *d = ctx->img->units[tgt].ast->decl;
      if (d->kind == STIGI_DECL_VAL && d->u.val_decl.type != NULL) {
        uint32_t ru = (uint32_t)tgt;
        const StigiType *rt = resolve_typedef_type(ctx->img, &ru, d->u.val_decl.type, 0);
        if (rt != NULL && rt->kind == STIGI_TYPE_NAMED) {
          int32_t et = stigi_image_resolve_name(ctx->img, ru, rt->name.ptr, rt->name.len);
          if (et >= 0 && ctx->img->units[et].ast->decl->kind == STIGI_DECL_ENUM) return et;
        }
      }
      return -1;
    }
    return find_enum_case_owner_det(ctx->img, ctx->uidx, e->u.ident.name, NULL);
  }
  if (e->kind == STIGI_EXPR_CALL &&
      (e->u.call.callee->kind == STIGI_EXPR_LOCAL || e->u.call.callee->kind == STIGI_EXPR_GLOBAL)) {
    StigiStr cn = e->u.call.callee->u.ident.name;
    if (e->u.call.callee->kind == STIGI_EXPR_LOCAL && e->u.call.arg_count == 1) {
      const EmitVar *av = emit_find_var(ctx, cn);
      if (av != NULL && av->is_array && av->elem_enum_uidx >= 0) return av->elem_enum_uidx;
    }
    const char *dot = memchr(cn.ptr, '.', cn.len);
    if (dot != NULL) {
      StigiStr bname = {cn.ptr, (size_t)(dot - cn.ptr)};
      StigiStr opname = {dot + 1, cn.len - bname.len - 1U};
      const EmitVar *bv = emit_find_var(ctx, bname);
      if (bv != NULL && bv->handler_uidx >= 0) {
        const StigiDecl *hd = ctx->img->units[bv->handler_uidx].ast->decl;
        for (uint32_t m = 0; m < hd->u.handler_decl.def_count; m++) {
          const StigiDecl *md = hd->u.handler_decl.defs[m];
          if (md->kind == STIGI_DECL_DEF && str_eq_ss(md->name, opname) &&
              md->u.def_decl.head.ret_type != NULL) {
            uint32_t ru = (uint32_t)bv->handler_uidx;
            const StigiType *rt = resolve_typedef_type(ctx->img, &ru, md->u.def_decl.head.ret_type, 0);
            if (rt != NULL && rt->kind == STIGI_TYPE_NAMED) {
              int32_t et = stigi_image_resolve_name(ctx->img, ru, rt->name.ptr, rt->name.len);
              if (et >= 0 && ctx->img->units[et].ast->decl->kind == STIGI_DECL_ENUM) return et;
            }
          }
        }
      }
    }
    int32_t tgt = stigi_image_resolve_name(ctx->img, ctx->uidx, cn.ptr, cn.len);
    if (tgt >= 0) {
      const StigiDecl *d = ctx->img->units[tgt].ast->decl;
      const StigiType *rty = NULL;
      if (d->kind == STIGI_DECL_DEF) rty = d->u.def_decl.head.ret_type;
      else if (d->kind == STIGI_DECL_EXTERN && !d->u.extern_decl.is_val) rty = d->u.extern_decl.head.ret_type;
      if (rty != NULL) {
        uint32_t ru = (uint32_t)tgt;
        const StigiType *rt = resolve_typedef_type(ctx->img, &ru, rty, 0);
        if (rt != NULL && rt->kind == STIGI_TYPE_NAMED) {
          int32_t et = stigi_image_resolve_name(ctx->img, ru, rt->name.ptr, rt->name.len);
          if (et >= 0 && ctx->img->units[et].ast->decl->kind == STIGI_DECL_ENUM) return et;
        }
      }
      return -1;
    }
    return find_enum_case_owner_det(ctx->img, ctx->uidx, cn, NULL);
  }
  if (e->kind == STIGI_EXPR_INDEX_READ && e->u.index_expr.target != NULL &&
      e->u.index_expr.target->kind == STIGI_EXPR_LOCAL) {
    const EmitVar *av = emit_find_var(ctx, e->u.index_expr.target->u.ident.name);
    if (av != NULL && av->elem_enum_uidx >= 0) return av->elem_enum_uidx;
  }
  if (e->kind == STIGI_EXPR_IF) {
    int32_t et = infer_expr_enum_uidx(ctx, e->u.if_expr.then_br);
    if (et >= 0) return et;
    return infer_expr_enum_uidx(ctx, e->u.if_expr.else_br);
  }
  if (e->kind == STIGI_EXPR_MATCH) {
    for (uint32_t i = 0; i < e->u.match_expr.arm_count; i++) {
      int32_t et = infer_expr_enum_uidx(ctx, e->u.match_expr.arms[i].body);
      if (et >= 0) return et;
    }
  }
  if (e->kind == STIGI_EXPR_BLOCK && e->u.block.count > 0) {
    return infer_expr_enum_uidx(ctx, e->u.block.stmts[e->u.block.count - 1]);
  }
  if (e->kind == STIGI_EXPR_HANDLE) {
    return infer_expr_enum_uidx(ctx, e->u.handle_stmt.body);
  }
  return -1;
}

static bool emit_inferred_expr_c_type(EmitBuf *out, const EmitCtx *ctx, const StigiExpr *e,
                                      EmitScalarTy hint) {
  if (e != NULL && e->kind == STIGI_EXPR_LIT_STR) {
    eb_cstr(out, "uint8_t *");
    return true;
  }
  if (e != NULL && (e->kind == STIGI_EXPR_LOCAL || e->kind == STIGI_EXPR_GLOBAL)) {
    const EmitVar *v = emit_find_var(ctx, e->u.ident.name);
    if (v != NULL && v->ast_type != NULL) {
      emit_c_type(out, ctx->img, v->type_uidx, v->ast_type, 0);
      return true;
    }
  }
  int32_t euidx = infer_expr_enum_uidx(ctx, e);
  if (euidx >= 0) {
    eb_mangle_name(out, ctx->img->units[euidx].name, ctx->img->units[euidx].name_len);
    return true;
  }
  EmitScalarTy st = hint != ETY_UNKNOWN ? hint : infer_expr_scalar(ctx, e);
  if (st != ETY_UNKNOWN) {
    emit_scalar_c_type(out, st);
    return true;
  }
  eb_cstr(out, "uint64_t");
  return false;
}

static bool expr_is_complex_stmt(const StigiExpr *e) {
  if (e == NULL) return false;
  switch (e->kind) {
    case STIGI_EXPR_MATCH:
    case STIGI_EXPR_BLOCK:
    case STIGI_EXPR_HANDLE:
    case STIGI_EXPR_WHILE:
    case STIGI_EXPR_VAL:
    case STIGI_EXPR_VAR:
    case STIGI_EXPR_ASSIGN:
    case STIGI_EXPR_INDEX_WRITE:
      return true;
    case STIGI_EXPR_IF:
      return expr_is_complex_stmt(e->u.if_expr.then_br) ||
             expr_is_complex_stmt(e->u.if_expr.else_br);
    default:
      return false;
  }
}

/* Whether a statement ends in a C switch: a match, or a block or a branch whose last statement does. An arm of a tail
   loop whose body ends so gets a dead `break;`: every inner arm returns or continues, but a C compiler cannot see that a
   switch over an enum tag without a default covers every value, so it would report a fall-through. */
static bool stmt_ends_in_switch(const StigiExpr *e) {
  if (e == NULL) return false;
  switch (e->kind) {
    case STIGI_EXPR_MATCH:
      return true;
    case STIGI_EXPR_BLOCK:
      return e->u.block.count > 0 && stmt_ends_in_switch(e->u.block.stmts[e->u.block.count - 1U]);
    case STIGI_EXPR_IF:
      return stmt_ends_in_switch(e->u.if_expr.then_br) || stmt_ends_in_switch(e->u.if_expr.else_br);
    default:
      return false;
  }
}

static const StigiExpr *find_array_len_expr(const EmitCtx *ctx, const StigiExpr *target,
                                            EmitScalarTy *out_elem_ty) {
  if (out_elem_ty != NULL) *out_elem_ty = ETY_UNKNOWN;
  if (target == NULL) return NULL;
  if (target->kind == STIGI_EXPR_LOCAL) {
    const EmitVar *av = emit_find_var(ctx, target->u.ident.name);
    if (av != NULL && av->is_array) {
      if (out_elem_ty != NULL) *out_elem_ty = av->elem_scalar_ty;
      return av->len_expr;
    }
  }
  if (target->kind == STIGI_EXPR_GLOBAL || target->kind == STIGI_EXPR_LOCAL) {
    int32_t tgt = stigi_image_resolve_name(ctx->img, ctx->uidx,
                                           target->u.ident.name.ptr, target->u.ident.name.len);
    if (tgt >= 0) {
      const StigiDecl *d = ctx->img->units[tgt].ast->decl;
      if (d->kind == STIGI_DECL_VAL && d->u.val_decl.type != NULL) {
        uint32_t ru = (uint32_t)tgt;
        const StigiType *rt = resolve_typedef_type(ctx->img, &ru, d->u.val_decl.type, 0);
        if (rt != NULL && rt->kind == STIGI_TYPE_ARRAY) {
          if (out_elem_ty != NULL && rt->type_arg_count > 0) {
            *out_elem_ty = ast_type_to_scalar(ctx->img, ru, rt->type_args[0], 0);
          }
          if (rt->len_arg_count > 0) return rt->len_args[0];
        }
      }
    }
  }
  return NULL;
}

static void emit_expr_typed(EmitCtx *ctx, const StigiExpr *e, EmitScalarTy hint);
static void emit_stmt_or_ret(EmitCtx *ctx, const StigiExpr *e, bool is_tail, bool is_void);
static void emit_assign_to(EmitCtx *ctx, const StigiExpr *e, const char *lhs_var,
                           EmitScalarTy hint);

/* Helper: evaluate expression with its own pre-hoist buffer, flushing pre into ctx->b */
static void emit_expr_with_pre(EmitCtx *ctx, const StigiExpr *e, EmitScalarTy hint,
                               EmitBuf *out_expr) {
  EmitBuf local_pre;
  memset(&local_pre, 0, sizeof local_pre);
  memset(out_expr, 0, sizeof *out_expr);

  EmitBuf *saved_b = ctx->b;
  EmitBuf *saved_pre = ctx->pre;
  ctx->b = out_expr;
  ctx->pre = &local_pre;
  emit_expr_typed(ctx, e, hint);
  ctx->b = saved_b;
  ctx->pre = saved_pre;

  if (local_pre.len > 0) {
    eb_mem(ctx->b, local_pre.buf, local_pre.len);
  }
  if (local_pre.oom || out_expr->oom) ctx->b->oom = true;
  free(local_pre.buf);
}

/* Helper: hoist a single argument into ctx->pre for strict left-to-right call evaluation */
static uint32_t hoist_arg(EmitCtx *ctx, const StigiExpr *arg, const StigiType *param_ty,
                          uint32_t param_uidx, EmitScalarTy hint) {
  uint32_t tid = ctx->tmp_id++;
  EmitBuf val_buf;
  memset(&val_buf, 0, sizeof val_buf);
  EmitBuf *saved_b = ctx->b;
  ctx->b = &val_buf;
  emit_expr_typed(ctx, arg, hint);
  ctx->b = saved_b;

  eb_cstr(ctx->pre, "  ");
  if (param_ty != NULL) {
    emit_c_type(ctx->pre, ctx->img, param_uidx, param_ty, 0);
  } else {
    (void)emit_inferred_expr_c_type(ctx->pre, ctx, arg, hint);
  }
  eb_fmt(ctx->pre, " _a%u = ", tid);
  eb_mem(ctx->pre, val_buf.buf, val_buf.len);
  eb_cstr(ctx->pre, ";\n");
  if (val_buf.oom) ctx->pre->oom = true;
  free(val_buf.buf);
  return tid;
}

/* Helper: emit handler state setup for `handle binder = Handler(args...)` */
static void emit_handle_setup(EmitCtx *ctx, const StigiExpr *e) {
  if (e == NULL || e->kind != STIGI_EXPR_HANDLE) return;
  int32_t hidx = stigi_image_resolve_name(ctx->img, ctx->uidx,
                                          e->u.handle_stmt.handler_name.ptr,
                                          e->u.handle_stmt.handler_name.len);
  if (hidx < 0 || ctx->img->units[hidx].ast->decl->kind != STIGI_DECL_HANDLER) return;
  const StigiImageUnit *hu = &ctx->img->units[hidx];
  const StigiDecl *hd = hu->ast->decl;

  uint32_t hid = ctx->tmp_id++;
  char sname[32];
  (void)snprintf(sname, sizeof sname, "_hs%u", hid);

  uint32_t argc = e->u.handle_stmt.arg_count;
  EmitBuf *abufs = argc > 0 ? calloc(argc, sizeof *abufs) : NULL;
  for (uint32_t i = 0; i < argc && abufs != NULL; i++) {
    EmitScalarTy ph = (i < hd->u.handler_decl.val_param_count)
                          ? ast_type_to_scalar(ctx->img, (uint32_t)hidx,
                                               hd->u.handler_decl.val_params[i].type, 0)
                          : ETY_UNKNOWN;
    emit_expr_with_pre(ctx, e->u.handle_stmt.args[i], ph, &abufs[i]);
  }
  eb_cstr(ctx->b, "  ");
  eb_mangle_name(ctx->b, hu->name, hu->name_len);
  eb_fmt(ctx->b, "_State %s = ", sname);
  eb_mangle_name(ctx->b, hu->name, hu->name_len);
  eb_cstr(ctx->b, "_init(");
  for (uint32_t i = 0; i < argc && abufs != NULL; i++) {
    if (i > 0) eb_cstr(ctx->b, ", ");
    eb_mem(ctx->b, abufs[i].buf, abufs[i].len);
    free(abufs[i].buf);
  }
  free(abufs);
  eb_cstr(ctx->b, ");\n");
  eb_fmt(ctx->b, "  (void)%s;\n", sname);

  int32_t cap_uidx = stigi_image_resolve_name(ctx->img, (uint32_t)hidx,
                                              hd->u.handler_decl.for_cap.ptr,
                                              hd->u.handler_decl.for_cap.len);
  char chname[32] = {0};
  if (cap_uidx >= 0 && cap_has_runtime_handle(ctx->img, (uint32_t)cap_uidx)) {
    int32_t tag = find_handler_tag_for_cap(ctx->img, (uint32_t)cap_uidx, (uint32_t)hidx);
    (void)snprintf(chname, sizeof chname, "_ch%u", hid);
    eb_cstr(ctx->b, "  ");
    eb_mangle_name(ctx->b, ctx->img->units[cap_uidx].name, ctx->img->units[cap_uidx].name_len);
    eb_fmt(ctx->b, "_Handle %s = { .tag = %dU, .ptr = { .h%d = &%s } };\n  (void)%s;\n",
           chname, tag >= 0 ? tag : 0, tag >= 0 ? tag : 0, sname, chname);
  }

  EmitVar *hv = emit_push_var_raw(ctx, e->u.handle_stmt.binder,
                                  e->u.handle_stmt.binder.len > 0);
  if (hv != NULL) {
    hv->handler_uidx = hidx;
    hv->cap_uidx = cap_uidx;
    size_t sl = strlen(sname);
    hv->handler_state_var = malloc(sl + 1U);
    if (hv->handler_state_var != NULL) memcpy(hv->handler_state_var, sname, sl + 1U);
    if (chname[0] != '\0') {
      size_t chl = strlen(chname);
      hv->cap_handle_var = malloc(chl + 1U);
      if (hv->cap_handle_var != NULL) memcpy(hv->cap_handle_var, chname, chl + 1U);
    }
  }
}

/* Helper: detect Array.tryFill / Array.tryGrow / Cell.tryMake scrutinee */
static int detect_alloc_scrutinee(const StigiExpr *scr) {
  if (scr == NULL || scr->kind != STIGI_EXPR_CALL) return 0;
  if (scr->u.call.callee->kind != STIGI_EXPR_GLOBAL &&
      scr->u.call.callee->kind != STIGI_EXPR_LOCAL) {
    return 0;
  }
  StigiStr cn = scr->u.call.callee->u.ident.name;
  if (str_eq_c(cn, "Array.tryFill") && scr->u.call.arg_count == 2) return 1;
  if (str_eq_c(cn, "Array.tryGrow") && scr->u.call.arg_count == 3) return 2;
  if (str_eq_c(cn, "Cell.tryMake") && scr->u.call.arg_count == 1) return 3;
  return 0;
}

static void emit_alloc_elem_ctype(EmitBuf *b, const EmitCtx *ctx, const StigiExpr *scr, int akind) {
  if (scr->u.call.type_arg_count > 0) {
    emit_c_type(b, ctx->img, ctx->uidx, scr->u.call.type_args[0], 0);
    return;
  }
  if (akind == 2) {
    /* Array.tryGrow(arr, nextCap, fill): element type from arr */
    if (scr->u.call.args[0]->kind == STIGI_EXPR_LOCAL) {
      const EmitVar *av = emit_find_var(ctx, scr->u.call.args[0]->u.ident.name);
      if (av != NULL && av->ast_type != NULL) {
        uint32_t ru = av->type_uidx;
        const StigiType *rt = resolve_typedef_type(ctx->img, &ru, av->ast_type, 0);
        if (rt != NULL && rt->kind == STIGI_TYPE_ARRAY && rt->type_arg_count > 0) {
          emit_c_type(b, ctx->img, ru, rt->type_args[0], 0);
          return;
        }
      }
    }
    (void)emit_inferred_expr_c_type(b, ctx, scr->u.call.args[2], ETY_UNKNOWN);
    return;
  }
  const StigiExpr *init_e = (akind == 1) ? scr->u.call.args[1] : scr->u.call.args[0];
  (void)emit_inferred_expr_c_type(b, ctx, init_e, ETY_UNKNOWN);
}

/* Conservatively returns false ONLY when `binder` is used strictly for element read/write
 * inside `e` (never returned, aliased, stored, or passed to a call) — 100% sound for stack
 * promotion and deterministic arm-exit free without risking UAF or double-free. */
static bool alloc_binder_escapes(const StigiExpr *e, StigiStr binder) {
  if (e == NULL || binder.len == 0) return false;
  switch (e->kind) {
    case STIGI_EXPR_LOCAL:
    case STIGI_EXPR_GLOBAL:
      return str_eq_ss(e->u.ident.name, binder);
    case STIGI_EXPR_INDEX_READ:
      if (e->u.index_expr.target->kind == STIGI_EXPR_LOCAL &&
          str_eq_ss(e->u.index_expr.target->u.ident.name, binder)) {
        return alloc_binder_escapes(e->u.index_expr.index, binder);
      }
      return alloc_binder_escapes(e->u.index_expr.target, binder) ||
             alloc_binder_escapes(e->u.index_expr.index, binder);
    case STIGI_EXPR_INDEX_WRITE:
      if (e->u.index_expr.target->kind == STIGI_EXPR_LOCAL &&
          str_eq_ss(e->u.index_expr.target->u.ident.name, binder)) {
        return alloc_binder_escapes(e->u.index_expr.index, binder) ||
               alloc_binder_escapes(e->u.index_expr.value, binder);
      }
      return alloc_binder_escapes(e->u.index_expr.target, binder) ||
             alloc_binder_escapes(e->u.index_expr.index, binder) ||
             alloc_binder_escapes(e->u.index_expr.value, binder);
    case STIGI_EXPR_CALL:
      if (e->u.call.callee->kind == STIGI_EXPR_LOCAL &&
          str_eq_ss(e->u.call.callee->u.ident.name, binder) && e->u.call.arg_count == 1) {
        return alloc_binder_escapes(e->u.call.args[0], binder);
      }
      if (alloc_binder_escapes(e->u.call.callee, binder)) return true;
      for (uint32_t i = 0; i < e->u.call.arg_count; i++) {
        if (alloc_binder_escapes(e->u.call.args[i], binder)) return true;
      }
      return false;
    case STIGI_EXPR_UNOP:
      return alloc_binder_escapes(e->u.unop.operand, binder);
    case STIGI_EXPR_BINOP:
      return alloc_binder_escapes(e->u.binop.lhs, binder) ||
             alloc_binder_escapes(e->u.binop.rhs, binder);
    case STIGI_EXPR_IF:
      return alloc_binder_escapes(e->u.if_expr.cond, binder) ||
             alloc_binder_escapes(e->u.if_expr.then_br, binder) ||
             alloc_binder_escapes(e->u.if_expr.else_br, binder);
    case STIGI_EXPR_WHILE:
      return alloc_binder_escapes(e->u.while_expr.cond, binder) ||
             alloc_binder_escapes(e->u.while_expr.body, binder);
    case STIGI_EXPR_BLOCK:
      for (uint32_t i = 0; i < e->u.block.count; i++) {
        if (alloc_binder_escapes(e->u.block.stmts[i], binder)) return true;
      }
      return false;
    case STIGI_EXPR_VAL:
      if (alloc_binder_escapes(e->u.val_stmt.init, binder)) return true;
      if (e->u.val_stmt.pat.kind == STIGI_PAT_VAR && str_eq_ss(e->u.val_stmt.pat.name, binder)) {
        return false;
      }
      return false;
    case STIGI_EXPR_VAR:
      return alloc_binder_escapes(e->u.var_stmt.init, binder);
    case STIGI_EXPR_ASSIGN:
      return alloc_binder_escapes(e->u.assign_stmt.value, binder);
    case STIGI_EXPR_MATCH:
      if (alloc_binder_escapes(e->u.match_expr.scrutinee, binder)) return true;
      for (uint32_t i = 0; i < e->u.match_expr.arm_count; i++) {
        if (alloc_binder_escapes(e->u.match_expr.arms[i].body, binder)) return true;
      }
      return false;
    case STIGI_EXPR_HANDLE:
      for (uint32_t i = 0; i < e->u.handle_stmt.arg_count; i++) {
        if (alloc_binder_escapes(e->u.handle_stmt.args[i], binder)) return true;
      }
      return alloc_binder_escapes(e->u.handle_stmt.body, binder);
    case STIGI_EXPR_ASSERT:
      return alloc_binder_escapes(e->u.assert_expr.expr, binder);
    default:
      return false;
  }
}

static uint64_t alloc_elem_size_bytes(const EmitCtx *ctx, const StigiExpr *scr, int akind) {
  if (scr->u.call.type_arg_count > 0) {
    StigiLayout l = ast_type_layout(ctx->img, ctx->uidx, scr->u.call.type_args[0], 0);
    return l.size > 0 ? l.size : 8ULL;
  }
  const StigiExpr *init_e = (akind == 1) ? scr->u.call.args[1]
                          : (akind == 2) ? scr->u.call.args[2]
                                         : scr->u.call.args[0];
  EmitScalarTy st = infer_expr_scalar(ctx, init_e);
  if (st == ETY_U8 || st == ETY_BOOL) return 1ULL;
  if (st == ETY_U32 || st == ETY_I32) return 4ULL;
  return 8ULL;
}

static void emit_alloc_arm_with_cleanup(EmitCtx *ctx, const StigiExpr *body,
                                        const char *ptr_name, bool free_always,
                                        bool free_filc_only, const char *lhs_var,
                                        EmitScalarTy hint, bool is_tail, bool is_void,
                                        uint32_t sid) {
  if (!free_always && !free_filc_only) {
    if (lhs_var != NULL) emit_assign_to(ctx, body, lhs_var, hint);
    else emit_stmt_or_ret(ctx, body, is_tail, is_void);
    return;
  }
  if (lhs_var != NULL) {
    emit_assign_to(ctx, body, lhs_var, hint);
    if (free_filc_only) eb_cstr(ctx->b, "#if defined(__FILC__)\n");
    eb_fmt(ctx->b, "    free(%s);\n", ptr_name);
    if (free_filc_only) eb_cstr(ctx->b, "#endif\n");
    return;
  }
  if (is_void) {
    emit_stmt_or_ret(ctx, body, false, true);
    if (free_filc_only) eb_cstr(ctx->b, "#if defined(__FILC__)\n");
    eb_fmt(ctx->b, "    free(%s);\n", ptr_name);
    if (free_filc_only) eb_cstr(ctx->b, "#endif\n");
    if (is_tail) eb_cstr(ctx->b, "    return;\n");
    return;
  }
  if (is_tail) {
    char rvar[32];
    (void)snprintf(rvar, sizeof rvar, "_ar%u", sid);
    eb_cstr(ctx->b, "    ");
    if (ctx->ret_ast_type != NULL) {
      emit_c_type(ctx->b, ctx->img, ctx->ret_type_uidx, ctx->ret_ast_type, 0);
    } else {
      (void)emit_inferred_expr_c_type(ctx->b, ctx, body, hint);
    }
    eb_fmt(ctx->b, " %s = {0};\n", rvar);
    emit_assign_to(ctx, body, rvar, ctx->ret_scalar != ETY_UNKNOWN ? ctx->ret_scalar : hint);
    if (free_filc_only) eb_cstr(ctx->b, "#if defined(__FILC__)\n");
    eb_fmt(ctx->b, "    free(%s);\n", ptr_name);
    if (free_filc_only) eb_cstr(ctx->b, "#endif\n");
    eb_fmt(ctx->b, "    return %s;\n", rvar);
    return;
  }
  emit_stmt_or_ret(ctx, body, false, false);
  if (free_filc_only) eb_cstr(ctx->b, "#if defined(__FILC__)\n");
  eb_fmt(ctx->b, "    free(%s);\n", ptr_name);
  if (free_filc_only) eb_cstr(ctx->b, "#endif\n");
}

static void emit_alloc_match(EmitCtx *ctx, const StigiExpr *e, int akind,
                             const char *lhs_var, EmitScalarTy hint,
                             bool is_tail, bool is_void) {
  const StigiExpr *scr = e->u.match_expr.scrutinee;
  const StigiImageUnit *u = &ctx->img->units[ctx->uidx];
  uint32_t sid = ctx->tmp_id++;

  EmitBuf elem_ct;
  memset(&elem_ct, 0, sizeof elem_ct);
  emit_alloc_elem_ctype(&elem_ct, ctx, scr, akind);

  eb_line_dir(ctx->b, e->span.line, u->name, u->name_len);
  if (akind == 1) {
    /* Array.tryFill{region}(cap, init) */
    const StigiMatchArm *filled_arm = NULL;
    const StigiMatchArm *fail_arm = NULL;
    for (uint32_t a = 0; a < e->u.match_expr.arm_count; a++) {
      const StigiPat *pat = &e->u.match_expr.arms[a].pat;
      if (str_eq_c(pat->name, "Filled")) filled_arm = &e->u.match_expr.arms[a];
      else if (str_eq_c(pat->name, "FillFailed")) fail_arm = &e->u.match_expr.arms[a];
    }
    bool escapes = true;
    StigiStr binder = {NULL, 0};
    if (filled_arm != NULL) {
      if (filled_arm->pat.field_count == 1 &&
          !(filled_arm->pat.fields[0].len == 1 && filled_arm->pat.fields[0].ptr[0] == '_')) {
        binder = filled_arm->pat.fields[0];
        escapes = alloc_binder_escapes(filled_arm->body, binder);
      } else {
        escapes = false;
      }
    }
    bool stack_promote = false;
    uint64_t stack_n = 0;
    uint64_t stack_bytes = 0;
    if (!escapes && scr->u.call.args[0]->kind == STIGI_EXPR_LIT_INT) {
      uint64_t n = scr->u.call.args[0]->u.lit_int.value;
      uint64_t esz = alloc_elem_size_bytes(ctx, scr, akind);
      if (n > 0 && n <= 256ULL && n * esz <= 256ULL) {
        stack_promote = true;
        stack_n = n;
        stack_bytes = n * esz;
        if (ctx->stats != NULL) ctx->stats->stack_objects++;
      }
    }
    if (!escapes && !stack_promote && ctx->stats != NULL) {
      ctx->stats->free_at_exit++;
    }

    EmitBuf cap_b, init_b;
    emit_expr_with_pre(ctx, scr->u.call.args[0], ETY_U64, &cap_b);
    emit_expr_with_pre(ctx, scr->u.call.args[1], ETY_UNKNOWN, &init_b);
    eb_fmt(ctx->b, "  uint64_t _alen%u = (uint64_t)(", sid);
    eb_mem(ctx->b, cap_b.buf, cap_b.len);
    eb_cstr(ctx->b, ");\n  ");
    eb_mem(ctx->b, elem_ct.buf, elem_ct.len);
    if (emit_is_c_scalar_type(elem_ct.buf, elem_ct.len)) {
      eb_fmt(ctx->b, " _iv%u = (", sid);
      eb_mem(ctx->b, elem_ct.buf, elem_ct.len);
      eb_cstr(ctx->b, ")(");
      eb_mem(ctx->b, init_b.buf, init_b.len);
      eb_cstr(ctx->b, ");\n");
    } else {
      eb_fmt(ctx->b, " _iv%u = (", sid);
      eb_mem(ctx->b, init_b.buf, init_b.len);
      eb_cstr(ctx->b, ");\n");
    }
    if (stack_promote) {
      /* memory.md 1 & 8 + S47: 64B cache-aligned stack object in release; per-object heap in Fil-C */
      eb_cstr(ctx->b, "#if defined(__FILC__)\n  ");
      eb_mem(ctx->b, elem_ct.buf, elem_ct.len);
      eb_fmt(ctx->b, " *_arr%u = (", sid);
      eb_mem(ctx->b, elem_ct.buf, elem_ct.len);
      eb_fmt(ctx->b, " *)stigi_rt_try_alloc(_alen%u, sizeof(", sid);
      eb_mem(ctx->b, elem_ct.buf, elem_ct.len);
      eb_fmt(ctx->b, "));\n  if (_arr%u != NULL) {\n#else\n  %s",
             sid, stack_bytes >= 64ULL ? "_Alignas(64) " : (stack_bytes >= 16ULL ? "_Alignas(16) " : ""));
      eb_mem(ctx->b, elem_ct.buf, elem_ct.len);
      eb_fmt(ctx->b, " _sarr%u[%llu];\n  ", sid, (unsigned long long)stack_n);
      eb_mem(ctx->b, elem_ct.buf, elem_ct.len);
      eb_fmt(ctx->b, " *_arr%u = _sarr%u;\n  {\n#endif\n"
                     "    for (uint64_t _fi%u = 0; _fi%u < _alen%u; _fi%u++) _arr%u[_fi%u] = _iv%u;\n",
             sid, sid, sid, sid, sid, sid, sid, sid, sid);
    } else {
      eb_cstr(ctx->b, "  ");
      eb_mem(ctx->b, elem_ct.buf, elem_ct.len);
      eb_fmt(ctx->b, " *_arr%u = (", sid);
      eb_mem(ctx->b, elem_ct.buf, elem_ct.len);
      eb_fmt(ctx->b, " *)stigi_rt_try_alloc(_alen%u, sizeof(", sid);
      eb_mem(ctx->b, elem_ct.buf, elem_ct.len);
      eb_fmt(ctx->b, "));\n  if (_arr%u != NULL) {\n"
                     "    for (uint64_t _fi%u = 0; _fi%u < _alen%u; _fi%u++) _arr%u[_fi%u] = _iv%u;\n",
             sid, sid, sid, sid, sid, sid, sid, sid);
    }
    free(cap_b.buf);
    free(init_b.buf);

    if (filled_arm != NULL) {
      uint32_t saved = ctx->var_count;
      if (binder.len > 0) {
        EmitVar *v = emit_push_var_raw(ctx, binder, true);
        if (v != NULL) {
          v->is_array = true;
          v->len_expr = scr->u.call.args[0];
          v->elem_enum_uidx = infer_expr_enum_uidx(ctx, scr->u.call.args[1]);
          if (v->elem_enum_uidx < 0 && scr->u.call.type_arg_count > 0 &&
              scr->u.call.type_args[0] != NULL) {
            uint32_t eru = ctx->uidx;
            const StigiType *erty = resolve_typedef_type(ctx->img, &eru, scr->u.call.type_args[0], 0);
            if (erty != NULL && erty->kind == STIGI_TYPE_NAMED) {
              int32_t eet = stigi_image_resolve_name(ctx->img, eru, erty->name.ptr, erty->name.len);
              if (eet >= 0 && ctx->img->units[eet].ast->decl->kind == STIGI_DECL_ENUM) {
                v->elem_enum_uidx = eet;
              }
            }
          }
          /* the binder carries its element type like every array variable (a generic call infers A from it) */
          if (v->elem_enum_uidx < 0) {
            uint32_t sru = ctx->uidx;
            const StigiType *sty = scr->u.call.type_arg_count > 0 && scr->u.call.type_args[0] != NULL
                                       ? resolve_typedef_type(ctx->img, &sru, scr->u.call.type_args[0], 0)
                                       : NULL;
            v->elem_scalar_ty = sty != NULL ? ast_type_to_scalar(ctx->img, sru, sty, 0)
                                            : infer_expr_scalar(ctx, scr->u.call.args[1]);
          }
          eb_cstr(ctx->b, "    ");
          eb_mem(ctx->b, elem_ct.buf, elem_ct.len);
          eb_cstr(ctx->b, " *");
          emit_var_cname(ctx->b, v);
          eb_fmt(ctx->b, " = _arr%u;\n", sid);
        }
      }
      char ptrn[32];
      (void)snprintf(ptrn, sizeof ptrn, "_arr%u", sid);
      emit_alloc_arm_with_cleanup(ctx, filled_arm->body, ptrn,
                                  !escapes && !stack_promote,
                                  !escapes && stack_promote,
                                  lhs_var, hint, is_tail, is_void, sid);
      emit_ctx_pop_vars(ctx, saved);
    }
    if (stack_promote) {
      eb_cstr(ctx->b, "  }\n#if defined(__FILC__)\n  else {\n");
    } else {
      eb_cstr(ctx->b, "  } else {\n");
    }
    if (fail_arm != NULL) {
      uint32_t saved = ctx->var_count;
      if (lhs_var != NULL) emit_assign_to(ctx, fail_arm->body, lhs_var, hint);
      else emit_stmt_or_ret(ctx, fail_arm->body, is_tail, is_void);
      emit_ctx_pop_vars(ctx, saved);
    }
    if (stack_promote) {
      eb_cstr(ctx->b, "  }\n#endif\n");
    } else {
      eb_cstr(ctx->b, "  }\n");
    }
  } else if (akind == 2) {
    /* Array.tryGrow{region}(oldArr, nextCap, fill) */
    const StigiExpr *old_len_e = find_array_len_expr(ctx, scr->u.call.args[0], NULL);
    EmitBuf old_b, cap_b, fill_b;
    emit_expr_with_pre(ctx, scr->u.call.args[0], ETY_UNKNOWN, &old_b);
    emit_expr_with_pre(ctx, scr->u.call.args[1], ETY_U64, &cap_b);
    emit_expr_with_pre(ctx, scr->u.call.args[2], ETY_UNKNOWN, &fill_b);
    eb_cstr(ctx->b, "  ");
    eb_mem(ctx->b, elem_ct.buf, elem_ct.len);
    eb_fmt(ctx->b, " *_old%u = ", sid);
    eb_mem(ctx->b, old_b.buf, old_b.len);
    eb_fmt(ctx->b, ";\n  uint64_t _ncap%u = (uint64_t)(", sid);
    eb_mem(ctx->b, cap_b.buf, cap_b.len);
    eb_cstr(ctx->b, ");\n  uint64_t _ocap");
    eb_fmt(ctx->b, "%u = ", sid);
    if (old_len_e != NULL) {
      eb_cstr(ctx->b, "(uint64_t)(");
      EmitBuf olen_b;
      emit_expr_with_pre(ctx, old_len_e, ETY_U64, &olen_b);
      eb_mem(ctx->b, olen_b.buf, olen_b.len);
      free(olen_b.buf);
      eb_cstr(ctx->b, ")");
    } else {
      /* an unknown old length never becomes 0: that would drop every old element */
      ctx->fail_closed = true;
      (void)snprintf(ctx->fail_msg, sizeof ctx->fail_msg,
                     "cannot emit Array.tryGrow: missing array length expression in '%s'",
                     ctx->img->units[ctx->uidx].name);
    }
    eb_cstr(ctx->b, ";\n  ");
    eb_mem(ctx->b, elem_ct.buf, elem_ct.len);
    if (emit_is_c_scalar_type(elem_ct.buf, elem_ct.len)) {
      eb_fmt(ctx->b, " _fv%u = (", sid);
      eb_mem(ctx->b, elem_ct.buf, elem_ct.len);
      eb_cstr(ctx->b, ")(");
      eb_mem(ctx->b, fill_b.buf, fill_b.len);
      eb_cstr(ctx->b, ");\n  ");
    } else {
      eb_fmt(ctx->b, " _fv%u = (", sid);
      eb_mem(ctx->b, fill_b.buf, fill_b.len);
      eb_cstr(ctx->b, ");\n  ");
    }
    eb_mem(ctx->b, elem_ct.buf, elem_ct.len);
    eb_fmt(ctx->b, " *_new%u = (", sid);
    eb_mem(ctx->b, elem_ct.buf, elem_ct.len);
    eb_fmt(ctx->b, " *)stigi_rt_try_alloc(_ncap%u, sizeof(", sid);
    eb_mem(ctx->b, elem_ct.buf, elem_ct.len);
    eb_fmt(ctx->b, "));\n  if (_new%u != NULL) {\n"
                   "    uint64_t _cp%u = _ocap%u < _ncap%u ? _ocap%u : _ncap%u;\n"
                   "    for (uint64_t _gi%u = 0; _gi%u < _cp%u; _gi%u++) _new%u[_gi%u] = _old%u[_gi%u];\n"
                   "    for (uint64_t _gi%u = _cp%u; _gi%u < _ncap%u; _gi%u++) _new%u[_gi%u] = _fv%u;\n"
                   "    free(_old%u);\n",
           sid, sid, sid, sid, sid, sid, sid, sid, sid, sid, sid, sid, sid, sid,
           sid, sid, sid, sid, sid, sid, sid, sid, sid);
    free(old_b.buf);
    free(cap_b.buf);
    free(fill_b.buf);

    for (uint32_t a = 0; a < e->u.match_expr.arm_count; a++) {
      const StigiPat *pat = &e->u.match_expr.arms[a].pat;
      if (str_eq_c(pat->name, "Grown")) {
        uint32_t saved = ctx->var_count;
        StigiStr binder = {NULL, 0};
        if (pat->field_count == 1 && !(pat->fields[0].len == 1 && pat->fields[0].ptr[0] == '_')) {
          binder = pat->fields[0];
          EmitVar *v = emit_push_var_raw(ctx, binder, true);
          if (v != NULL) {
            v->is_array = true;
            v->len_expr = scr->u.call.args[1];
            v->elem_enum_uidx = infer_expr_enum_uidx(ctx, scr->u.call.args[2]);
            eb_cstr(ctx->b, "    ");
            eb_mem(ctx->b, elem_ct.buf, elem_ct.len);
            eb_cstr(ctx->b, " *");
            emit_var_cname(ctx->b, v);
            eb_fmt(ctx->b, " = _new%u;\n", sid);
          }
        }
        bool escapes = binder.len > 0 && alloc_binder_escapes(e->u.match_expr.arms[a].body, binder);
        if (!escapes && ctx->stats != NULL) ctx->stats->free_at_exit++;
        char ptrn[32];
        (void)snprintf(ptrn, sizeof ptrn, "_new%u", sid);
        emit_alloc_arm_with_cleanup(ctx, e->u.match_expr.arms[a].body, ptrn,
                                    !escapes, false, lhs_var, hint, is_tail, is_void, sid);
        emit_ctx_pop_vars(ctx, saved);
        break;
      }
    }
    eb_cstr(ctx->b, "  } else {\n");
    for (uint32_t a = 0; a < e->u.match_expr.arm_count; a++) {
      const StigiPat *pat = &e->u.match_expr.arms[a].pat;
      if (str_eq_c(pat->name, "GrowFailed")) {
        uint32_t saved = ctx->var_count;
        if (pat->field_count == 1 && !(pat->fields[0].len == 1 && pat->fields[0].ptr[0] == '_')) {
          EmitVar *v = emit_push_var_raw(ctx, pat->fields[0], true);
          if (v != NULL) {
            v->is_array = true;
            v->len_expr = old_len_e;
            eb_cstr(ctx->b, "    ");
            eb_mem(ctx->b, elem_ct.buf, elem_ct.len);
            eb_cstr(ctx->b, " *");
            emit_var_cname(ctx->b, v);
            eb_fmt(ctx->b, " = _old%u;\n", sid);
          }
        }
        if (lhs_var != NULL) emit_assign_to(ctx, e->u.match_expr.arms[a].body, lhs_var, hint);
        else emit_stmt_or_ret(ctx, e->u.match_expr.arms[a].body, is_tail, is_void);
        emit_ctx_pop_vars(ctx, saved);
        break;
      }
    }
    eb_cstr(ctx->b, "  }\n");
  } else {
    /* Cell.tryMake{region}(init) */
    EmitBuf init_b;
    emit_expr_with_pre(ctx, scr->u.call.args[0], ETY_UNKNOWN, &init_b);
    eb_cstr(ctx->b, "  ");
    eb_mem(ctx->b, elem_ct.buf, elem_ct.len);
    eb_fmt(ctx->b, " *_cell%u = (", sid);
    eb_mem(ctx->b, elem_ct.buf, elem_ct.len);
    eb_fmt(ctx->b, " *)stigi_rt_try_alloc(1U, sizeof(");
    eb_mem(ctx->b, elem_ct.buf, elem_ct.len);
    eb_fmt(ctx->b, "));\n  if (_cell%u != NULL) {\n    *_cell%u = (", sid, sid);
    eb_mem(ctx->b, elem_ct.buf, elem_ct.len);
    eb_cstr(ctx->b, ")(");
    eb_mem(ctx->b, init_b.buf, init_b.len);
    eb_cstr(ctx->b, ");\n");
    free(init_b.buf);
    for (uint32_t a = 0; a < e->u.match_expr.arm_count; a++) {
      const StigiPat *pat = &e->u.match_expr.arms[a].pat;
      if (str_eq_c(pat->name, "Filled")) {
        uint32_t saved = ctx->var_count;
        StigiStr binder = {NULL, 0};
        if (pat->field_count == 1 && !(pat->fields[0].len == 1 && pat->fields[0].ptr[0] == '_')) {
          binder = pat->fields[0];
          EmitVar *v = emit_push_var_raw(ctx, binder, true);
          if (v != NULL) {
            v->is_cell = true;
            eb_cstr(ctx->b, "    ");
            eb_mem(ctx->b, elem_ct.buf, elem_ct.len);
            eb_cstr(ctx->b, " *");
            emit_var_cname(ctx->b, v);
            eb_fmt(ctx->b, " = _cell%u;\n", sid);
          }
        }
        bool escapes = binder.len > 0 && alloc_binder_escapes(e->u.match_expr.arms[a].body, binder);
        if (!escapes && ctx->stats != NULL) ctx->stats->free_at_exit++;
        char ptrn[32];
        (void)snprintf(ptrn, sizeof ptrn, "_cell%u", sid);
        emit_alloc_arm_with_cleanup(ctx, e->u.match_expr.arms[a].body, ptrn,
                                    !escapes, false, lhs_var, hint, is_tail, is_void, sid);
        emit_ctx_pop_vars(ctx, saved);
        break;
      }
    }
    eb_cstr(ctx->b, "  } else {\n");
    for (uint32_t a = 0; a < e->u.match_expr.arm_count; a++) {
      const StigiPat *pat = &e->u.match_expr.arms[a].pat;
      if (str_eq_c(pat->name, "FillFailed")) {
        uint32_t saved = ctx->var_count;
        if (lhs_var != NULL) emit_assign_to(ctx, e->u.match_expr.arms[a].body, lhs_var, hint);
        else emit_stmt_or_ret(ctx, e->u.match_expr.arms[a].body, is_tail, is_void);
        emit_ctx_pop_vars(ctx, saved);
        break;
      }
    }
    eb_cstr(ctx->b, "  }\n");
  }
  free(elem_ct.buf);
}

/* Unified match emitter supporting alloc results, nullable-ptr enums, tail-tag enums,
 * standard enums, Bool, and integer match — with ZERO __builtin_unreachable() */
static void emit_match_core(EmitCtx *ctx, const StigiExpr *e, const char *lhs_var,
                            EmitScalarTy hint, bool is_tail, bool is_void) {
  int akind = detect_alloc_scrutinee(e->u.match_expr.scrutinee);
  if (akind > 0) {
    emit_alloc_match(ctx, e, akind, lhs_var, hint, is_tail, is_void);
    if (lhs_var == NULL && is_tail && is_void && ctx->is_tail_loop) eb_cstr(ctx->b, "  return;\n");
    return;
  }

  const StigiImageUnit *u = &ctx->img->units[ctx->uidx];
  int32_t enum_uidx = infer_expr_enum_uidx(ctx, e->u.match_expr.scrutinee);
  if (enum_uidx < 0 && e->u.match_expr.arm_count > 0 &&
      e->u.match_expr.arms[0].pat.kind == STIGI_PAT_CTOR) {
    enum_uidx = find_enum_case_owner_det(ctx->img, ctx->uidx,
                                         e->u.match_expr.arms[0].pat.name, NULL);
  }

  if (enum_uidx >= 0) {
    const StigiDecl *ed = ctx->img->units[enum_uidx].ast->decl;
    StigiEnumLayoutInfo eli = stigi_enum_layout_info(ctx->img, (uint32_t)enum_uidx, NULL, 0);
    uint32_t sid = ctx->tmp_id++;
    EmitBuf scr_buf;
    emit_expr_with_pre(ctx, e->u.match_expr.scrutinee, ETY_UNKNOWN, &scr_buf);
    eb_line_dir(ctx->b, e->span.line, u->name, u->name_len);
    eb_cstr(ctx->b, "  ");
    eb_mangle_name(ctx->b, ctx->img->units[enum_uidx].name, ctx->img->units[enum_uidx].name_len);
    eb_fmt(ctx->b, " _sc%u = ", sid);
    eb_mem(ctx->b, scr_buf.buf, scr_buf.len);
    free(scr_buf.buf);
    eb_cstr(ctx->b, ";\n");

    if (eli.is_nullable_ptr) {
      /* 1-word nullable-pointer enum: NULL = null_case, non-NULL = ptr_case */
      const StigiMatchArm *null_arm = NULL;
      const StigiMatchArm *ptr_arm = NULL;
      for (uint32_t a = 0; a < e->u.match_expr.arm_count; a++) {
        uint32_t cidx = 0;
        if (find_case_in_enum(ed, e->u.match_expr.arms[a].pat.name, &cidx)) {
          if (cidx == eli.null_case) null_arm = &e->u.match_expr.arms[a];
          else if (cidx == eli.ptr_case) ptr_arm = &e->u.match_expr.arms[a];
        }
      }
      eb_fmt(ctx->b, "  if (_sc%u == NULL) {\n", sid);
      if (null_arm != NULL) {
        uint32_t saved = ctx->var_count;
        if (lhs_var != NULL) emit_assign_to(ctx, null_arm->body, lhs_var, hint);
        else emit_stmt_or_ret(ctx, null_arm->body, is_tail, is_void);
        emit_ctx_pop_vars(ctx, saved);
      }
      eb_cstr(ctx->b, "  } else {\n");
      if (ptr_arm != NULL) {
        uint32_t saved = ctx->var_count;
        const StigiEnumCase *ec = &ed->u.enum_decl.cases[eli.ptr_case];
        if (ptr_arm->pat.field_count == 1 && ec->field_count == 1 &&
            !(ptr_arm->pat.fields[0].len == 1 && ptr_arm->pat.fields[0].ptr[0] == '_')) {
          EmitVar *v = emit_register_var(ctx, ptr_arm->pat.fields[0], ec->fields[0].type,
                                         (uint32_t)enum_uidx, false);
          if (v != NULL) {
            eb_cstr(ctx->b, "    ");
            emit_c_type(ctx->b, ctx->img, (uint32_t)enum_uidx, ec->fields[0].type, 0);
            eb_cstr(ctx->b, " ");
            emit_var_cname(ctx->b, v);
            eb_fmt(ctx->b, " = _sc%u;\n", sid);
          }
        }
        if (lhs_var != NULL) emit_assign_to(ctx, ptr_arm->body, lhs_var, hint);
        else emit_stmt_or_ret(ctx, ptr_arm->body, is_tail, is_void);
        emit_ctx_pop_vars(ctx, saved);
      }
      eb_cstr(ctx->b, "  }\n");
      if (lhs_var == NULL && is_tail && is_void && ctx->is_tail_loop) eb_cstr(ctx->b, "  return;\n");
      return;
    }

    /* Tagged enum: switch on tag with every case and NO default (emitter.md 2).
     * When is_tail && !is_void && lhs_var == NULL, capture result in zero-initialized local
     * so there is ZERO __builtin_unreachable() (emitter.md 3) AND zero -Wreturn-type! */
    char mret_name[32];
    mret_name[0] = '\0';
    const char *eff_lhs = lhs_var;
    bool emit_tail_ret = false;
    if (lhs_var == NULL && is_tail && !is_void && !ctx->is_tail_loop) {
      uint32_t rid = ctx->tmp_id++;
      (void)snprintf(mret_name, sizeof mret_name, "_mr%u", rid);
      eb_cstr(ctx->b, "  ");
      if (ctx->ret_ast_type != NULL) {
        emit_c_type(ctx->b, ctx->img, ctx->ret_type_uidx, ctx->ret_ast_type, 0);
      } else {
        emit_scalar_c_type(ctx->b, ctx->ret_scalar);
      }
      eb_fmt(ctx->b, " %s = {0};\n", mret_name);
      eff_lhs = mret_name;
      emit_tail_ret = true;
    }

    if (eli.tag_in_tail) {
      eb_fmt(ctx->b, "  switch (_sc%u._hdr.tag) {\n", sid);
    } else {
      eb_fmt(ctx->b, "  switch (_sc%u.tag) {\n", sid);
    }
    for (uint32_t a = 0; a < e->u.match_expr.arm_count; a++) {
      const StigiPat *pat = &e->u.match_expr.arms[a].pat;
      uint32_t cidx = 0;
      if (!find_case_in_enum(ed, pat->name, &cidx)) continue;
      const StigiEnumCase *ec = &ed->u.enum_decl.cases[cidx];
      eb_cstr(ctx->b, "    case ");
      eb_mangle_name(ctx->b, ctx->img->units[enum_uidx].name, ctx->img->units[enum_uidx].name_len);
      eb_cstr(ctx->b, "_");
      eb_str(ctx->b, ec->name);
      eb_cstr(ctx->b, ": {\n");
      EmitBuf srcb;
      memset(&srcb, 0, sizeof srcb);
      eb_fmt(&srcb, eli.tag_in_tail ? "_sc%u." : "_sc%u.u.", sid);
      eb_str(&srcb, ec->name);
      eb_cstr(&srcb, ".");
      uint32_t saved = ctx->var_count;
      const CaseBind cb = {ec, pat->fields, pat->field_count, (uint32_t)enum_uidx, srcb.buf, saved};
      for (uint32_t f = 0; f < pat->field_count && f < ec->field_count; f++) {
        if (pat->fields[f].len == 1 && pat->fields[f].ptr[0] == '_') continue;
        if (ast_type_is_void(ctx->img, (uint32_t)enum_uidx, ec->fields[f].type)) continue;
        EmitVar *v = emit_register_var(ctx, pat->fields[f], ec->fields[f].type,
                                       (uint32_t)enum_uidx, false);
        if (v != NULL && v->is_array) {
          uint32_t vi = (uint32_t)(v - ctx->vars);
          const StigiExpr *len = case_binder_len(ctx, &cb, v->len_expr); /* may grow ctx->vars */
          v = &ctx->vars[vi];
          v->len_expr = len;
        }
        if (v != NULL) {
          eb_cstr(ctx->b, "      ");
          emit_c_type(ctx->b, ctx->img, (uint32_t)enum_uidx, ec->fields[f].type, 0);
          eb_cstr(ctx->b, " ");
          emit_var_cname(ctx->b, v);
          if (eli.tag_in_tail) {
            eb_fmt(ctx->b, " = _sc%u.", sid);
          } else {
            eb_fmt(ctx->b, " = _sc%u.u.", sid);
          }
          eb_str(ctx->b, ec->name);
          eb_cstr(ctx->b, ".");
          eb_c_ident(ctx->b, ec->fields[f].name);
          eb_cstr(ctx->b, ";\n");
        }
      }
      free(srcb.buf);
      if (eff_lhs != NULL) {
        emit_assign_to(ctx, e->u.match_expr.arms[a].body, eff_lhs,
                       emit_tail_ret ? ctx->ret_scalar : hint);
        eb_cstr(ctx->b, "      break;\n");
      } else {
        emit_stmt_or_ret(ctx, e->u.match_expr.arms[a].body, is_tail, is_void);
        if (is_void || !is_tail || (ctx->is_tail_loop && stmt_ends_in_switch(e->u.match_expr.arms[a].body))) eb_cstr(ctx->b, "      break;\n");
      }
      emit_ctx_pop_vars(ctx, saved);
      eb_cstr(ctx->b, "    }\n");
    }
    eb_cstr(ctx->b, "  }\n");
    if (emit_tail_ret) {
      eb_fmt(ctx->b, "  return %s;\n", mret_name);
    } else if (lhs_var == NULL && is_tail && is_void && ctx->is_tail_loop) {
      eb_cstr(ctx->b, "  return;\n");
    }
    return;
  }

  /* Integer or Bool match */
  EmitScalarTy st = infer_expr_scalar(ctx, e->u.match_expr.scrutinee);
  if (st == ETY_UNKNOWN) st = ETY_U64;
  uint32_t sid = ctx->tmp_id++;
  EmitBuf scr_buf;
  emit_expr_with_pre(ctx, e->u.match_expr.scrutinee, st, &scr_buf);
  eb_line_dir(ctx->b, e->span.line, u->name, u->name_len);
  eb_cstr(ctx->b, "  ");
  emit_scalar_c_type(ctx->b, st);
  eb_fmt(ctx->b, " _sc%u = ", sid);
  eb_mem(ctx->b, scr_buf.buf, scr_buf.len);
  free(scr_buf.buf);
  eb_cstr(ctx->b, ";\n");

  char mret_name[32];
  mret_name[0] = '\0';
  const char *eff_lhs = lhs_var;
  bool emit_tail_ret = false;
  if (lhs_var == NULL && is_tail && !is_void && !ctx->is_tail_loop) {
    uint32_t rid = ctx->tmp_id++;
    (void)snprintf(mret_name, sizeof mret_name, "_mr%u", rid);
    eb_cstr(ctx->b, "  ");
    if (ctx->ret_ast_type != NULL) {
      emit_c_type(ctx->b, ctx->img, ctx->ret_type_uidx, ctx->ret_ast_type, 0);
    } else {
      emit_scalar_c_type(ctx->b, ctx->ret_scalar);
    }
    eb_fmt(ctx->b, " %s = {0};\n", mret_name);
    eff_lhs = mret_name;
    emit_tail_ret = true;
  }

  if (st == ETY_BOOL) {
    eb_fmt(ctx->b, "  switch ((int)_sc%u) {\n", sid);
  } else {
    eb_fmt(ctx->b, "  switch (_sc%u) {\n", sid);
  }
  for (uint32_t a = 0; a < e->u.match_expr.arm_count; a++) {
    const StigiPat *pat = &e->u.match_expr.arms[a].pat;
    uint32_t saved = ctx->var_count;
    if (pat->kind == STIGI_PAT_BOOL) {
      eb_fmt(ctx->b, "    case %d: {\n", pat->bool_val ? 1 : 0);
    } else if (pat->kind == STIGI_PAT_INT) {
      eb_cstr(ctx->b, "    case ");
      emit_typed_int_lit(ctx->b, pat->int_val, false, st);
      eb_cstr(ctx->b, ": {\n");
    } else {
      eb_cstr(ctx->b, "    default: {\n");
      if (pat->kind == STIGI_PAT_VAR && !(pat->name.len == 1 && pat->name.ptr[0] == '_')) {
        EmitVar *v = emit_push_var_raw(ctx, pat->name, true);
        if (v != NULL) {
          v->scalar_ty = st;
          eb_cstr(ctx->b, "      ");
          emit_scalar_c_type(ctx->b, st);
          eb_cstr(ctx->b, " ");
          emit_var_cname(ctx->b, v);
          eb_fmt(ctx->b, " = _sc%u;\n", sid);
        }
      }
    }
    if (eff_lhs != NULL) {
      emit_assign_to(ctx, e->u.match_expr.arms[a].body, eff_lhs,
                     emit_tail_ret ? ctx->ret_scalar : hint);
      eb_cstr(ctx->b, "      break;\n");
    } else {
      emit_stmt_or_ret(ctx, e->u.match_expr.arms[a].body, is_tail, is_void);
      if (is_void || !is_tail || (ctx->is_tail_loop && stmt_ends_in_switch(e->u.match_expr.arms[a].body))) eb_cstr(ctx->b, "      break;\n");
    }
    emit_ctx_pop_vars(ctx, saved);
    eb_cstr(ctx->b, "    }\n");
  }
  eb_cstr(ctx->b, "  }\n");
  if (emit_tail_ret) {
    eb_fmt(ctx->b, "  return %s;\n", mret_name);
  } else if (lhs_var == NULL && is_tail && is_void && ctx->is_tail_loop) {
    eb_cstr(ctx->b, "  return;\n");
  }
}

/* Lower value-producing complex expressions into an assignment to lhs_var */
static void emit_assign_to(EmitCtx *ctx, const StigiExpr *e, const char *lhs_var,
                           EmitScalarTy hint) {
  if (e == NULL) {
    eb_fmt(ctx->b, "  %s = 0;\n", lhs_var);
    return;
  }
  const StigiImageUnit *u = &ctx->img->units[ctx->uidx];
  if (e->kind == STIGI_EXPR_BLOCK) {
    uint32_t saved = ctx->var_count;
    eb_cstr(ctx->b, "  {\n");
    for (uint32_t i = 0; i < e->u.block.count; i++) {
      if (i + 1 == e->u.block.count) {
        emit_assign_to(ctx, e->u.block.stmts[i], lhs_var, hint);
      } else {
        emit_stmt_or_ret(ctx, e->u.block.stmts[i], false, true);
      }
    }
    emit_ctx_pop_vars(ctx, saved);
    eb_cstr(ctx->b, "  }\n");
    return;
  }
  if (e->kind == STIGI_EXPR_HANDLE) {
    uint32_t saved = ctx->var_count;
    eb_cstr(ctx->b, "  {\n");
    emit_handle_setup(ctx, e);
    emit_assign_to(ctx, e->u.handle_stmt.body, lhs_var, hint);
    emit_ctx_pop_vars(ctx, saved);
    eb_cstr(ctx->b, "  }\n");
    return;
  }
  if (e->kind == STIGI_EXPR_IF) {
    EmitBuf cond_buf;
    emit_expr_with_pre(ctx, e->u.if_expr.cond, ETY_BOOL, &cond_buf);
    eb_line_dir(ctx->b, e->span.line, u->name, u->name_len);
    if (e->u.if_expr.cond->kind == STIGI_EXPR_BINOP && cond_buf.len >= 2U &&
        cond_buf.buf[0] == '(' && cond_buf.buf[cond_buf.len - 1U] == ')') {
      eb_cstr(ctx->b, "  if ");
      eb_mem(ctx->b, cond_buf.buf, cond_buf.len);
      eb_cstr(ctx->b, " {\n");
    } else {
      eb_cstr(ctx->b, "  if (");
      eb_mem(ctx->b, cond_buf.buf, cond_buf.len);
      eb_cstr(ctx->b, ") {\n");
    }
    free(cond_buf.buf);
    uint32_t saved = ctx->var_count;
    emit_assign_to(ctx, e->u.if_expr.then_br, lhs_var, hint);
    emit_ctx_pop_vars(ctx, saved);
    eb_cstr(ctx->b, "  } else {\n");
    emit_assign_to(ctx, e->u.if_expr.else_br, lhs_var, hint);
    emit_ctx_pop_vars(ctx, saved);
    eb_cstr(ctx->b, "  }\n");
    return;
  }
  if (e->kind == STIGI_EXPR_MATCH) {
    emit_match_core(ctx, e, lhs_var, hint, false, false);
    return;
  }
  EmitBuf rhs_buf;
  emit_expr_with_pre(ctx, e, hint, &rhs_buf);
  eb_fmt(ctx->b, "  %s = ", lhs_var);
  eb_mem(ctx->b, rhs_buf.buf, rhs_buf.len);
  eb_cstr(ctx->b, ";\n");
  free(rhs_buf.buf);
}

static void emit_stmt_or_ret(EmitCtx *ctx, const StigiExpr *e, bool is_tail, bool is_void) {
  if (e == NULL) {
    if (is_tail && is_void && ctx->is_tail_loop) eb_cstr(ctx->b, "  return;\n");
    return;
  }
  const StigiImageUnit *u = &ctx->img->units[ctx->uidx];

  /* Erase bare @<..>, deny, local_decl, and lemma calls completely (S108/S119) */
  if (e->kind == STIGI_EXPR_ASSERT && e->u.assert_expr.expr == NULL) {
    if (is_tail && is_void && ctx->is_tail_loop) eb_cstr(ctx->b, "  return;\n");
    return;
  }
  if (e->kind == STIGI_EXPR_DENY || e->kind == STIGI_EXPR_LOCAL_DECL) {
    if (is_tail && is_void && ctx->is_tail_loop) eb_cstr(ctx->b, "  return;\n");
    return;
  }
  if (e->kind == STIGI_EXPR_CALL &&
      (e->u.call.callee->kind == STIGI_EXPR_LOCAL || e->u.call.callee->kind == STIGI_EXPR_GLOBAL)) {
    int32_t tgt = stigi_image_resolve_name(ctx->img, ctx->uidx,
                                           e->u.call.callee->u.ident.name.ptr,
                                           e->u.call.callee->u.ident.name.len);
    if (tgt >= 0 && ctx->img->units[tgt].ast->decl->kind == STIGI_DECL_LEMMA_DEF) {
      if (is_tail && is_void && ctx->is_tail_loop) eb_cstr(ctx->b, "  return;\n");
      return;
    }
  }

  if (e->kind == STIGI_EXPR_BLOCK) {
    uint32_t saved = ctx->var_count;
    eb_cstr(ctx->b, "  {\n");
    if (e->u.block.count == 0) {
      if (is_tail && is_void && ctx->is_tail_loop) eb_cstr(ctx->b, "  return;\n");
    } else {
      for (uint32_t i = 0; i < e->u.block.count; i++) {
        bool last = (i + 1 == e->u.block.count);
        emit_stmt_or_ret(ctx, e->u.block.stmts[i], is_tail && last, last ? is_void : true);
      }
    }
    emit_ctx_pop_vars(ctx, saved);
    eb_cstr(ctx->b, "  }\n");
    return;
  }

  if (e->kind == STIGI_EXPR_HANDLE) {
    if (e->u.handle_stmt.body != NULL) {
      uint32_t saved = ctx->var_count;
      eb_cstr(ctx->b, "  {\n");
      emit_handle_setup(ctx, e);
      emit_stmt_or_ret(ctx, e->u.handle_stmt.body, is_tail, is_void);
      emit_ctx_pop_vars(ctx, saved);
      eb_cstr(ctx->b, "  }\n");
    } else {
      /* Statement-form handle stays in scope for rest of enclosing block */
      emit_handle_setup(ctx, e);
      if (is_tail && is_void && ctx->is_tail_loop) eb_cstr(ctx->b, "  return;\n");
    }
    return;
  }

  /* Tail self-call -> update original params left-to-right & continue (emitter.md 2) */
  if (is_tail && ctx->is_tail_loop && is_self_tail_call(ctx, e)) {
    const StigiDefHead *h = ctx->cur_head;
    eb_line_dir(ctx->b, e->span.line, u->name, u->name_len);
    eb_cstr(ctx->b, "  {\n");
    uint32_t n = e->u.call.arg_count < h->param_count ? e->u.call.arg_count : h->param_count;
    for (uint32_t i = 0; i < n; i++) {
      EmitScalarTy phint = ast_type_to_scalar(ctx->img, ctx->uidx, h->params[i].type, 0);
      EmitBuf abuf;
      emit_expr_with_pre(ctx, e->u.call.args[i], phint, &abuf);
      eb_cstr(ctx->b, "    ");
      emit_c_type(ctx->b, ctx->img, ctx->uidx, h->params[i].type, 0);
      eb_fmt(ctx->b, " _t%u = ", i);
      eb_mem(ctx->b, abuf.buf, abuf.len);
      eb_cstr(ctx->b, ";\n");
      free(abuf.buf);
    }
    for (uint32_t i = 0; i < n; i++) {
      eb_cstr(ctx->b, "    ");
      if (i < ctx->param_cname_count && ctx->param_cnames[i] != NULL) {
        eb_cstr(ctx->b, ctx->param_cnames[i]);
      } else {
        eb_c_ident(ctx->b, h->params[i].name);
      }
      eb_fmt(ctx->b, " = _t%u;\n", i);
    }
    eb_cstr(ctx->b, "    continue;\n  }\n");
    return;
  }

  if (e->kind == STIGI_EXPR_IF) {
    EmitBuf cond_buf;
    emit_expr_with_pre(ctx, e->u.if_expr.cond, ETY_BOOL, &cond_buf);
    eb_line_dir(ctx->b, e->span.line, u->name, u->name_len);
    if (e->u.if_expr.cond->kind == STIGI_EXPR_BINOP && cond_buf.len >= 2U &&
        cond_buf.buf[0] == '(' && cond_buf.buf[cond_buf.len - 1U] == ')') {
      eb_cstr(ctx->b, "  if ");
      eb_mem(ctx->b, cond_buf.buf, cond_buf.len);
      eb_cstr(ctx->b, " {\n");
    } else {
      eb_cstr(ctx->b, "  if (");
      eb_mem(ctx->b, cond_buf.buf, cond_buf.len);
      eb_cstr(ctx->b, ") {\n");
    }
    free(cond_buf.buf);
    uint32_t saved = ctx->var_count;
    emit_stmt_or_ret(ctx, e->u.if_expr.then_br, is_tail, is_void);
    emit_ctx_pop_vars(ctx, saved);
    if (e->u.if_expr.else_br != NULL) {
      eb_cstr(ctx->b, "  } else {\n");
      emit_stmt_or_ret(ctx, e->u.if_expr.else_br, is_tail, is_void);
      emit_ctx_pop_vars(ctx, saved);
      eb_cstr(ctx->b, "  }\n");
    } else if (is_tail && is_void && ctx->is_tail_loop) {
      eb_cstr(ctx->b, "  } else {\n    return;\n  }\n");
    } else {
      eb_cstr(ctx->b, "  }\n");
    }
    return;
  }

  if (e->kind == STIGI_EXPR_MATCH) {
    emit_match_core(ctx, e, NULL, ETY_UNKNOWN, is_tail, is_void);
    return;
  }

  eb_line_dir(ctx->b, e->span.line, u->name, u->name_len);
  if (e->kind == STIGI_EXPR_VAL) {
    if ((e->u.val_stmt.type != NULL && ast_type_is_void(ctx->img, ctx->uidx, e->u.val_stmt.type)) ||
        e->u.val_stmt.pat.kind == STIGI_PAT_WILD ||
        (e->u.val_stmt.pat.kind == STIGI_PAT_VAR && e->u.val_stmt.pat.name.len == 1 &&
         e->u.val_stmt.pat.name.ptr[0] == '_')) {
      emit_stmt_or_ret(ctx, e->u.val_stmt.init, false, true);
      if (is_tail && is_void && ctx->is_tail_loop) eb_cstr(ctx->b, "  return;\n");
      return;
    }
    EmitScalarTy vhint = e->u.val_stmt.type != NULL
                             ? ast_type_to_scalar(ctx->img, ctx->uidx, e->u.val_stmt.type, 0)
                             : infer_expr_scalar(ctx, e->u.val_stmt.init);
    EmitBuf ctype_b;
    memset(&ctype_b, 0, sizeof ctype_b);
    if (e->u.val_stmt.type != NULL) {
      emit_c_type(&ctype_b, ctx->img, ctx->uidx, e->u.val_stmt.type, 0);
    } else {
      (void)emit_inferred_expr_c_type(&ctype_b, ctx, e->u.val_stmt.init, vhint);
    }

    if (e->u.val_stmt.pat.kind == STIGI_PAT_CTOR) {
      /* Single-case enum destructuring: `val Pair(a, b) = expr` */
      uint32_t cidx = 0;
      int32_t eidx = infer_expr_enum_uidx(ctx, e->u.val_stmt.init);
      if (eidx < 0) eidx = find_enum_case_owner_det(ctx->img, ctx->uidx, e->u.val_stmt.pat.name, &cidx);
      else (void)find_case_in_enum(ctx->img->units[eidx].ast->decl, e->u.val_stmt.pat.name, &cidx);
      uint32_t tid = ctx->tmp_id++;
      char tname[32];
      (void)snprintf(tname, sizeof tname, "_ds%u", tid);
      eb_cstr(ctx->b, "  ");
      eb_mem(ctx->b, ctype_b.buf, ctype_b.len);
      eb_fmt(ctx->b, " %s = {0};\n", tname);
      emit_assign_to(ctx, e->u.val_stmt.init, tname, ETY_UNKNOWN);
      if (eidx >= 0) {
        const StigiDecl *ed = ctx->img->units[eidx].ast->decl;
        const StigiEnumCase *ec = &ed->u.enum_decl.cases[cidx];
        StigiEnumLayoutInfo eli = stigi_enum_layout_info(ctx->img, (uint32_t)eidx, NULL, 0);
        uint32_t first = ctx->var_count;
        EmitBuf srcb;
        memset(&srcb, 0, sizeof srcb);
        eb_fmt(&srcb, eli.tag_in_tail ? "%s." : "%s.u.", tname);
        eb_str(&srcb, ec->name);
        eb_cstr(&srcb, ".");
        const CaseBind cb = {ec, e->u.val_stmt.pat.fields, e->u.val_stmt.pat.field_count, (uint32_t)eidx, srcb.buf,
                             first};
        for (uint32_t f = 0; f < e->u.val_stmt.pat.field_count && f < ec->field_count; f++) {
          StigiStr fn = e->u.val_stmt.pat.fields[f];
          if (fn.len == 1 && fn.ptr[0] == '_') continue;
          if (ast_type_is_void(ctx->img, (uint32_t)eidx, ec->fields[f].type)) continue;
          EmitVar *v = emit_register_var(ctx, fn, ec->fields[f].type, (uint32_t)eidx, false);
          if (v != NULL && v->is_array) {
            uint32_t vi = (uint32_t)(v - ctx->vars);
            const StigiExpr *len = case_binder_len(ctx, &cb, v->len_expr); /* may grow ctx->vars */
            v = &ctx->vars[vi];
            v->len_expr = len;
          }
          if (v != NULL) {
            eb_cstr(ctx->b, "  ");
            emit_c_type(ctx->b, ctx->img, (uint32_t)eidx, ec->fields[f].type, 0);
            eb_cstr(ctx->b, " const ");
            emit_var_cname(ctx->b, v);
            if (eli.is_nullable_ptr) {
              eb_fmt(ctx->b, " = %s;\n", tname);
            } else if (eli.tag_in_tail) {
              eb_fmt(ctx->b, " = %s.", tname);
              eb_str(ctx->b, ec->name);
              eb_cstr(ctx->b, ".");
              eb_c_ident(ctx->b, ec->fields[f].name);
              eb_cstr(ctx->b, ";\n");
            } else {
              eb_fmt(ctx->b, " = %s.u.", tname);
              eb_str(ctx->b, ec->name);
              eb_cstr(ctx->b, ".");
              eb_c_ident(ctx->b, ec->fields[f].name);
              eb_cstr(ctx->b, ";\n");
            }
          }
        }
        free(srcb.buf);
      }
      free(ctype_b.buf);
      if (is_tail && is_void && ctx->is_tail_loop) eb_cstr(ctx->b, "  return;\n");
      return;
    }

    if (expr_is_complex_stmt(e->u.val_stmt.init)) {
      uint32_t tid = ctx->tmp_id++;
      char tname[32];
      (void)snprintf(tname, sizeof tname, "_v%u", tid);
      eb_cstr(ctx->b, "  ");
      eb_mem(ctx->b, ctype_b.buf, ctype_b.len);
      eb_fmt(ctx->b, " %s = {0};\n", tname);
      emit_assign_to(ctx, e->u.val_stmt.init, tname, vhint);
      EmitVar *v = emit_register_var(ctx, e->u.val_stmt.pat.name, e->u.val_stmt.type,
                                     ctx->uidx, false);
      if (v != NULL) {
        if (v->scalar_ty == ETY_UNKNOWN) v->scalar_ty = vhint;
        if (v->enum_uidx < 0) v->enum_uidx = infer_expr_enum_uidx(ctx, e->u.val_stmt.init);
        eb_cstr(ctx->b, "  ");
        eb_mem(ctx->b, ctype_b.buf, ctype_b.len);
        eb_cstr(ctx->b, " const ");
        emit_var_cname(ctx->b, v);
        eb_fmt(ctx->b, " = %s;\n", tname);
      }
    } else {
      EmitBuf init_buf;
      emit_expr_with_pre(ctx, e->u.val_stmt.init, vhint, &init_buf);
      EmitVar *v = emit_register_var(ctx, e->u.val_stmt.pat.name, e->u.val_stmt.type,
                                     ctx->uidx, false);
      emit_copy_alias(ctx, e->u.val_stmt.init, v);
      if (v != NULL) {
        if (v->scalar_ty == ETY_UNKNOWN) v->scalar_ty = vhint;
        if (v->enum_uidx < 0) v->enum_uidx = infer_expr_enum_uidx(ctx, e->u.val_stmt.init);
        eb_cstr(ctx->b, "  ");
        eb_mem(ctx->b, ctype_b.buf, ctype_b.len);
        eb_cstr(ctx->b, " const ");
        emit_var_cname(ctx->b, v);
        eb_cstr(ctx->b, " = ");
        eb_mem(ctx->b, init_buf.buf, init_buf.len);
        eb_cstr(ctx->b, ";\n");
      }
      free(init_buf.buf);
    }
    free(ctype_b.buf);
    if (is_tail && is_void && ctx->is_tail_loop) eb_cstr(ctx->b, "  return;\n");
    return;
  }

  if (e->kind == STIGI_EXPR_VAR) {
    if (e->u.var_stmt.type != NULL && ast_type_is_void(ctx->img, ctx->uidx, e->u.var_stmt.type)) {
      emit_stmt_or_ret(ctx, e->u.var_stmt.init, false, true);
      if (is_tail && is_void && ctx->is_tail_loop) eb_cstr(ctx->b, "  return;\n");
      return;
    }
    EmitScalarTy vhint = e->u.var_stmt.type != NULL
                             ? ast_type_to_scalar(ctx->img, ctx->uidx, e->u.var_stmt.type, 0)
                             : infer_expr_scalar(ctx, e->u.var_stmt.init);
    EmitBuf ctype_b;
    memset(&ctype_b, 0, sizeof ctype_b);
    if (e->u.var_stmt.type != NULL) {
      emit_c_type(&ctype_b, ctx->img, ctx->uidx, e->u.var_stmt.type, 0);
    } else {
      (void)emit_inferred_expr_c_type(&ctype_b, ctx, e->u.var_stmt.init, vhint);
    }
    if (expr_is_complex_stmt(e->u.var_stmt.init)) {
      uint32_t tid = ctx->tmp_id++;
      char tname[32];
      (void)snprintf(tname, sizeof tname, "_v%u", tid);
      eb_cstr(ctx->b, "  ");
      eb_mem(ctx->b, ctype_b.buf, ctype_b.len);
      eb_fmt(ctx->b, " %s = {0};\n", tname);
      emit_assign_to(ctx, e->u.var_stmt.init, tname, vhint);
      EmitVar *v = emit_register_var(ctx, e->u.var_stmt.name, e->u.var_stmt.type,
                                     ctx->uidx, false);
      if (v != NULL) {
        if (v->scalar_ty == ETY_UNKNOWN) v->scalar_ty = vhint;
        if (v->enum_uidx < 0) v->enum_uidx = infer_expr_enum_uidx(ctx, e->u.var_stmt.init);
        eb_cstr(ctx->b, "  ");
        eb_mem(ctx->b, ctype_b.buf, ctype_b.len);
        eb_cstr(ctx->b, " ");
        emit_var_cname(ctx->b, v);
        eb_fmt(ctx->b, " = %s;\n", tname);
      }
    } else {
      EmitBuf init_buf;
      emit_expr_with_pre(ctx, e->u.var_stmt.init, vhint, &init_buf);
      EmitVar *v = emit_register_var(ctx, e->u.var_stmt.name, e->u.var_stmt.type,
                                     ctx->uidx, false);
      emit_copy_alias(ctx, e->u.var_stmt.init, v);
      if (v != NULL) {
        if (v->scalar_ty == ETY_UNKNOWN) v->scalar_ty = vhint;
        if (v->enum_uidx < 0) v->enum_uidx = infer_expr_enum_uidx(ctx, e->u.var_stmt.init);
        eb_cstr(ctx->b, "  ");
        eb_mem(ctx->b, ctype_b.buf, ctype_b.len);
        eb_cstr(ctx->b, " ");
        emit_var_cname(ctx->b, v);
        eb_cstr(ctx->b, " = ");
        eb_mem(ctx->b, init_buf.buf, init_buf.len);
        eb_cstr(ctx->b, ";\n");
      }
      free(init_buf.buf);
    }
    free(ctype_b.buf);
    if (is_tail && is_void && ctx->is_tail_loop) eb_cstr(ctx->b, "  return;\n");
    return;
  }

  if (e->kind == STIGI_EXPR_ASSIGN) {
    const EmitVar *av = emit_find_var(ctx, e->u.assign_stmt.name);
    EmitScalarTy vhint = av != NULL ? av->scalar_ty : ETY_UNKNOWN;
    EmitBuf lhs_b;
    memset(&lhs_b, 0, sizeof lhs_b);
    if (av != NULL) emit_var_cname(&lhs_b, av);
    else eb_c_ident(&lhs_b, e->u.assign_stmt.name);
    if (expr_is_complex_stmt(e->u.assign_stmt.value)) {
      emit_assign_to(ctx, e->u.assign_stmt.value, lhs_b.buf != NULL ? lhs_b.buf : "", vhint);
    } else {
      EmitBuf val_buf;
      emit_expr_with_pre(ctx, e->u.assign_stmt.value, vhint, &val_buf);
      eb_cstr(ctx->b, "  ");
      eb_mem(ctx->b, lhs_b.buf, lhs_b.len);
      eb_cstr(ctx->b, " = ");
      eb_mem(ctx->b, val_buf.buf, val_buf.len);
      eb_cstr(ctx->b, ";\n");
      free(val_buf.buf);
    }
    free(lhs_b.buf);
    if (is_tail && is_void && ctx->is_tail_loop) eb_cstr(ctx->b, "  return;\n");
    return;
  }

  if (e->kind == STIGI_EXPR_WHILE) {
    EmitBuf cpre, cbuf;
    memset(&cpre, 0, sizeof cpre);
    memset(&cbuf, 0, sizeof cbuf);
    EmitBuf *saved_b = ctx->b;
    EmitBuf *saved_pre = ctx->pre;
    ctx->b = &cbuf;
    ctx->pre = &cpre;
    emit_expr_typed(ctx, e->u.while_expr.cond, ETY_BOOL);
    ctx->b = saved_b;
    ctx->pre = saved_pre;
    if (cpre.len > 0) {
      eb_cstr(ctx->b, "  while (1) {\n");
      eb_mem(ctx->b, cpre.buf, cpre.len);
      eb_cstr(ctx->b, "    if (!(");
      eb_mem(ctx->b, cbuf.buf, cbuf.len);
      eb_cstr(ctx->b, ")) break;\n");
    } else if (e->u.while_expr.cond->kind == STIGI_EXPR_BINOP && cbuf.len >= 2U &&
               cbuf.buf[0] == '(' && cbuf.buf[cbuf.len - 1U] == ')') {
      eb_cstr(ctx->b, "  while ");
      eb_mem(ctx->b, cbuf.buf, cbuf.len);
      eb_cstr(ctx->b, " {\n");
    } else {
      eb_cstr(ctx->b, "  while (");
      eb_mem(ctx->b, cbuf.buf, cbuf.len);
      eb_cstr(ctx->b, ") {\n");
    }
    free(cpre.buf);
    free(cbuf.buf);
    uint32_t saved = ctx->var_count;
    emit_stmt_or_ret(ctx, e->u.while_expr.body, false, true);
    emit_ctx_pop_vars(ctx, saved);
    eb_cstr(ctx->b, "  }\n");
    if (is_tail && is_void && ctx->is_tail_loop) eb_cstr(ctx->b, "  return;\n");
    return;
  }

  if (e->kind == STIGI_EXPR_INDEX_WRITE) {
    /* Cell write `c(0) := v` */
    if (e->u.index_expr.target->kind == STIGI_EXPR_LOCAL) {
      const EmitVar *cv = emit_find_var(ctx, e->u.index_expr.target->u.ident.name);
      if (cv != NULL && cv->is_cell) {
        EmitBuf val_buf;
        emit_expr_with_pre(ctx, e->u.index_expr.value, cv->elem_scalar_ty, &val_buf);
        eb_cstr(ctx->b, "  *(");
        emit_var_cname(ctx->b, cv);
        eb_cstr(ctx->b, ") = ");
        eb_mem(ctx->b, val_buf.buf, val_buf.len);
        eb_cstr(ctx->b, ";\n");
        free(val_buf.buf);
        if (is_tail && is_void && ctx->is_tail_loop) eb_cstr(ctx->b, "  return;\n");
        return;
      }
    }
    EmitScalarTy elem_ty = ETY_UNKNOWN;
    const StigiExpr *len_expr = find_array_len_expr(ctx, e->u.index_expr.target, &elem_ty);
    EmitBuf tgt_buf, idx_buf, val_buf;
    emit_expr_with_pre(ctx, e->u.index_expr.target, ETY_UNKNOWN, &tgt_buf);
    emit_expr_with_pre(ctx, e->u.index_expr.index, ETY_U64, &idx_buf);
    emit_expr_with_pre(ctx, e->u.index_expr.value, elem_ty, &val_buf);
    bool do_mask = ctx_next_access_masked(ctx);
    EmitBuf len_buf;
    memset(&len_buf, 0, sizeof len_buf);
    if (do_mask) {
      if (len_expr == NULL) {
        ctx->fail_closed = true;
        (void)snprintf(ctx->fail_msg, sizeof ctx->fail_msg,
                       "cannot emit required Spectre mask: missing array length expression in '%s'",
                       u->name);
      } else {
        /* Flush any pre-hoist BEFORE opening the write statement */
        emit_expr_with_pre(ctx, len_expr, ETY_U64, &len_buf);
      }
    }
    eb_cstr(ctx->b, "  ");
    eb_mem(ctx->b, tgt_buf.buf, tgt_buf.len);
    eb_cstr(ctx->b, "[");
    if (do_mask && len_expr != NULL) {
      ctx->stats->spectre_masks++;
      eb_cstr(ctx->b, "Stigi_mask_index((uint64_t)(");
      eb_mem(ctx->b, idx_buf.buf, idx_buf.len);
      eb_cstr(ctx->b, "), (uint64_t)(");
      eb_mem(ctx->b, len_buf.buf, len_buf.len);
      eb_cstr(ctx->b, "))");
    } else {
      eb_mem(ctx->b, idx_buf.buf, idx_buf.len);
    }
    eb_cstr(ctx->b, "] = ");
    eb_mem(ctx->b, val_buf.buf, val_buf.len);
    eb_cstr(ctx->b, ";\n");
    free(len_buf.buf);
    free(tgt_buf.buf);
    free(idx_buf.buf);
    free(val_buf.buf);
    if (is_tail && is_void && ctx->is_tail_loop) eb_cstr(ctx->b, "  return;\n");
    return;
  }

  /* Expression statement or tail return */
  if (is_tail && !is_void) {
    EmitBuf expr_buf;
    emit_expr_with_pre(ctx, e, ctx->ret_scalar, &expr_buf);
    eb_cstr(ctx->b, "  return ");
    eb_mem(ctx->b, expr_buf.buf, expr_buf.len);
    eb_cstr(ctx->b, ";\n");
    free(expr_buf.buf);
  } else {
    if (e->kind != STIGI_EXPR_LIT_UNIT) {
      EmitBuf expr_buf;
      emit_expr_with_pre(ctx, e, ETY_UNKNOWN, &expr_buf);
      EmitScalarTy es = infer_expr_scalar(ctx, e);
      if (es == ETY_UNIT || es == ETY_NEVER) {
        eb_cstr(ctx->b, "  ");
        eb_mem(ctx->b, expr_buf.buf, expr_buf.len);
        eb_cstr(ctx->b, ";\n");
      } else {
        eb_cstr(ctx->b, "  (void)(");
        eb_mem(ctx->b, expr_buf.buf, expr_buf.len);
        eb_cstr(ctx->b, ");\n");
      }
      free(expr_buf.buf);
    }
    if (is_tail && is_void && ctx->is_tail_loop) {
      eb_cstr(ctx->b, "  return;\n");
    }
  }
}

/* Helper: emit enum constructor expression for all 4 layout representations */
static void emit_enum_ctor_expr(EmitCtx *ctx, int32_t eidx, uint32_t cidx, const StigiExpr *call_e) {
  const StigiImageUnit *eu = &ctx->img->units[eidx];
  const StigiDecl *ed = eu->ast->decl;
  const StigiEnumCase *ec = &ed->u.enum_decl.cases[cidx];
  StigiEnumLayoutInfo eli = stigi_enum_layout_info(ctx->img, (uint32_t)eidx, NULL, 0);

  if (eli.is_nullable_ptr) {
    if (cidx == eli.null_case) {
      eb_cstr(ctx->b, "((");
      eb_mangle_name(ctx->b, eu->name, eu->name_len);
      eb_cstr(ctx->b, ")0)");
    } else {
      eb_cstr(ctx->b, "((");
      eb_mangle_name(ctx->b, eu->name, eu->name_len);
      eb_cstr(ctx->b, ")(");
      if (call_e != NULL && call_e->u.call.arg_count >= 1) {
        emit_expr_typed(ctx, call_e->u.call.args[0], ETY_UNKNOWN);
      } else {
        eb_cstr(ctx->b, "0");
      }
      eb_cstr(ctx->b, "))");
    }
    return;
  }

  uint32_t argc = call_e != NULL ? call_e->u.call.arg_count : 0U;
  uint32_t nfields = ec->field_count < argc ? ec->field_count : argc;
  uint32_t *tids = NULL;
  bool hoisted = (ctx->pre != NULL && nfields >= 2U);
  if (hoisted) {
    tids = malloc((size_t)nfields * sizeof *tids);
    if (tids == NULL) {
      ctx->b->oom = true;
      hoisted = false;
    } else {
      for (uint32_t f = 0; f < nfields; f++) {
        EmitScalarTy fh = ast_type_to_scalar(ctx->img, (uint32_t)eidx, ec->fields[f].type, 0);
        tids[f] = hoist_arg(ctx, call_e->u.call.args[f], ec->fields[f].type, (uint32_t)eidx, fh);
      }
    }
  }

  eb_cstr(ctx->b, "((");
  eb_mangle_name(ctx->b, eu->name, eu->name_len);
  eb_cstr(ctx->b, "){ ");
  if (eli.tag_in_tail) {
    if (nfields == 0) {
      eb_cstr(ctx->b, "._hdr = { .tag = ");
      eb_mangle_name(ctx->b, eu->name, eu->name_len);
      eb_cstr(ctx->b, "_");
      eb_str(ctx->b, ec->name);
      eb_cstr(ctx->b, " }");
    } else {
      eb_cstr(ctx->b, ".");
      eb_str(ctx->b, ec->name);
      eb_cstr(ctx->b, " = { ");
      for (uint32_t f = 0; f < nfields; f++) {
        if (ast_type_is_void(ctx->img, (uint32_t)eidx, ec->fields[f].type)) continue;
        eb_cstr(ctx->b, ".");
        eb_c_ident(ctx->b, ec->fields[f].name);
        eb_cstr(ctx->b, " = ");
        if (hoisted) {
          eb_fmt(ctx->b, "_a%u", tids[f]);
        } else {
          EmitScalarTy fh = ast_type_to_scalar(ctx->img, (uint32_t)eidx, ec->fields[f].type, 0);
          emit_expr_typed(ctx, call_e->u.call.args[f], fh);
        }
        eb_cstr(ctx->b, ", ");
      }
      eb_cstr(ctx->b, ".tag = ");
      eb_mangle_name(ctx->b, eu->name, eu->name_len);
      eb_cstr(ctx->b, "_");
      eb_str(ctx->b, ec->name);
      eb_cstr(ctx->b, " }");
    }
  } else {
    if (nfields > 0 && !eli.is_tag_only) {
      eb_cstr(ctx->b, ".u = { .");
      eb_str(ctx->b, ec->name);
      eb_cstr(ctx->b, " = { ");
      bool first_f = true;
      for (uint32_t f = 0; f < nfields; f++) {
        if (ast_type_is_void(ctx->img, (uint32_t)eidx, ec->fields[f].type)) continue;
        if (!first_f) eb_cstr(ctx->b, ", ");
        first_f = false;
        eb_cstr(ctx->b, ".");
        eb_c_ident(ctx->b, ec->fields[f].name);
        eb_cstr(ctx->b, " = ");
        if (hoisted) {
          eb_fmt(ctx->b, "_a%u", tids[f]);
        } else {
          EmitScalarTy fh = ast_type_to_scalar(ctx->img, (uint32_t)eidx, ec->fields[f].type, 0);
          emit_expr_typed(ctx, call_e->u.call.args[f], fh);
        }
      }
      eb_cstr(ctx->b, " } }, ");
    }
    eb_cstr(ctx->b, ".tag = ");
    eb_mangle_name(ctx->b, eu->name, eu->name_len);
    eb_cstr(ctx->b, "_");
    eb_str(ctx->b, ec->name);
  }
  eb_cstr(ctx->b, " })");
  free(tids);
}

static void emit_expr_typed(EmitCtx *ctx, const StigiExpr *e, EmitScalarTy hint) {
  if (e == NULL) {
    eb_cstr(ctx->b, "UINT64_C(0)");
    return;
  }
  switch (e->kind) {
    case STIGI_EXPR_LIT_INT:
      emit_typed_int_lit(ctx->b, e->u.lit_int.value, false, hint);
      break;
    case STIGI_EXPR_LIT_CHAR:
      eb_fmt(ctx->b, "(uint8_t)%uU", (unsigned)e->u.lit_char.value);
      break;
    case STIGI_EXPR_LIT_STR:
      eb_cstr(ctx->b, "((uint8_t *)");
      eb_str(ctx->b, e->u.lit_str.raw);
      eb_cstr(ctx->b, ")");
      break;
    case STIGI_EXPR_LIT_BOOL:
      eb_cstr(ctx->b, e->u.lit_bool.value ? "true" : "false");
      break;
    case STIGI_EXPR_LIT_UNIT:
      eb_cstr(ctx->b, "UINT64_C(0)");
      break;
    case STIGI_EXPR_LOCAL:
    case STIGI_EXPR_GLOBAL: {
      const EmitVar *lv = emit_find_var(ctx, e->u.ident.name);
      if (lv != NULL) {
        emit_var_cname(ctx->b, lv);
        break;
      }
      int32_t tgt = stigi_image_resolve_name(ctx->img, ctx->uidx,
                                             e->u.ident.name.ptr, e->u.ident.name.len);
      if (tgt >= 0) {
        const StigiDecl *td = ctx->img->units[tgt].ast->decl;
        if (td->kind == STIGI_DECL_EXTERN && td->u.extern_decl.is_val) {
          eb_str(ctx->b, extern_c_sym(td->name));
          break;
        }
        eb_mangle_name(ctx->b, ctx->img->units[tgt].name, ctx->img->units[tgt].name_len);
        break;
      }
      uint32_t cidx = 0;
      int32_t eidx = find_enum_case_owner_det(ctx->img, ctx->uidx, e->u.ident.name, &cidx);
      if (eidx >= 0) {
        emit_enum_ctor_expr(ctx, eidx, cidx, NULL);
        break;
      }
      eb_c_ident(ctx->b, e->u.ident.name);
      break;
    }
    case STIGI_EXPR_UNOP:
      if (e->u.unop.op == STIGI_OP_NEG && e->u.unop.operand != NULL &&
          e->u.unop.operand->kind == STIGI_EXPR_LIT_INT) {
        EmitScalarTy nh = (hint == ETY_UNKNOWN) ? ETY_I64 : hint;
        emit_typed_int_lit(ctx->b, e->u.unop.operand->u.lit_int.value, true, nh);
        break;
      }
      eb_cstr(ctx->b, "(");
      eb_cstr(ctx->b, c_op_str(e->u.unop.op));
      emit_expr_typed(ctx, e->u.unop.operand,
                      e->u.unop.op == STIGI_OP_NOT ? ETY_BOOL : hint);
      eb_cstr(ctx->b, ")");
      break;
    case STIGI_EXPR_BINOP: {
      /* Enum equality `==` / `!=` uses generated `Stigi_Enum_eq` helper */
      if (e->u.binop.op == STIGI_OP_EQ || e->u.binop.op == STIGI_OP_NE) {
        int32_t euidx = infer_expr_enum_uidx(ctx, e->u.binop.lhs);
        if (euidx < 0) euidx = infer_expr_enum_uidx(ctx, e->u.binop.rhs);
        if (euidx >= 0) {
          if (e->u.binop.op == STIGI_OP_NE) eb_cstr(ctx->b, "(!");
          eb_mangle_name(ctx->b, ctx->img->units[euidx].name, ctx->img->units[euidx].name_len);
          eb_cstr(ctx->b, "_eq(");
          emit_expr_typed(ctx, e->u.binop.lhs, ETY_UNKNOWN);
          eb_cstr(ctx->b, ", ");
          emit_expr_typed(ctx, e->u.binop.rhs, ETY_UNKNOWN);
          eb_cstr(ctx->b, ")");
          if (e->u.binop.op == STIGI_OP_NE) eb_cstr(ctx->b, ")");
          break;
        }
      }
      EmitScalarTy op_hint = hint;
      if (e->u.binop.op == STIGI_OP_AND || e->u.binop.op == STIGI_OP_OR) {
        op_hint = ETY_BOOL;
      } else {
        EmitScalarTy lt = infer_expr_scalar(ctx, e->u.binop.lhs);
        EmitScalarTy rt = infer_expr_scalar(ctx, e->u.binop.rhs);
        if (lt != ETY_UNKNOWN) op_hint = lt;
        else if (rt != ETY_UNKNOWN) op_hint = rt;
        else if (e->u.binop.op >= STIGI_OP_LT && e->u.binop.op <= STIGI_OP_NE) op_hint = ETY_U64;
      }
      if (e->u.binop.op == STIGI_OP_AND || e->u.binop.op == STIGI_OP_OR) {
        /* The right operand runs only when the left one does not decide: when it needs statements (an
           elaborated `is` or match), they go under `if` on a temp that carries the left value. */
        bool is_and = e->u.binop.op == STIGI_OP_AND;
        EmitBuf lbuf, rpre, rbuf;
        memset(&lbuf, 0, sizeof lbuf);
        memset(&rpre, 0, sizeof rpre);
        memset(&rbuf, 0, sizeof rbuf);
        EmitBuf *saved_b = ctx->b;
        EmitBuf *saved_pre = ctx->pre;
        ctx->b = &lbuf;
        emit_expr_typed(ctx, e->u.binop.lhs, op_hint);
        /* first as a plain C operand; a match value in it refuses (fail_closed), then it is emitted with a place
           for its statements */
        bool saved_fail = ctx->fail_closed;
        char saved_msg[sizeof ctx->fail_msg];
        memcpy(saved_msg, ctx->fail_msg, sizeof saved_msg);
        ctx->fail_closed = false;
        ctx->b = &rbuf;
        ctx->pre = NULL;
        emit_expr_typed(ctx, e->u.binop.rhs, op_hint);
        bool needs_pre = ctx->fail_closed;
        ctx->fail_closed = saved_fail;
        memcpy(ctx->fail_msg, saved_msg, sizeof saved_msg);
        if (needs_pre) {
          rbuf.len = 0;
          ctx->pre = &rpre;
          emit_expr_typed(ctx, e->u.binop.rhs, op_hint);
        }
        ctx->b = saved_b;
        ctx->pre = saved_pre;
        if (rpre.len == 0) {
          eb_cstr(ctx->b, "(");
          eb_mem(ctx->b, lbuf.buf, lbuf.len);
          eb_fmt(ctx->b, " %s ", c_op_str(e->u.binop.op));
          eb_mem(ctx->b, rbuf.buf, rbuf.len);
          eb_cstr(ctx->b, ")");
        } else if (ctx->pre == NULL) {
          ctx->fail_closed = true;
          (void)snprintf(ctx->fail_msg, sizeof ctx->fail_msg,
                         "a short-circuit operand needs statements where none can be placed");
          eb_cstr(ctx->b, "false");
        } else {
          char ename[32];
          (void)snprintf(ename, sizeof ename, "_e%u", ctx->tmp_id++);
          eb_fmt(ctx->pre, "  bool %s = ", ename);
          eb_mem(ctx->pre, lbuf.buf, lbuf.len);
          eb_fmt(ctx->pre, ";\n  if (%s%s) {\n", is_and ? "" : "!", ename);
          eb_mem(ctx->pre, rpre.buf, rpre.len);
          eb_fmt(ctx->pre, "  %s = ", ename);
          eb_mem(ctx->pre, rbuf.buf, rbuf.len);
          eb_cstr(ctx->pre, ";\n  }\n");
          eb_cstr(ctx->b, ename);
        }
        free(lbuf.buf);
        free(rpre.buf);
        free(rbuf.buf);
        break;
      }
      eb_cstr(ctx->b, "(");
      emit_expr_typed(ctx, e->u.binop.lhs, op_hint);
      eb_fmt(ctx->b, " %s ", c_op_str(e->u.binop.op));
      {
        emit_expr_typed(ctx, e->u.binop.rhs, op_hint);
      }
      eb_cstr(ctx->b, ")");
      break;
    }
    case STIGI_EXPR_IF: {
      if (ctx->pre != NULL && expr_is_complex_stmt(e)) {
        uint32_t tid = ctx->tmp_id++;
        char ename[32];
        (void)snprintf(ename, sizeof ename, "_e%u", tid);
        EmitBuf *saved_b = ctx->b;
        EmitBuf *saved_pre = ctx->pre;
        ctx->b = saved_pre;
        ctx->pre = NULL;
        eb_cstr(ctx->b, "  ");
        (void)emit_inferred_expr_c_type(ctx->b, ctx, e, hint);
        eb_fmt(ctx->b, " %s = {0};\n", ename);
        emit_assign_to(ctx, e, ename, hint);
        ctx->b = saved_b;
        ctx->pre = saved_pre;
        eb_cstr(ctx->b, ename);
        break;
      }
      EmitBuf *saved_pre = ctx->pre;
      eb_cstr(ctx->b, "(");
      emit_expr_typed(ctx, e->u.if_expr.cond, ETY_BOOL);
      eb_cstr(ctx->b, " ? ");
      ctx->pre = NULL;
      emit_expr_typed(ctx, e->u.if_expr.then_br, hint);
      eb_cstr(ctx->b, " : ");
      emit_expr_typed(ctx, e->u.if_expr.else_br, hint);
      ctx->pre = saved_pre;
      eb_cstr(ctx->b, ")");
      break;
    }
    case STIGI_EXPR_ASSERT:
      if (e->u.assert_expr.expr != NULL) emit_expr_typed(ctx, e->u.assert_expr.expr, hint);
      else eb_cstr(ctx->b, "UINT64_C(0)");
      break;
    case STIGI_EXPR_INDEX_READ: {
      if (e->u.index_expr.target->kind == STIGI_EXPR_LOCAL) {
        const EmitVar *cv = emit_find_var(ctx, e->u.index_expr.target->u.ident.name);
        if (cv != NULL && cv->is_cell) {
          eb_cstr(ctx->b, "(*(");
          emit_var_cname(ctx->b, cv);
          eb_cstr(ctx->b, "))");
          break;
        }
      }
      const StigiExpr *len_expr = find_array_len_expr(ctx, e->u.index_expr.target, NULL);
      EmitBuf idx_buf;
      memset(&idx_buf, 0, sizeof idx_buf);
      EmitBuf *saved_b = ctx->b;
      ctx->b = &idx_buf;
      emit_expr_typed(ctx, e->u.index_expr.index, ETY_U64);
      ctx->b = saved_b;
      bool do_mask = ctx_next_access_masked(ctx);
      if (do_mask && len_expr == NULL) {
        ctx->fail_closed = true;
        (void)snprintf(ctx->fail_msg, sizeof ctx->fail_msg,
                       "cannot emit required Spectre mask: missing array length expression in '%s'",
                       ctx->img->units[ctx->uidx].name);
      }
      emit_expr_typed(ctx, e->u.index_expr.target, ETY_UNKNOWN);
      eb_cstr(ctx->b, "[");
      if (do_mask && len_expr != NULL) {
        ctx->stats->spectre_masks++;
        eb_cstr(ctx->b, "Stigi_mask_index((uint64_t)(");
        eb_mem(ctx->b, idx_buf.buf, idx_buf.len);
        eb_cstr(ctx->b, "), (uint64_t)(");
        emit_expr_typed(ctx, len_expr, ETY_U64);
        eb_cstr(ctx->b, "))");
      } else {
        eb_mem(ctx->b, idx_buf.buf, idx_buf.len);
      }
      eb_cstr(ctx->b, "]");
      free(idx_buf.buf);
      break;
    }
    case STIGI_EXPR_CALL: {
      /* C-DSL `size[A]` -> `(uint64_t)sizeof(A)` (emitter.md 3) */
      if (e->u.call.callee->kind == STIGI_EXPR_LOCAL &&
          str_eq_c(e->u.call.callee->u.ident.name, "size") &&
          e->u.call.type_arg_count == 1 && !e->u.call.has_parens) {
        eb_cstr(ctx->b, "((uint64_t)sizeof(");
        emit_c_type(ctx->b, ctx->img, ctx->uidx, e->u.call.type_args[0], 0);
        eb_cstr(ctx->b, "))");
        break;
      }

      /* Array or Cell element read: arr(idx) / cell(0) */
      if (e->u.call.callee->kind == STIGI_EXPR_LOCAL && e->u.call.arg_count == 1) {
        const EmitVar *av = emit_find_var(ctx, e->u.call.callee->u.ident.name);
        if (av != NULL && av->is_cell) {
          eb_cstr(ctx->b, "(*(");
          emit_var_cname(ctx->b, av);
          eb_cstr(ctx->b, "))");
          break;
        }
        if (av != NULL && av->is_array) {
          EmitBuf idx_buf;
          memset(&idx_buf, 0, sizeof idx_buf);
          EmitBuf *saved_b = ctx->b;
          ctx->b = &idx_buf;
          emit_expr_typed(ctx, e->u.call.args[0], ETY_U64);
          ctx->b = saved_b;
          bool do_mask = ctx_next_access_masked(ctx);
          if (do_mask && av->len_expr == NULL) {
            ctx->fail_closed = true;
            (void)snprintf(ctx->fail_msg, sizeof ctx->fail_msg,
                           "cannot emit required Spectre mask: missing length for '%.*s' in '%s'",
                           (int)av->name.len, av->name.ptr, ctx->img->units[ctx->uidx].name);
          }
          emit_var_cname(ctx->b, av);
          eb_cstr(ctx->b, "[");
          if (do_mask && av->len_expr != NULL) {
            ctx->stats->spectre_masks++;
            eb_cstr(ctx->b, "Stigi_mask_index((uint64_t)(");
            eb_mem(ctx->b, idx_buf.buf, idx_buf.len);
            eb_cstr(ctx->b, "), (uint64_t)(");
            emit_expr_typed(ctx, av->len_expr, ETY_U64);
            eb_cstr(ctx->b, "))");
          } else {
            eb_mem(ctx->b, idx_buf.buf, idx_buf.len);
          }
          eb_cstr(ctx->b, "]");
          free(idx_buf.buf);
          break;
        }
      }

      if (e->u.call.callee->kind == STIGI_EXPR_LOCAL ||
          e->u.call.callee->kind == STIGI_EXPR_GLOBAL) {
        StigiStr cn = e->u.call.callee->u.ident.name;

        /* Stigi.Test.expect(got, want, reason) — supports both scalars AND enums! */
        if (str_eq_c(cn, "Stigi.Test.expect") && e->u.call.arg_count == 3) {
          int32_t euidx = infer_expr_enum_uidx(ctx, e->u.call.args[0]);
          if (euidx < 0) euidx = infer_expr_enum_uidx(ctx, e->u.call.args[1]);
          if (euidx >= 0) {
            if (ctx->pre != NULL) {
              uint32_t t0 = hoist_arg(ctx, e->u.call.args[0], NULL, ctx->uidx, ETY_UNKNOWN);
              uint32_t t1 = hoist_arg(ctx, e->u.call.args[1], NULL, ctx->uidx, ETY_UNKNOWN);
              eb_cstr(ctx->b, "(");
              eb_mangle_name(ctx->b, ctx->img->units[euidx].name, ctx->img->units[euidx].name_len);
              eb_fmt(ctx->b, "_eq(_a%u, _a%u) ? INT32_C(0) : INT32_C(1))", t0, t1);
            } else {
              eb_cstr(ctx->b, "(");
              eb_mangle_name(ctx->b, ctx->img->units[euidx].name, ctx->img->units[euidx].name_len);
              eb_cstr(ctx->b, "_eq(");
              emit_expr_typed(ctx, e->u.call.args[0], ETY_UNKNOWN);
              eb_cstr(ctx->b, ", ");
              emit_expr_typed(ctx, e->u.call.args[1], ETY_UNKNOWN);
              eb_cstr(ctx->b, ") ? INT32_C(0) : INT32_C(1))");
            }
            break;
          }
          EmitScalarTy cmp_hint = infer_expr_scalar(ctx, e->u.call.args[0]);
          if (cmp_hint == ETY_UNKNOWN) cmp_hint = infer_expr_scalar(ctx, e->u.call.args[1]);
          if (cmp_hint == ETY_UNKNOWN) cmp_hint = ETY_U64;
          if (ctx->pre != NULL) {
            uint32_t t0 = hoist_arg(ctx, e->u.call.args[0], NULL, ctx->uidx, cmp_hint);
            uint32_t t1 = hoist_arg(ctx, e->u.call.args[1], NULL, ctx->uidx, cmp_hint);
            eb_fmt(ctx->b, "((_a%u) == (_a%u) ? INT32_C(0) : INT32_C(1))", t0, t1);
          } else {
            eb_cstr(ctx->b, "((");
            emit_expr_typed(ctx, e->u.call.args[0], cmp_hint);
            eb_cstr(ctx->b, ") == (");
            emit_expr_typed(ctx, e->u.call.args[1], cmp_hint);
            eb_cstr(ctx->b, ") ? INT32_C(0) : INT32_C(1))");
          }
          break;
        }

        /* Builtin integer conversions & bitwise / wrapping ops (emitter.md 4) */
        if (str_eq_c(cn, "U64.ofU8") && e->u.call.arg_count == 1) {
          eb_cstr(ctx->b, "((uint64_t)("); emit_expr_typed(ctx, e->u.call.args[0], ETY_U8); eb_cstr(ctx->b, "))"); break;
        }
        if (str_eq_c(cn, "U64.ofU32") && e->u.call.arg_count == 1) {
          eb_cstr(ctx->b, "((uint64_t)("); emit_expr_typed(ctx, e->u.call.args[0], ETY_U32); eb_cstr(ctx->b, "))"); break;
        }
        if (str_eq_c(cn, "U64.ofI64") && e->u.call.arg_count == 1) {
          eb_cstr(ctx->b, "((uint64_t)("); emit_expr_typed(ctx, e->u.call.args[0], ETY_I64); eb_cstr(ctx->b, "))"); break;
        }
        if (str_eq_c(cn, "U64.ofI32") && e->u.call.arg_count == 1) {
          eb_cstr(ctx->b, "((uint64_t)("); emit_expr_typed(ctx, e->u.call.args[0], ETY_I32); eb_cstr(ctx->b, "))"); break;
        }
        if (str_eq_c(cn, "U8.ofU64") && e->u.call.arg_count == 1) {
          eb_cstr(ctx->b, "((uint8_t)("); emit_expr_typed(ctx, e->u.call.args[0], ETY_U64); eb_cstr(ctx->b, "))"); break;
        }
        if (str_eq_c(cn, "U32.ofU64") && e->u.call.arg_count == 1) {
          eb_cstr(ctx->b, "((uint32_t)("); emit_expr_typed(ctx, e->u.call.args[0], ETY_U64); eb_cstr(ctx->b, "))"); break;
        }
        if (str_eq_c(cn, "I64.ofU64") && e->u.call.arg_count == 1) {
          eb_cstr(ctx->b, "((int64_t)("); emit_expr_typed(ctx, e->u.call.args[0], ETY_U64); eb_cstr(ctx->b, "))"); break;
        }
        if (str_eq_c(cn, "I32.ofU64") && e->u.call.arg_count == 1) {
          eb_cstr(ctx->b, "((int32_t)(uint32_t)("); emit_expr_typed(ctx, e->u.call.args[0], ETY_U64); eb_cstr(ctx->b, "))"); break;
        }
        if (str_eq_c(cn, "U64.mix") && e->u.call.arg_count == 1) {
          eb_cstr(ctx->b, "stigi_u64_mix("); emit_expr_typed(ctx, e->u.call.args[0], ETY_U64); eb_cstr(ctx->b, ")"); break;
        }
        if (e->u.call.arg_count == 2) {
          const char *bop = NULL;
          EmitScalarTy bty = ETY_U64;
          if (str_eq_c(cn, "U64.bitwiseAnd")) { bop = "&"; bty = ETY_U64; }
          else if (str_eq_c(cn, "U64.bitwiseOr")) { bop = "|"; bty = ETY_U64; }
          else if (str_eq_c(cn, "U64.bitwiseXor")) { bop = "^"; bty = ETY_U64; }
          else if (str_eq_c(cn, "U64.shiftLeft")) { bop = "<<"; bty = ETY_U64; }
          else if (str_eq_c(cn, "U64.shiftRight")) { bop = ">>"; bty = ETY_U64; }
          else if (str_eq_c(cn, "U64.wrappingAdd")) { bop = "+"; bty = ETY_U64; }
          else if (str_eq_c(cn, "U64.wrappingSubtract")) { bop = "-"; bty = ETY_U64; }
          else if (str_eq_c(cn, "U64.wrappingMultiply")) { bop = "*"; bty = ETY_U64; }
          else if (str_eq_c(cn, "U32.bitwiseAnd")) { bop = "&"; bty = ETY_U32; }
          else if (str_eq_c(cn, "U32.bitwiseOr")) { bop = "|"; bty = ETY_U32; }
          else if (str_eq_c(cn, "U32.bitwiseXor")) { bop = "^"; bty = ETY_U32; }
          else if (str_eq_c(cn, "U32.shiftLeft")) { bop = "<<"; bty = ETY_U32; }
          else if (str_eq_c(cn, "U32.shiftRight")) { bop = ">>"; bty = ETY_U32; }
          else if (str_eq_c(cn, "U32.wrappingAdd")) { bop = "+"; bty = ETY_U32; }
          else if (str_eq_c(cn, "U32.wrappingSubtract")) { bop = "-"; bty = ETY_U32; }
          else if (str_eq_c(cn, "U32.wrappingMultiply")) { bop = "*"; bty = ETY_U32; }
          if (bop != NULL) {
            eb_cstr(ctx->b, "((");
            emit_expr_typed(ctx, e->u.call.args[0], bty);
            eb_fmt(ctx->b, ") %s (", bop);
            emit_expr_typed(ctx, e->u.call.args[1], bty);
            eb_cstr(ctx->b, "))");
            break;
          }
          if (str_eq_c(cn, "U32.rotateRight")) {
            eb_cstr(ctx->b, "stigi_u32_rotr(");
            emit_expr_typed(ctx, e->u.call.args[0], ETY_U32);
            eb_cstr(ctx->b, ", ");
            emit_expr_typed(ctx, e->u.call.args[1], ETY_U32);
            eb_cstr(ctx->b, ")");
            break;
          }
        }

        /* Capability operation call `binder.op(args...)` or unnamed `Cap.op(args...)` (S125) */
        const char *dot = NULL;
        for (size_t i = 0; i < cn.len; i++) {
          if (cn.ptr[i] == '.') dot = cn.ptr + i;
        }
        if (dot != NULL) {
          StigiStr bname = {cn.ptr, (size_t)(dot - cn.ptr)};
          StigiStr opname = {dot + 1, cn.len - bname.len - 1U};
          const EmitVar *bv = emit_find_var(ctx, bname);
          if (bv == NULL || (bv->handler_uidx < 0 && bv->cap_uidx < 0)) {
            int32_t ctgt = stigi_image_resolve_name(ctx->img, ctx->uidx, bname.ptr, bname.len);
            if (ctgt >= 0 && ctx->img->units[ctgt].ast->decl->kind == STIGI_DECL_CAPABILITY) {
              bv = emit_find_cap_var(ctx, ctgt);
            }
          }
          if (bv != NULL && bv->handler_uidx >= 0 && bv->handler_state_var != NULL) {
            const StigiImageUnit *hu = &ctx->img->units[bv->handler_uidx];
            const StigiDecl *hd = hu->ast->decl;
            const StigiDefHead *oph = NULL;
            for (uint32_t m = 0; m < hd->u.handler_decl.def_count; m++) {
              const StigiDecl *md = hd->u.handler_decl.defs[m];
              if (md->kind == STIGI_DECL_DEF && str_eq_ss(md->name, opname)) {
                oph = &md->u.def_decl.head;
                break;
              }
            }
            uint32_t argc = e->u.call.arg_count;
            uint32_t *tids = NULL;
            bool hoisted = (ctx->pre != NULL && argc >= 2U);
            if (hoisted) {
              tids = malloc((size_t)argc * sizeof *tids);
              if (tids == NULL) { ctx->b->oom = true; hoisted = false; }
              else {
                for (uint32_t i = 0; i < argc; i++) {
                  const StigiType *pty = (oph != NULL && i < oph->param_count) ? oph->params[i].type : NULL;
                  EmitScalarTy ph = pty != NULL ? ast_type_to_scalar(ctx->img, (uint32_t)bv->handler_uidx, pty, 0) : ETY_UNKNOWN;
                  tids[i] = hoist_arg(ctx, e->u.call.args[i], pty, (uint32_t)bv->handler_uidx, ph);
                }
              }
            }
            eb_mangle_name(ctx->b, hu->name, hu->name_len);
            eb_cstr(ctx->b, "_op_");
            eb_str(ctx->b, opname);
            eb_fmt(ctx->b, "(&%s", bv->handler_state_var);
            for (uint32_t i = 0; i < argc; i++) {
              eb_cstr(ctx->b, ", ");
              if (hoisted) {
                eb_fmt(ctx->b, "_a%u", tids[i]);
              } else {
                const StigiType *pty = (oph != NULL && i < oph->param_count) ? oph->params[i].type : NULL;
                EmitScalarTy ph = pty != NULL ? ast_type_to_scalar(ctx->img, (uint32_t)bv->handler_uidx, pty, 0) : ETY_UNKNOWN;
                emit_expr_typed(ctx, e->u.call.args[i], ph);
              }
            }
            eb_cstr(ctx->b, ")");
            free(tids);
            break;
          }
          if (bv != NULL && bv->cap_uidx >= 0 && bv->cap_handle_var != NULL) {
            const StigiImageUnit *cu = &ctx->img->units[bv->cap_uidx];
            const StigiDecl *cd = cu->ast->decl;
            const StigiDefHead *oph = NULL;
            for (uint32_t m = 0; m < cd->u.cap_decl.member_count; m++) {
              const StigiDecl *md = cd->u.cap_decl.members[m];
              if (md->kind == STIGI_DECL_DEF && str_eq_ss(md->name, opname)) {
                oph = &md->u.def_decl.head;
                break;
              }
            }
            uint32_t argc = e->u.call.arg_count;
            uint32_t *tids = NULL;
            bool hoisted = (ctx->pre != NULL && argc >= 2U);
            if (hoisted) {
              tids = malloc((size_t)argc * sizeof *tids);
              if (tids == NULL) { ctx->b->oom = true; hoisted = false; }
              else {
                for (uint32_t i = 0; i < argc; i++) {
                  const StigiType *pty = (oph != NULL && i < oph->param_count) ? oph->params[i].type : NULL;
                  EmitScalarTy ph = pty != NULL ? ast_type_to_scalar(ctx->img, (uint32_t)bv->cap_uidx, pty, 0) : ETY_UNKNOWN;
                  tids[i] = hoist_arg(ctx, e->u.call.args[i], pty, (uint32_t)bv->cap_uidx, ph);
                }
              }
            }
            eb_mangle_name(ctx->b, cu->name, cu->name_len);
            eb_cstr(ctx->b, "_proto_");
            eb_str(ctx->b, opname);
            eb_fmt(ctx->b, "(%s", bv->cap_handle_var);
            for (uint32_t i = 0; i < argc; i++) {
              eb_cstr(ctx->b, ", ");
              if (hoisted) {
                eb_fmt(ctx->b, "_a%u", tids[i]);
              } else {
                const StigiType *pty = (oph != NULL && i < oph->param_count) ? oph->params[i].type : NULL;
                EmitScalarTy ph = pty != NULL ? ast_type_to_scalar(ctx->img, (uint32_t)bv->cap_uidx, pty, 0) : ETY_UNKNOWN;
                emit_expr_typed(ctx, e->u.call.args[i], ph);
              }
            }
            eb_cstr(ctx->b, ")");
            free(tids);
            break;
          }

          /* Trait operation call via uKanren (S125): direct static C call, zero dictionary */
          int32_t tr_uidx = stigi_image_resolve_name(ctx->img, ctx->uidx, bname.ptr, bname.len);
          if (tr_uidx >= 0 && ctx->img->units[tr_uidx].ast->decl->kind == STIGI_DECL_TRAIT) {
            const StigiDecl *td = ctx->img->units[tr_uidx].ast->decl;
            const StigiDecl *tr_op = NULL;
            for (uint32_t m = 0; m < td->u.trait_decl.member_count; m++) {
              if (td->u.trait_decl.members[m]->kind == STIGI_DECL_DEF &&
                  str_eq_ss(td->u.trait_decl.members[m]->name, opname)) {
                tr_op = td->u.trait_decl.members[m];
                break;
              }
            }
            Arena ka;
            arena_init(&ka);
            StigiType *goal = arena_alloc(&ka, sizeof *goal);
            int32_t guidx = -1;
            if (goal != NULL) {
              memset(goal, 0, sizeof *goal);
              goal->kind = STIGI_TYPE_NAMED;
              goal->name = (StigiStr){ ctx->img->units[tr_uidx].name,
                                       (uint32_t)ctx->img->units[tr_uidx].name_len };
              uint32_t tpc = td->u.trait_decl.type_param_count;
              goal->type_arg_count = tpc;
              if (tpc > 0) {
                goal->type_args = arena_alloc(&ka, (size_t)tpc * sizeof *goal->type_args);
                for (uint32_t tp = 0; tp < tpc && goal->type_args != NULL; tp++) {
                  goal->type_args[tp] = NULL;
                  if (tp < e->u.call.type_arg_count) {
                    goal->type_args[tp] = e->u.call.type_args[tp];
                  } else if (tr_op != NULL) {
                    StigiStr tpn = td->u.trait_decl.type_params[tp].name;
                    const StigiDefHead *oh = &tr_op->u.def_decl.head;
                    for (uint32_t p = 0; p < oh->param_count && p < e->u.call.arg_count; p++) {
                      if (oh->params[p].type != NULL &&
                          oh->params[p].type->kind == STIGI_TYPE_NAMED &&
                          str_eq_ss(oh->params[p].type->name, tpn)) {
                        const StigiExpr *ae = e->u.call.args[p];
                        if (ae->kind == STIGI_EXPR_LOCAL) {
                          const EmitVar *av = emit_find_var(ctx, ae->u.ident.name);
                          if (av != NULL && av->ast_type != NULL) {
                            goal->type_args[tp] = (StigiType *)av->ast_type;
                            break;
                          }
                        }
                        int32_t ae_enum = infer_expr_enum_uidx(ctx, ae);
                        if (ae_enum >= 0) {
                          StigiType *et = arena_alloc(&ka, sizeof *et);
                          if (et != NULL) {
                            memset(et, 0, sizeof *et);
                            et->kind = STIGI_TYPE_NAMED;
                            et->name = (StigiStr){ ctx->img->units[ae_enum].name,
                                                   (uint32_t)ctx->img->units[ae_enum].name_len };
                            goal->type_args[tp] = et;
                          }
                          break;
                        }
                        EmitScalarTy ast = infer_expr_scalar(ctx, ae);
                        const char *sn = "U64";
                        switch (ast) {
                          case ETY_U8: sn = "U8"; break;
                          case ETY_U32: sn = "U32"; break;
                          case ETY_I32: sn = "I32"; break;
                          case ETY_I64: sn = "I64"; break;
                          case ETY_BOOL: sn = "Bool"; break;
                          case ETY_UNIT: sn = "Unit"; break;
                          default: sn = "U64"; break;
                        }
                        StigiType *st = arena_alloc(&ka, sizeof *st);
                        if (st != NULL) {
                          memset(st, 0, sizeof *st);
                          st->kind = STIGI_TYPE_NAMED;
                          st->name = (StigiStr){ sn, (uint32_t)strlen(sn) };
                          goal->type_args[tp] = st;
                        }
                        break;
                      }
                    }
                  }
                }
              }
              StigiKanrenProof pr = stigi_kanren_solve(ctx->img, ctx->uidx, goal, NULL, 0, NULL, 0, &ka);
              if (pr.status == STIGI_KANREN_FOUND) guidx = pr.given_uidx;
            }
            arena_free(&ka);
            if (guidx >= 0) {
              const StigiImageUnit *gu = &ctx->img->units[guidx];
              const StigiDecl *gd = gu->ast->decl;
              const StigiDefHead *oph = NULL;
              for (uint32_t m = 0; m < gd->u.given_decl.def_count; m++) {
                if (gd->u.given_decl.defs[m]->kind == STIGI_DECL_DEF &&
                    str_eq_ss(gd->u.given_decl.defs[m]->name, opname)) {
                  oph = &gd->u.given_decl.defs[m]->u.def_decl.head;
                  break;
                }
              }
              uint32_t argc = e->u.call.arg_count;
              uint32_t *tids = NULL;
              bool hoisted = (ctx->pre != NULL && argc >= 2U);
              if (hoisted) {
                tids = malloc((size_t)argc * sizeof *tids);
                if (tids == NULL) { ctx->b->oom = true; hoisted = false; }
                else {
                  for (uint32_t i = 0; i < argc; i++) {
                    const StigiType *pty = (oph != NULL && i < oph->param_count) ? oph->params[i].type : NULL;
                    EmitScalarTy ph = pty != NULL ? ast_type_to_scalar(ctx->img, (uint32_t)guidx, pty, 0) : ETY_UNKNOWN;
                    tids[i] = hoist_arg(ctx, e->u.call.args[i], pty, (uint32_t)guidx, ph);
                  }
                }
              }
              eb_mangle_name(ctx->b, gu->name, gu->name_len);
              eb_cstr(ctx->b, "_");
              eb_str(ctx->b, opname);
              eb_cstr(ctx->b, "(");
              for (uint32_t i = 0; i < argc; i++) {
                if (i > 0) eb_cstr(ctx->b, ", ");
                if (hoisted) {
                  eb_fmt(ctx->b, "_a%u", tids[i]);
                } else {
                  const StigiType *pty = (oph != NULL && i < oph->param_count) ? oph->params[i].type : NULL;
                  EmitScalarTy ph = pty != NULL ? ast_type_to_scalar(ctx->img, (uint32_t)guidx, pty, 0) : ETY_UNKNOWN;
                  emit_expr_typed(ctx, e->u.call.args[i], ph);
                }
              }
              eb_cstr(ctx->b, ")");
              free(tids);
              break;
            }
          }
        }

        /* Enum constructor call (qualified or unqualified; allow enum Foo: case Foo(...)) */
        uint32_t cidx = 0;
        int32_t eidx = find_enum_case_owner_det(ctx->img, ctx->uidx, cn, &cidx);
        int32_t direct_tgt = stigi_image_resolve_name(ctx->img, ctx->uidx, cn.ptr, cn.len);
        if ((direct_tgt < 0 || ctx->img->units[direct_tgt].ast->decl->kind == STIGI_DECL_ENUM) &&
            eidx >= 0) {
          emit_enum_ctor_expr(ctx, eidx, cidx, e);
          break;
        }
      }

      /* Regular def / extern call: evaluate arguments left-to-right into temporaries */
      int32_t tgt = -1;
      const StigiDefHead *callee_h = NULL;
      bool callee_is_bodyless_extern = false;
      const StigiDecl *callee_d = NULL;
      const char *mono_suffix = NULL;
      const StigiStr *mono_call_tp_names = NULL;
      const StigiType *const *mono_call_tp_vals = NULL;
      uint32_t mono_call_tp_count = 0;
      if (e->u.call.callee->kind == STIGI_EXPR_LOCAL ||
          e->u.call.callee->kind == STIGI_EXPR_GLOBAL) {
        tgt = stigi_image_resolve_name(ctx->img, ctx->uidx,
                                       e->u.call.callee->u.ident.name.ptr,
                                       e->u.call.callee->u.ident.name.len);
        if (tgt >= 0) {
          callee_d = ctx->img->units[tgt].ast->decl;
          /* a generic C-DSL unit is monomorphized per type argument like a generic def (emitter.md 3: C types come
             from the call site, with A resolved) */
          bool generic_dsl = callee_d->kind == STIGI_DECL_EXTERN && !callee_d->u.extern_decl.is_val &&
                             callee_d->u.extern_decl.body != NULL &&
                             callee_d->u.extern_decl.head.type_param_count > 0;
          if (callee_d->kind == STIGI_DECL_DEF || generic_dsl) {
            callee_h = generic_dsl ? &callee_d->u.extern_decl.head : &callee_d->u.def_decl.head;
            if (callee_h->type_param_count > 0 && ctx->mono_list != NULL && ctx->mono_arena != NULL) {
              uint32_t tpc = callee_h->type_param_count;
              StigiStr *tp_names = arena_alloc(ctx->mono_arena, (size_t)tpc * sizeof *tp_names);
              const StigiType **tp_vals = malloc((size_t)tpc * sizeof *tp_vals);
              if (tp_names != NULL && tp_vals != NULL) {
                for (uint32_t tp = 0; tp < tpc; tp++) {
                  tp_names[tp] = callee_h->type_params[tp].name;
                  tp_vals[tp] = NULL;
                  if (tp < e->u.call.type_arg_count && e->u.call.type_args[tp] != NULL) {
                    tp_vals[tp] = mono_subst_type(ctx->mono_arena, ctx->mono_tp_names, ctx->mono_tp_vals,
                                                  ctx->mono_tp_count, e->u.call.type_args[tp]);
                  } else {
                    for (uint32_t p = 0; p < callee_h->param_count && p < e->u.call.arg_count; p++) {
                      /* A as the element type of an Array[A](n) parameter: the element type of the argument */
                      const StigiType *pt = callee_h->params[p].type;
                      const StigiExpr *pa = e->u.call.args[p];
                      if (pt != NULL && pt->kind == STIGI_TYPE_ARRAY && pt->type_arg_count == 1 &&
                          pt->type_args[0]->kind == STIGI_TYPE_NAMED &&
                          str_eq_ss(pt->type_args[0]->name, tp_names[tp]) && pa->kind == STIGI_EXPR_LOCAL) {
                        const EmitVar *av = emit_find_var(ctx, pa->u.ident.name);
                        if (av != NULL && av->ast_type != NULL && av->ast_type->kind == STIGI_TYPE_ARRAY &&
                            av->ast_type->type_arg_count == 1) {
                          tp_vals[tp] = mono_subst_type(ctx->mono_arena, ctx->mono_tp_names, ctx->mono_tp_vals,
                                                        ctx->mono_tp_count, av->ast_type->type_args[0]);
                          break;
                        }
                        /* an array bound by a pattern (`Filled(xs)`) carries its element type, not an Array type */
                        StigiType *et = av != NULL && av->is_array ? arena_alloc(ctx->mono_arena, sizeof *et) : NULL;
                        if (et != NULL) {
                          memset(et, 0, sizeof *et);
                          et->kind = STIGI_TYPE_NAMED;
                          et->name = av->elem_enum_uidx >= 0
                                         ? (StigiStr){ctx->img->units[av->elem_enum_uidx].name,
                                                      (uint32_t)ctx->img->units[av->elem_enum_uidx].name_len}
                                         : (StigiStr){scalar_type_name(av->elem_scalar_ty),
                                                      (uint32_t)strlen(scalar_type_name(av->elem_scalar_ty))};
                          tp_vals[tp] = et;
                          break;
                        }
                      }
                      if (callee_h->params[p].type != NULL &&
                          callee_h->params[p].type->kind == STIGI_TYPE_NAMED &&
                          str_eq_ss(callee_h->params[p].type->name, tp_names[tp])) {
                        const StigiExpr *ae = e->u.call.args[p];
                        if (ae->kind == STIGI_EXPR_LOCAL) {
                          const EmitVar *av = emit_find_var(ctx, ae->u.ident.name);
                          if (av != NULL && av->ast_type != NULL) {
                            tp_vals[tp] = mono_subst_type(ctx->mono_arena, ctx->mono_tp_names, ctx->mono_tp_vals,
                                                          ctx->mono_tp_count, av->ast_type);
                            break;
                          }
                        }
                        int32_t ae_enum = infer_expr_enum_uidx(ctx, ae);
                        if (ae_enum >= 0) {
                          StigiType *et = arena_alloc(ctx->mono_arena, sizeof *et);
                          if (et != NULL) {
                            memset(et, 0, sizeof *et);
                            et->kind = STIGI_TYPE_NAMED;
                            et->name = (StigiStr){ ctx->img->units[ae_enum].name,
                                                   (uint32_t)ctx->img->units[ae_enum].name_len };
                            tp_vals[tp] = et;
                          }
                          break;
                        }
                        EmitScalarTy ast = infer_expr_scalar(ctx, ae);
                        const char *sn = "U64";
                        switch (ast) {
                          case ETY_U8: sn = "U8"; break;
                          case ETY_U32: sn = "U32"; break;
                          case ETY_I32: sn = "I32"; break;
                          case ETY_I64: sn = "I64"; break;
                          case ETY_BOOL: sn = "Bool"; break;
                          case ETY_UNIT: sn = "Unit"; break;
                          default: sn = "U64"; break;
                        }
                        StigiType *st = arena_alloc(ctx->mono_arena, sizeof *st);
                        if (st != NULL) {
                          memset(st, 0, sizeof *st);
                          st->kind = STIGI_TYPE_NAMED;
                          st->name = (StigiStr){ sn, (uint32_t)strlen(sn) };
                          tp_vals[tp] = st;
                        }
                        break;
                      }
                    }
                  }
                }
                EmitBuf kb = {NULL, 0, 0, false};
                for (uint32_t tp = 0; tp < tpc; tp++) {
                  eb_cstr(&kb, "__");
                  if (tp_vals[tp] != NULL && tp_vals[tp]->name.len > 0) {
                    for (uint32_t ci = 0; ci < tp_vals[tp]->name.len; ci++) {
                      char ch = tp_vals[tp]->name.ptr[ci] == '.' ? '_' : tp_vals[tp]->name.ptr[ci];
                      eb_mem(&kb, &ch, 1);
                    }
                  } else {
                    eb_cstr(&kb, "U64");
                  }
                }
                if (!kb.oom && kb.buf != NULL) {
                  int32_t found_mi = -1;
                  for (uint32_t mi = 0; mi < ctx->mono_list->count; mi++) {
                    if (ctx->mono_list->items[mi].uidx == (uint32_t)tgt &&
                        strcmp(ctx->mono_list->items[mi].key, kb.buf) == 0) {
                      found_mi = (int32_t)mi;
                      break;
                    }
                  }
                  if (found_mi >= 0) {
                    free(kb.buf);
                    free(tp_vals);
                    mono_suffix = ctx->mono_list->items[found_mi].key;
                    mono_call_tp_names = ctx->mono_list->items[found_mi].tp_names;
                    mono_call_tp_vals = ctx->mono_list->items[found_mi].tp_vals;
                    mono_call_tp_count = ctx->mono_list->items[found_mi].tp_count;
                  } else {
                    if (ctx->mono_list->count == ctx->mono_list->cap) {
                      uint32_t ncap = ctx->mono_list->cap == 0 ? 8U : ctx->mono_list->cap * 2U;
                      MonoInst *ni = realloc(ctx->mono_list->items, (size_t)ncap * sizeof *ni);
                      if (ni != NULL) {
                        ctx->mono_list->items = ni;
                        ctx->mono_list->cap = ncap;
                      }
                    }
                    if (ctx->mono_list->count < ctx->mono_list->cap) {
                      MonoInst *ins = &ctx->mono_list->items[ctx->mono_list->count++];
                      ins->uidx = (uint32_t)tgt;
                      ins->key = kb.buf;
                      ins->tp_names = tp_names;
                      ins->tp_vals = tp_vals;
                      ins->tp_count = tpc;
                      mono_suffix = ins->key;
                      mono_call_tp_names = ins->tp_names;
                      mono_call_tp_vals = ins->tp_vals;
                      mono_call_tp_count = ins->tp_count;
                    } else {
                      free(kb.buf);
                      free(tp_vals);
                    }
                  }
                } else {
                  free(kb.buf);
                  free(tp_vals);
                }
              } else {
                free(tp_vals);
              }
            }
          } else if (callee_d->kind == STIGI_DECL_EXTERN && !callee_d->u.extern_decl.is_val) {
            callee_h = &callee_d->u.extern_decl.head;
            callee_is_bodyless_extern = (callee_d->u.extern_decl.body == NULL);
          }
        }
      }
      uint32_t argc = e->u.call.arg_count;
      uint32_t *tids = NULL;
      bool hoisted = (ctx->pre != NULL && argc >= 2U && !callee_is_bodyless_extern);
      if (hoisted) {
        tids = malloc((size_t)argc * sizeof *tids);
        if (tids == NULL) {
          ctx->b->oom = true;
          hoisted = false;
        } else {
          for (uint32_t i = 0; i < argc; i++) {
            const StigiType *pty = (callee_h != NULL && i < callee_h->param_count)
                                       ? callee_h->params[i].type
                                       : NULL;
            if (mono_call_tp_count > 0 && pty != NULL) {
              pty = mono_subst_type(ctx->mono_arena, mono_call_tp_names, mono_call_tp_vals, mono_call_tp_count, pty);
            }
            uint32_t puidx = tgt >= 0 ? (uint32_t)tgt : ctx->uidx;
            EmitScalarTy ph = pty != NULL ? ast_type_to_scalar(ctx->img, puidx, pty, 0) : ETY_UNKNOWN;
            tids[i] = hoist_arg(ctx, e->u.call.args[i], pty, puidx, ph);
          }
        }
      }
      if (tgt >= 0) {
        if (callee_is_bodyless_extern) {
          eb_str(ctx->b, extern_c_sym(callee_d->name));
        } else {
          eb_mangle_name(ctx->b, ctx->img->units[tgt].name, ctx->img->units[tgt].name_len);
          if (mono_suffix != NULL) eb_cstr(ctx->b, mono_suffix);
        }
      } else if (e->u.call.callee->kind == STIGI_EXPR_LOCAL ||
                 e->u.call.callee->kind == STIGI_EXPR_GLOBAL) {
        eb_c_ident(ctx->b, e->u.call.callee->u.ident.name);
      } else {
        emit_expr_typed(ctx, e->u.call.callee, ETY_UNKNOWN);
      }
      const char *posix_cast = NULL;
      StigiStr ext_sym = (tgt >= 0 && callee_is_bodyless_extern) ? extern_c_sym(callee_d->name)
                                                                  : (StigiStr){NULL, 0};
      eb_cstr(ctx->b, "(");
      bool first_call_arg = true;
      if (callee_h != NULL && !callee_is_bodyless_extern && tgt >= 0) {
        for (uint32_t g = 0; g < callee_h->pre_group_count; g++) {
          if (callee_h->pre_groups[g].is_fact) continue;
          for (uint32_t c = 0; c < callee_h->pre_groups[g].cap_count; c++) {
            const StigiType *ct = callee_h->pre_groups[g].caps[c].cap_type;
            if (ct == NULL) continue;
            int32_t cidx = stigi_image_resolve_name(ctx->img, (uint32_t)tgt, ct->name.ptr, ct->name.len);
            if (cidx >= 0 && cap_has_runtime_handle(ctx->img, (uint32_t)cidx)) {
              const EmitVar *cv = NULL;
              for (uint32_t ca = 0; ca < e->u.call.cap_arg_count; ca++) {
                const EmitVar *ev = emit_find_var(ctx, e->u.call.cap_args[ca]);
                if (ev != NULL && ev->cap_uidx == cidx && ev->cap_handle_var != NULL) {
                  cv = ev;
                  break;
                }
              }
              if (cv == NULL) cv = emit_find_cap_var(ctx, cidx);
              if (!first_call_arg) eb_cstr(ctx->b, ", ");
              first_call_arg = false;
              if (cv != NULL && cv->cap_handle_var != NULL) {
                eb_cstr(ctx->b, cv->cap_handle_var);
              } else {
                eb_cstr(ctx->b, "(");
                eb_mangle_name(ctx->b, ctx->img->units[cidx].name, ctx->img->units[cidx].name_len);
                eb_cstr(ctx->b, "_Handle){0}");
              }
            }
          }
        }
      }
      for (uint32_t i = 0; i < argc; i++) {
        if (!first_call_arg) eb_cstr(ctx->b, ", ");
        first_call_arg = false;
        posix_cast = NULL;
        if (callee_is_bodyless_extern) {
          if (str_eq_c(ext_sym, "openat") && i == 1) posix_cast = "(const char *)";
          else if (str_eq_c(ext_sym, "unlink") && i == 0) posix_cast = "(const char *)";
          else if (str_eq_c(ext_sym, "getrusage") && i == 1) posix_cast = "(struct rusage *)(void *)";
          else if (str_eq_c(ext_sym, "clock_gettime") && i == 0) posix_cast = "(clockid_t)";
          else if (str_eq_c(ext_sym, "clock_gettime") && i == 1) posix_cast = "(struct timespec *)(void *)";
          else if (str_eq_c(ext_sym, "bind") && i == 1) posix_cast = "(const struct sockaddr *)(void *)";
          else if (str_eq_c(ext_sym, "bind") && i == 2) posix_cast = "(socklen_t)";
          else if (str_eq_c(ext_sym, "accept") && i == 1) posix_cast = "(struct sockaddr *)(void *)";
          else if (str_eq_c(ext_sym, "accept") && i == 2) posix_cast = "(socklen_t *)(void *)";
          else if (str_eq_c(ext_sym, "read") && i == 1) posix_cast = "(void *)";
          else if (str_eq_c(ext_sym, "read") && i == 2) posix_cast = "(size_t)";
          else if (str_eq_c(ext_sym, "write") && i == 1) posix_cast = "(const void *)";
          else if (str_eq_c(ext_sym, "write") && i == 2) posix_cast = "(size_t)";
          else if (str_eq_c(ext_sym, "lseek") && i == 1) posix_cast = "(off_t)";
        }
        if (posix_cast != NULL) {
          eb_cstr(ctx->b, posix_cast);
          eb_cstr(ctx->b, "(");
        }
        if (hoisted) {
          eb_fmt(ctx->b, "_a%u", tids[i]);
        } else {
          const StigiType *pty = (callee_h != NULL && i < callee_h->param_count)
                                     ? callee_h->params[i].type
                                     : NULL;
          uint32_t puidx = tgt >= 0 ? (uint32_t)tgt : ctx->uidx;
          /* C-DSL rule 1 (emitter.md 3): `xs(i)` passed to `Ptr[A]` param of body-less extern -> `&xs[i]` */
          bool param_is_ptr = false;
          if (callee_is_bodyless_extern && pty != NULL) {
            uint32_t ru = puidx;
            const StigiType *rt = resolve_typedef_type(ctx->img, &ru, pty, 0);
            param_is_ptr = (rt != NULL && rt->kind == STIGI_TYPE_PTR);
          }
          const StigiExpr *arg = e->u.call.args[i];
          if (param_is_ptr && arg->kind == STIGI_EXPR_CALL &&
              arg->u.call.callee->kind == STIGI_EXPR_LOCAL && arg->u.call.arg_count == 1) {
            const EmitVar *av = emit_find_var(ctx, arg->u.call.callee->u.ident.name);
            if (av != NULL && av->is_array) {
              eb_cstr(ctx->b, "(&");
              emit_var_cname(ctx->b, av);
              eb_cstr(ctx->b, "[");
              emit_expr_typed(ctx, arg->u.call.args[0], ETY_U64);
              eb_cstr(ctx->b, "])");
              if (posix_cast != NULL) eb_cstr(ctx->b, ")");
              continue;
            }
          }
          if (param_is_ptr && arg->kind == STIGI_EXPR_INDEX_READ) {
            eb_cstr(ctx->b, "(&(");
            emit_expr_typed(ctx, arg->u.index_expr.target, ETY_UNKNOWN);
            eb_cstr(ctx->b, ")[");
            emit_expr_typed(ctx, arg->u.index_expr.index, ETY_U64);
            eb_cstr(ctx->b, "])");
            if (posix_cast != NULL) eb_cstr(ctx->b, ")");
            continue;
          }
          EmitScalarTy ph = pty != NULL ? ast_type_to_scalar(ctx->img, puidx, pty, 0) : ETY_UNKNOWN;
          emit_expr_typed(ctx, arg, ph);
        }
        if (posix_cast != NULL) eb_cstr(ctx->b, ")");
      }
      eb_cstr(ctx->b, ")");
      free(tids);
      break;
    }
    /* Expression-position complex constructs: hoist into temp via ctx->pre (NEVER drop to 0!) */
    case STIGI_EXPR_MATCH:
    case STIGI_EXPR_BLOCK:
    case STIGI_EXPR_HANDLE:
      if (ctx->pre != NULL) {
        if (e->kind == STIGI_EXPR_BLOCK && e->u.block.count == 0) {
          eb_cstr(ctx->b, "UINT64_C(0)");
          break;
        }
        if (e->kind == STIGI_EXPR_BLOCK && e->u.block.count == 1 &&
            !expr_is_complex_stmt(e->u.block.stmts[0])) {
          emit_expr_typed(ctx, e->u.block.stmts[0], hint);
          break;
        }
        uint32_t tid = ctx->tmp_id++;
        char ename[32];
        (void)snprintf(ename, sizeof ename, "_e%u", tid);
        EmitBuf *saved_b = ctx->b;
        EmitBuf *saved_pre = ctx->pre;
        ctx->b = saved_pre;
        ctx->pre = NULL;
        eb_cstr(ctx->b, "  ");
        (void)emit_inferred_expr_c_type(ctx->b, ctx, e, hint);
        eb_fmt(ctx->b, " %s = {0};\n", ename);
        emit_assign_to(ctx, e, ename, hint);
        ctx->b = saved_b;
        ctx->pre = saved_pre;
        eb_cstr(ctx->b, ename);
        break;
      }
      if (e->kind == STIGI_EXPR_BLOCK && e->u.block.count == 1) {
        emit_expr_typed(ctx, e->u.block.stmts[0], hint);
        break;
      }
      if (e->kind == STIGI_EXPR_HANDLE && e->u.handle_stmt.body != NULL) {
        emit_expr_typed(ctx, e->u.handle_stmt.body, hint);
        break;
      }
      if (e->kind == STIGI_EXPR_MATCH || (e->kind == STIGI_EXPR_BLOCK && e->u.block.count > 1)) {
        /* a match or block value with no place for its statements is refused, never emitted as a constant */
        ctx->fail_closed = true;
        (void)snprintf(ctx->fail_msg, sizeof ctx->fail_msg, "a match or block value has no place for its statements");
      }
      eb_cstr(ctx->b, "UINT64_C(0)");
      break;
    case STIGI_EXPR_WHILE:
    case STIGI_EXPR_VAL:
    case STIGI_EXPR_VAR:
    case STIGI_EXPR_ASSIGN:
    case STIGI_EXPR_INDEX_WRITE:
      if (ctx->pre != NULL) {
        EmitBuf *saved_b = ctx->b;
        EmitBuf *saved_pre = ctx->pre;
        ctx->b = saved_pre;
        ctx->pre = NULL;
        emit_stmt_or_ret(ctx, e, false, true);
        ctx->b = saved_b;
        ctx->pre = saved_pre;
      }
      eb_cstr(ctx->b, "UINT64_C(0)");
      break;
    case STIGI_EXPR_DENY:
    case STIGI_EXPR_LOCAL_DECL:
      eb_cstr(ctx->b, "UINT64_C(0)");
      break;
    case STIGI_EXPR_IS:
      /* the elaborator rewrites every `is` into a match (S129); one that reaches the emitter is refused */
      ctx->fail_closed = true;
      (void)snprintf(ctx->fail_msg, sizeof ctx->fail_msg, "an `is` expression reached the emitter unelaborated");
      eb_cstr(ctx->b, "false");
      break;
  }
}

typedef struct {
  const StigiImageUnit *u;
} UnitRef;

/* Deterministic ordering strictly by 32-byte runtime_id (emitter.md 1, NEVER by pointer) */
static int cmp_unit_by_id(const void *pa, const void *pb) {
  const UnitRef *a = pa;
  const UnitRef *b = pb;
  int c = memcmp(a->u->unit_id, b->u->unit_id, 32);
  if (c != 0) return c;
  return strcmp(a->u->name, b->u->name);
}

/* Mark enum + transitively any field enums that need Stigi_<Enum>_eq */
static void mark_enum_needs_eq(const StigiImage *img, uint32_t enum_uidx, bool *enum_needs_eq) {
  if (enum_uidx >= img->unit_count || enum_needs_eq[enum_uidx]) return;
  const StigiDecl *d = img->units[enum_uidx].ast->decl;
  if (d->kind != STIGI_DECL_ENUM) return;
  enum_needs_eq[enum_uidx] = true;
  for (uint32_t c = 0; c < d->u.enum_decl.case_count; c++) {
    const StigiEnumCase *ec = &d->u.enum_decl.cases[c];
    for (uint32_t f = 0; f < ec->field_count; f++) {
      uint32_t ru = enum_uidx;
      const StigiType *rt = resolve_typedef_type(img, &ru, ec->fields[f].type, 0);
      if (rt != NULL && rt->kind == STIGI_TYPE_NAMED) {
        int32_t ft = stigi_image_resolve_name(img, ru, rt->name.ptr, rt->name.len);
        if (ft >= 0 && img->units[ft].ast->decl->kind == STIGI_DECL_ENUM) {
          mark_enum_needs_eq(img, (uint32_t)ft, enum_needs_eq);
        }
      }
    }
  }
}

/* Scan AST to check runtime helpers, enum_eq usage, and pure parallel-safe call sites */
static void scan_expr_helpers(const StigiImage *img, uint32_t uidx, const StigiExpr *e,
                              const UnitMaskInfo *mask_infos,
                              bool *need_mix, bool *need_rotr, bool *need_alloc,
                              bool *enum_needs_eq, uint32_t *parallel_sites) {
  if (e == NULL) return;
  switch (e->kind) {
    case STIGI_EXPR_CALL:
      if (e->u.call.callee->kind == STIGI_EXPR_GLOBAL || e->u.call.callee->kind == STIGI_EXPR_LOCAL) {
        StigiStr cn = e->u.call.callee->u.ident.name;
        if (str_eq_c(cn, "U64.mix")) *need_mix = true;
        else if (str_eq_c(cn, "U32.rotateRight")) *need_rotr = true;
        else if (str_eq_c(cn, "Array.tryFill") || str_eq_c(cn, "Array.tryGrow") ||
                 str_eq_c(cn, "Cell.tryMake")) {
          *need_alloc = true;
        } else if (str_eq_c(cn, "Stigi.Test.expect") && e->u.call.arg_count == 3 &&
                   enum_needs_eq != NULL) {
          EmitCtx tmp_c;
          memset(&tmp_c, 0, sizeof tmp_c);
          tmp_c.img = img;
          tmp_c.uidx = uidx;
          int32_t e0 = infer_expr_enum_uidx(&tmp_c, e->u.call.args[0]);
          int32_t e1 = infer_expr_enum_uidx(&tmp_c, e->u.call.args[1]);
          if (e0 >= 0) mark_enum_needs_eq(img, (uint32_t)e0, enum_needs_eq);
          if (e1 >= 0) mark_enum_needs_eq(img, (uint32_t)e1, enum_needs_eq);
        } else if (mask_infos != NULL && parallel_sites != NULL) {
          int32_t tgt = stigi_image_resolve_name(img, uidx, cn.ptr, cn.len);
          if (tgt >= 0 && mask_infos[tgt].is_pure_def) {
            (*parallel_sites)++;
          }
        }
      }
      for (uint32_t i = 0; i < e->u.call.arg_count; i++) {
        scan_expr_helpers(img, uidx, e->u.call.args[i], mask_infos, need_mix, need_rotr,
                          need_alloc, enum_needs_eq, parallel_sites);
      }
      break;
    case STIGI_EXPR_BINOP:
      if ((e->u.binop.op == STIGI_OP_EQ || e->u.binop.op == STIGI_OP_NE) && enum_needs_eq != NULL) {
        /* Mark all enums referenced directly in == / != comparisons */
        EmitCtx tmp_c;
        memset(&tmp_c, 0, sizeof tmp_c);
        tmp_c.img = img;
        tmp_c.uidx = uidx;
        int32_t e0 = infer_expr_enum_uidx(&tmp_c, e->u.binop.lhs);
        int32_t e1 = infer_expr_enum_uidx(&tmp_c, e->u.binop.rhs);
        if (e0 >= 0) mark_enum_needs_eq(img, (uint32_t)e0, enum_needs_eq);
        if (e1 >= 0) mark_enum_needs_eq(img, (uint32_t)e1, enum_needs_eq);
      }
      scan_expr_helpers(img, uidx, e->u.binop.lhs, mask_infos, need_mix, need_rotr, need_alloc,
                        enum_needs_eq, parallel_sites);
      scan_expr_helpers(img, uidx, e->u.binop.rhs, mask_infos, need_mix, need_rotr, need_alloc,
                        enum_needs_eq, parallel_sites);
      break;
    case STIGI_EXPR_UNOP:
      scan_expr_helpers(img, uidx, e->u.unop.operand, mask_infos, need_mix, need_rotr, need_alloc,
                        enum_needs_eq, parallel_sites);
      break;
    case STIGI_EXPR_IF:
      scan_expr_helpers(img, uidx, e->u.if_expr.cond, mask_infos, need_mix, need_rotr, need_alloc,
                        enum_needs_eq, parallel_sites);
      scan_expr_helpers(img, uidx, e->u.if_expr.then_br, mask_infos, need_mix, need_rotr, need_alloc,
                        enum_needs_eq, parallel_sites);
      scan_expr_helpers(img, uidx, e->u.if_expr.else_br, mask_infos, need_mix, need_rotr, need_alloc,
                        enum_needs_eq, parallel_sites);
      break;
    case STIGI_EXPR_WHILE:
      scan_expr_helpers(img, uidx, e->u.while_expr.cond, mask_infos, need_mix, need_rotr, need_alloc,
                        enum_needs_eq, parallel_sites);
      scan_expr_helpers(img, uidx, e->u.while_expr.body, mask_infos, need_mix, need_rotr, need_alloc,
                        enum_needs_eq, parallel_sites);
      break;
    case STIGI_EXPR_MATCH:
      scan_expr_helpers(img, uidx, e->u.match_expr.scrutinee, mask_infos, need_mix, need_rotr,
                        need_alloc, enum_needs_eq, parallel_sites);
      for (uint32_t i = 0; i < e->u.match_expr.arm_count; i++) {
        scan_expr_helpers(img, uidx, e->u.match_expr.arms[i].body, mask_infos, need_mix, need_rotr,
                          need_alloc, enum_needs_eq, parallel_sites);
      }
      break;
    case STIGI_EXPR_BLOCK:
      for (uint32_t i = 0; i < e->u.block.count; i++) {
        scan_expr_helpers(img, uidx, e->u.block.stmts[i], mask_infos, need_mix, need_rotr, need_alloc,
                          enum_needs_eq, parallel_sites);
      }
      break;
    case STIGI_EXPR_VAL:
      scan_expr_helpers(img, uidx, e->u.val_stmt.init, mask_infos, need_mix, need_rotr, need_alloc,
                        enum_needs_eq, parallel_sites);
      break;
    case STIGI_EXPR_VAR:
      scan_expr_helpers(img, uidx, e->u.var_stmt.init, mask_infos, need_mix, need_rotr, need_alloc,
                        enum_needs_eq, parallel_sites);
      break;
    case STIGI_EXPR_ASSIGN:
      scan_expr_helpers(img, uidx, e->u.assign_stmt.value, mask_infos, need_mix, need_rotr, need_alloc,
                        enum_needs_eq, parallel_sites);
      break;
    case STIGI_EXPR_INDEX_READ:
    case STIGI_EXPR_INDEX_WRITE:
      scan_expr_helpers(img, uidx, e->u.index_expr.target, mask_infos, need_mix, need_rotr, need_alloc,
                        enum_needs_eq, parallel_sites);
      scan_expr_helpers(img, uidx, e->u.index_expr.index, mask_infos, need_mix, need_rotr, need_alloc,
                        enum_needs_eq, parallel_sites);
      scan_expr_helpers(img, uidx, e->u.index_expr.value, mask_infos, need_mix, need_rotr, need_alloc,
                        enum_needs_eq, parallel_sites);
      break;
    case STIGI_EXPR_HANDLE:
      for (uint32_t i = 0; i < e->u.handle_stmt.arg_count; i++) {
        scan_expr_helpers(img, uidx, e->u.handle_stmt.args[i], mask_infos, need_mix, need_rotr,
                          need_alloc, enum_needs_eq, parallel_sites);
      }
      scan_expr_helpers(img, uidx, e->u.handle_stmt.body, mask_infos, need_mix, need_rotr, need_alloc,
                        enum_needs_eq, parallel_sites);
      break;
    case STIGI_EXPR_ASSERT:
      scan_expr_helpers(img, uidx, e->u.assert_expr.expr, mask_infos, need_mix, need_rotr, need_alloc,
                        enum_needs_eq, parallel_sites);
      break;
    default:
      break;
  }
}

static void emit_def_sig_head_mono(EmitBuf *b, const StigiImage *img, uint32_t uidx,
                                   const StigiDefHead *h, const char *name, size_t name_len,
                                   bool is_static, bool is_inline, const char *extra_first_param,
                                   const char *mono_suffix, const StigiStr *tp_names,
                                   const StigiType *const *tp_vals, uint32_t tp_count, Arena *arena) {
  if (is_static) eb_cstr(b, "static ");
  if (is_inline) eb_cstr(b, "inline ");
  const StigiType *rty = mono_subst_type(arena, tp_names, tp_vals, tp_count, h->ret_type);
  emit_c_type(b, img, uidx, rty, 0);
  eb_cstr(b, " ");
  eb_mangle_name(b, name, name_len);
  if (mono_suffix != NULL) eb_cstr(b, mono_suffix);
  eb_cstr(b, "(");
  bool first = true;
  if (extra_first_param != NULL) {
    eb_cstr(b, extra_first_param);
    first = false;
  }
  uint32_t cap_param_seq = 0;
  for (uint32_t g = 0; g < h->pre_group_count; g++) {
    if (h->pre_groups[g].is_fact) continue;
    for (uint32_t c = 0; c < h->pre_groups[g].cap_count; c++) {
      const StigiType *ct = h->pre_groups[g].caps[c].cap_type;
      if (ct == NULL) continue;
      int32_t cidx = stigi_image_resolve_name(img, uidx, ct->name.ptr, ct->name.len);
      if (cidx >= 0 && cap_has_runtime_handle(img, (uint32_t)cidx)) {
        if (!first) eb_cstr(b, ", ");
        first = false;
        eb_mangle_name(b, img->units[cidx].name, img->units[cidx].name_len);
        eb_fmt(b, "_Handle _cap%u", cap_param_seq++);
      }
    }
  }
  if (h->param_count == 0 && first) {
    eb_cstr(b, "void");
  } else {
    for (uint32_t i = 0; i < h->param_count; i++) {
      if (!first) eb_cstr(b, ", ");
      first = false;
      const StigiType *pty = mono_subst_type(arena, tp_names, tp_vals, tp_count, h->params[i].type);
      emit_c_type(b, img, uidx, pty, 0);
      eb_cstr(b, " ");
      eb_c_ident(b, h->params[i].name);
    }
  }
  eb_cstr(b, ")");
}

static void emit_def_sig_head(EmitBuf *b, const StigiImage *img, uint32_t uidx,
                              const StigiDefHead *h, const char *name, size_t name_len,
                              bool is_static, bool is_inline, const char *extra_first_param) {
  emit_def_sig_head_mono(b, img, uidx, h, name, name_len, is_static, is_inline,
                         extra_first_param, NULL, NULL, NULL, 0, NULL);
}

/* Emit enum in lockstep with stigi_enum_layout_info (memory.md 8):
 * 1-word nullable pointer, tail-padding tag union, tag-only, or standard struct+union */
static void emit_enum_unit(EmitBuf *b, const StigiImage *img, const StigiImageUnit *u,
                           const StigiDecl *d, bool emit_eq) {
  StigiEnumLayoutInfo eli = stigi_enum_layout_info(img, u->idx, NULL, 0);
  uint32_t case_count = d->u.enum_decl.case_count;
  const char *tag_ctype = (eli.tag_size == 1ULL) ? "uint8_t"
                          : (eli.tag_size == 2ULL ? "uint16_t" : "uint32_t");

  eb_line_dir(b, d->span.line, u->name, u->name_len);

  if (eli.is_nullable_ptr) {
    const StigiEnumCase *pc = &d->u.enum_decl.cases[eli.ptr_case];
    eb_cstr(b, "typedef ");
    emit_c_type(b, img, u->idx, pc->fields[0].type, 0);
    eb_cstr(b, " ");
    eb_mangle_name(b, u->name, u->name_len);
    eb_cstr(b, ";\n_Static_assert(sizeof(");
    eb_mangle_name(b, u->name, u->name_len);
    eb_cstr(b, ") == 8ULL, \"nullable-ptr enum sizeOf mismatch\");\n_Static_assert(_Alignof(");
    eb_mangle_name(b, u->name, u->name_len);
    eb_cstr(b, ") == 8ULL, \"nullable-ptr enum alignOf mismatch\");\n");
    if (emit_eq) {
      eb_cstr(b, "static inline bool ");
      eb_mangle_name(b, u->name, u->name_len);
      eb_cstr(b, "_eq(");
      eb_mangle_name(b, u->name, u->name_len);
      eb_cstr(b, " _a, ");
      eb_mangle_name(b, u->name, u->name_len);
      eb_cstr(b, " _b) { return _a == _b; }\n");
    }
    eb_cstr(b, "\n");
    return;
  }

  eb_cstr(b, "typedef enum {\n");
  for (uint32_t c = 0; c < case_count; c++) {
    const StigiEnumCase *ec = &d->u.enum_decl.cases[c];
    eb_cstr(b, "  ");
    eb_mangle_name(b, u->name, u->name_len);
    eb_cstr(b, "_");
    eb_str(b, ec->name);
    eb_fmt(b, " = %u%s\n", c, (c + 1 < case_count) ? "," : "");
  }
  eb_cstr(b, "} ");
  eb_mangle_name(b, u->name, u->name_len);
  eb_cstr(b, "_Tag;\n");

  EmitBuf asserts_buf;
  memset(&asserts_buf, 0, sizeof asserts_buf);

  if (eli.tag_in_tail) {
    /* Tag placed inside tail padding of case union (zero extra stride!) */
    eb_cstr(b, "typedef union {\n");
    eb_fmt(b, "  struct { uint8_t _tag_pad[%llu]; %s tag; } _hdr;\n",
           (unsigned long long)eli.tag_offset, tag_ctype);
    for (uint32_t c = 0; c < case_count; c++) {
      const StigiEnumCase *ec = &d->u.enum_decl.cases[c];
      if (ec->field_count == 0) continue;
      StigiLayout *flays = malloc((size_t)ec->field_count * sizeof *flays);
      uint32_t *order = malloc((size_t)ec->field_count * sizeof *order);
      if (flays == NULL || order == NULL) {
        free(flays);
        free(order);
        b->oom = true;
        free(asserts_buf.buf);
        return;
      }
      uint32_t non_empty = 0;
      for (uint32_t f = 0; f < ec->field_count; f++) {
        flays[f] = ast_type_layout(img, u->idx, ec->fields[f].type, 0);
        order[f] = f;
        if (flays[f].size > 0) non_empty++;
      }
      if (non_empty == 0) {
        free(flays);
        free(order);
        continue;
      }
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
      eb_cstr(b, "  struct {\n");
      uint64_t off = 0;
      for (uint32_t i = 0; i < ec->field_count; i++) {
        uint32_t fi = order[i];
        StigiLayout fl = flays[fi];
        if (fl.size == 0) continue;
        off = emit_align_up(off, fl.align);
        uint64_t field_off = off;
        off += fl.size;
        eb_cstr(b, "    ");
        emit_c_type(b, img, u->idx, ec->fields[fi].type, 0);
        eb_cstr(b, " ");
        eb_c_ident(b, ec->fields[fi].name);
        eb_cstr(b, ";\n");

        eb_cstr(&asserts_buf, "_Static_assert(offsetof(");
        eb_mangle_name(&asserts_buf, u->name, u->name_len);
        eb_cstr(&asserts_buf, ", ");
        eb_str(&asserts_buf, ec->name);
        eb_cstr(&asserts_buf, ".");
        eb_c_ident(&asserts_buf, ec->fields[fi].name);
        eb_fmt(&asserts_buf, ") == %lluULL, \"enum field offset mismatch\");\n",
               (unsigned long long)field_off);
      }
      if (eli.tag_offset > off) {
        eb_fmt(b, "    uint8_t _tag_pad[%llu];\n", (unsigned long long)(eli.tag_offset - off));
      }
      eb_fmt(b, "    %s tag;\n  } ", tag_ctype);
      eb_str(b, ec->name);
      eb_cstr(b, ";\n");

      eb_cstr(&asserts_buf, "_Static_assert(offsetof(");
      eb_mangle_name(&asserts_buf, u->name, u->name_len);
      eb_cstr(&asserts_buf, ", ");
      eb_str(&asserts_buf, ec->name);
      eb_fmt(&asserts_buf, ".tag) == %lluULL, \"enum tail tag offset mismatch\");\n",
             (unsigned long long)eli.tag_offset);
      free(flays);
      free(order);
    }
    eb_cstr(b, "} ");
    eb_mangle_name(b, u->name, u->name_len);
    eb_cstr(b, ";\n");
  } else {
    eb_cstr(b, "typedef struct {\n");
    if (!eli.is_tag_only) {
      eb_cstr(b, "  union {\n");
      for (uint32_t c = 0; c < case_count; c++) {
        const StigiEnumCase *ec = &d->u.enum_decl.cases[c];
        if (ec->field_count == 0) continue;
        StigiLayout *flays = malloc((size_t)ec->field_count * sizeof *flays);
        uint32_t *order = malloc((size_t)ec->field_count * sizeof *order);
        if (flays == NULL || order == NULL) {
          free(flays);
          free(order);
          b->oom = true;
          free(asserts_buf.buf);
          return;
        }
        uint32_t non_empty = 0;
        for (uint32_t f = 0; f < ec->field_count; f++) {
          flays[f] = ast_type_layout(img, u->idx, ec->fields[f].type, 0);
          order[f] = f;
          if (flays[f].size > 0) non_empty++;
        }
        if (non_empty == 0) {
          free(flays);
          free(order);
          continue;
        }
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
        eb_cstr(b, "    struct {\n");
        uint64_t off = 0;
        for (uint32_t i = 0; i < ec->field_count; i++) {
          uint32_t fi = order[i];
          StigiLayout fl = flays[fi];
          if (fl.size == 0) continue;
          off = emit_align_up(off, fl.align);
          uint64_t field_off = off;
          off += fl.size;
          eb_cstr(b, "      ");
          emit_c_type(b, img, u->idx, ec->fields[fi].type, 0);
          eb_cstr(b, " ");
          eb_c_ident(b, ec->fields[fi].name);
          eb_cstr(b, ";\n");

          eb_cstr(&asserts_buf, "_Static_assert(offsetof(");
          eb_mangle_name(&asserts_buf, u->name, u->name_len);
          eb_cstr(&asserts_buf, ", u.");
          eb_str(&asserts_buf, ec->name);
          eb_cstr(&asserts_buf, ".");
          eb_c_ident(&asserts_buf, ec->fields[fi].name);
          eb_fmt(&asserts_buf, ") == %lluULL, \"enum field offset mismatch\");\n",
                 (unsigned long long)field_off);
        }
        eb_cstr(b, "    } ");
        eb_str(b, ec->name);
        eb_cstr(b, ";\n");
        free(flays);
        free(order);
      }
      eb_cstr(b, "  } u;\n");
    }
    eb_fmt(b, "  %s tag;\n} ", tag_ctype);
    eb_mangle_name(b, u->name, u->name_len);
    eb_cstr(b, ";\n");
  }

  eb_cstr(b, "_Static_assert(sizeof(");
  eb_mangle_name(b, u->name, u->name_len);
  eb_fmt(b, ") == %lluULL, \"enum sizeOf mismatch\");\n", (unsigned long long)eli.layout.size);
  eb_cstr(b, "_Static_assert(_Alignof(");
  eb_mangle_name(b, u->name, u->name_len);
  eb_fmt(b, ") == %lluULL, \"enum alignOf mismatch\");\n", (unsigned long long)eli.layout.align);
  eb_cstr(b, "_Static_assert(offsetof(");
  eb_mangle_name(b, u->name, u->name_len);
  eb_fmt(b, ", %s) == %lluULL, \"enum tag offset mismatch\");\n",
         eli.tag_in_tail ? "_hdr.tag" : "tag", (unsigned long long)eli.tag_offset);
  if (asserts_buf.len > 0) eb_mem(b, asserts_buf.buf, asserts_buf.len);
  if (asserts_buf.oom) b->oom = true;
  free(asserts_buf.buf);

  /* Emit structural equality helper ONLY when actually compared (zero -Wunused-function) */
  if (!emit_eq) {
    eb_cstr(b, "\n");
    return;
  }
  eb_cstr(b, "static inline bool ");
  eb_mangle_name(b, u->name, u->name_len);
  eb_cstr(b, "_eq(");
  eb_mangle_name(b, u->name, u->name_len);
  eb_cstr(b, " _a, ");
  eb_mangle_name(b, u->name, u->name_len);
  eb_fmt(b, " _b) {\n  if (_a.%s != _b.%s) return false;\n",
         eli.tag_in_tail ? "_hdr.tag" : "tag", eli.tag_in_tail ? "_hdr.tag" : "tag");
  if (!eli.is_tag_only) {
    eb_fmt(b, "  switch (_a.%s) {\n", eli.tag_in_tail ? "_hdr.tag" : "tag");
    for (uint32_t c = 0; c < case_count; c++) {
      const StigiEnumCase *ec = &d->u.enum_decl.cases[c];
      eb_cstr(b, "    case ");
      eb_mangle_name(b, u->name, u->name_len);
      eb_cstr(b, "_");
      eb_str(b, ec->name);
      eb_cstr(b, ": return true");
      for (uint32_t f = 0; f < ec->field_count; f++) {
        if (ast_type_layout(img, u->idx, ec->fields[f].type, 0).size == 0) continue;
        uint32_t ru = u->idx;
        const StigiType *rft = resolve_typedef_type(img, &ru, ec->fields[f].type, 0);
        int32_t fe_uidx = -1;
        if (rft != NULL && rft->kind == STIGI_TYPE_NAMED) {
          int32_t ft = stigi_image_resolve_name(img, ru, rft->name.ptr, rft->name.len);
          if (ft >= 0 && img->units[ft].ast->decl->kind == STIGI_DECL_ENUM) fe_uidx = ft;
        }
        eb_cstr(b, " && ");
        if (fe_uidx >= 0) {
          eb_mangle_name(b, img->units[fe_uidx].name, img->units[fe_uidx].name_len);
          eb_fmt(b, "_eq(_a.%s", eli.tag_in_tail ? "" : "u.");
          eb_str(b, ec->name);
          eb_cstr(b, ".");
          eb_c_ident(b, ec->fields[f].name);
          eb_fmt(b, ", _b.%s", eli.tag_in_tail ? "" : "u.");
          eb_str(b, ec->name);
          eb_cstr(b, ".");
          eb_c_ident(b, ec->fields[f].name);
          eb_cstr(b, ")");
        } else {
          eb_fmt(b, "(_a.%s", eli.tag_in_tail ? "" : "u.");
          eb_str(b, ec->name);
          eb_cstr(b, ".");
          eb_c_ident(b, ec->fields[f].name);
          eb_fmt(b, " == _b.%s", eli.tag_in_tail ? "" : "u.");
          eb_str(b, ec->name);
          eb_cstr(b, ".");
          eb_c_ident(b, ec->fields[f].name);
          eb_cstr(b, ")");
        }
      }
      eb_cstr(b, ";\n");
    }
    eb_cstr(b, "  }\n");
  }
  eb_cstr(b, "  return true;\n}\n\n");
}

static bool cap_has_proto_caller(const StigiImage *img, uint32_t cap_uidx) {
  for (uint32_t i = 0; i < img->unit_count; i++) {
    if (!img->units[i].reachable) continue;
    const StigiDecl *d = img->units[i].ast->decl;
    const StigiDefHead *h = NULL;
    if (d->kind == STIGI_DECL_DEF) h = &d->u.def_decl.head;
    else if (d->kind == STIGI_DECL_EXTERN && !d->u.extern_decl.is_val &&
             d->u.extern_decl.body != NULL) {
      h = &d->u.extern_decl.head;
    }
    if (h != NULL) {
      for (uint32_t g = 0; g < h->pre_group_count; g++) {
        if (h->pre_groups[g].is_fact) continue;
        for (uint32_t c = 0; c < h->pre_groups[g].cap_count; c++) {
          const StigiType *ct = h->pre_groups[g].caps[c].cap_type;
          if (ct == NULL) continue;
          int32_t cidx = stigi_image_resolve_name(img, i, ct->name.ptr, ct->name.len);
          if (cidx == (int32_t)cap_uidx) return true;
        }
      }
    }
  }
  return false;
}

static bool expr_refs_unit(const StigiImage *img, uint32_t from_uidx, const StigiExpr *e,
                           uint32_t target_uidx) {
  if (e == NULL) return false;
  switch (e->kind) {
    case STIGI_EXPR_GLOBAL:
    case STIGI_EXPR_LOCAL:
      return stigi_image_resolve_name(img, from_uidx, e->u.ident.name.ptr, e->u.ident.name.len) ==
             (int32_t)target_uidx;
    case STIGI_EXPR_UNOP:
      return expr_refs_unit(img, from_uidx, e->u.unop.operand, target_uidx);
    case STIGI_EXPR_BINOP:
      return expr_refs_unit(img, from_uidx, e->u.binop.lhs, target_uidx) ||
             expr_refs_unit(img, from_uidx, e->u.binop.rhs, target_uidx);
    case STIGI_EXPR_CALL:
      if (expr_refs_unit(img, from_uidx, e->u.call.callee, target_uidx)) return true;
      for (uint32_t i = 0; i < e->u.call.arg_count; i++) {
        if (expr_refs_unit(img, from_uidx, e->u.call.args[i], target_uidx)) return true;
      }
      return false;
    case STIGI_EXPR_IF:
      return expr_refs_unit(img, from_uidx, e->u.if_expr.cond, target_uidx) ||
             expr_refs_unit(img, from_uidx, e->u.if_expr.then_br, target_uidx) ||
             expr_refs_unit(img, from_uidx, e->u.if_expr.else_br, target_uidx);
    case STIGI_EXPR_WHILE:
      return expr_refs_unit(img, from_uidx, e->u.while_expr.cond, target_uidx) ||
             expr_refs_unit(img, from_uidx, e->u.while_expr.body, target_uidx);
    case STIGI_EXPR_MATCH:
      if (expr_refs_unit(img, from_uidx, e->u.match_expr.scrutinee, target_uidx)) return true;
      for (uint32_t i = 0; i < e->u.match_expr.arm_count; i++) {
        if (expr_refs_unit(img, from_uidx, e->u.match_expr.arms[i].guard, target_uidx) ||
            expr_refs_unit(img, from_uidx, e->u.match_expr.arms[i].body, target_uidx)) {
          return true;
        }
      }
      return false;
    case STIGI_EXPR_BLOCK:
      for (uint32_t i = 0; i < e->u.block.count; i++) {
        if (expr_refs_unit(img, from_uidx, e->u.block.stmts[i], target_uidx)) return true;
      }
      return false;
    case STIGI_EXPR_VAL:
      return expr_refs_unit(img, from_uidx, e->u.val_stmt.init, target_uidx);
    case STIGI_EXPR_VAR:
      return expr_refs_unit(img, from_uidx, e->u.var_stmt.init, target_uidx);
    case STIGI_EXPR_ASSIGN:
      return expr_refs_unit(img, from_uidx, e->u.assign_stmt.value, target_uidx);
    case STIGI_EXPR_INDEX_READ:
    case STIGI_EXPR_INDEX_WRITE:
      return expr_refs_unit(img, from_uidx, e->u.index_expr.target, target_uidx) ||
             expr_refs_unit(img, from_uidx, e->u.index_expr.index, target_uidx) ||
             expr_refs_unit(img, from_uidx, e->u.index_expr.value, target_uidx);
    case STIGI_EXPR_HANDLE:
      for (uint32_t i = 0; i < e->u.handle_stmt.arg_count; i++) {
        if (expr_refs_unit(img, from_uidx, e->u.handle_stmt.args[i], target_uidx)) return true;
      }
      return expr_refs_unit(img, from_uidx, e->u.handle_stmt.body, target_uidx);
    case STIGI_EXPR_ASSERT:
      return expr_refs_unit(img, from_uidx, e->u.assert_expr.expr, target_uidx);
    default:
      return false;
  }
}

static bool val_used_at_runtime(const StigiImage *img, uint32_t val_uidx) {
  for (uint32_t i = 0; i < img->unit_count; i++) {
    if (!img->units[i].reachable || i == val_uidx) continue;
    const StigiDecl *d = img->units[i].ast->decl;
    if (d->kind == STIGI_DECL_DEF) {
      if (expr_refs_unit(img, i, stigi_def_body(d), val_uidx)) return true;
    } else if (d->kind == STIGI_DECL_VAL || d->kind == STIGI_DECL_TEST) {
      if (expr_refs_unit(img, i, stigi_val_body(d), val_uidx)) return true;
    } else if (d->kind == STIGI_DECL_EXTERN && !d->u.extern_decl.is_val &&
               d->u.extern_decl.body != NULL) {
      if (expr_refs_unit(img, i, d->u.extern_decl.body, val_uidx)) return true;
    } else if (d->kind == STIGI_DECL_HANDLER) {
      for (uint32_t s = 0; s < d->u.handler_decl.var_count; s++) {
        if (expr_refs_unit(img, i, d->u.handler_decl.vars[s], val_uidx)) return true;
      }
      for (uint32_t m = 0; m < d->u.handler_decl.def_count; m++) {
        if (expr_refs_unit(img, i, stigi_def_body(d->u.handler_decl.defs[m]), val_uidx)) return true;
      }
    } else if (d->kind == STIGI_DECL_GIVEN) {
      for (uint32_t m = 0; m < d->u.given_decl.def_count; m++) {
        if (expr_refs_unit(img, i, stigi_def_body(d->u.given_decl.defs[m]), val_uidx)) return true;
      }
    }
  }
  return false;
}

/* Topological emission of ENUM / VAL units (deterministic tie-break by runtime_id order) */
static void topo_order_kind(const StigiImage *img, const UnitRef *sorted, uint32_t scnt,
                            StigiDeclKind kind, uint32_t *out_indices, uint32_t *out_count) {
  uint8_t *state = calloc(img->unit_count, 1); /* 0=unvisited, 1=visiting, 2=done */
  bool *in_set = calloc(img->unit_count, 1);
  if (state == NULL || in_set == NULL) {
    free(state);
    free(in_set);
    *out_count = 0;
    return;
  }
  for (uint32_t i = 0; i < scnt; i++) {
    if (sorted[i].u->ast->decl->kind == kind) in_set[sorted[i].u->idx] = true;
  }
  uint32_t oc = 0;
  uint32_t *stack = malloc((size_t)img->unit_count * 2U * sizeof *stack);
  uint32_t *edge_cur = malloc((size_t)img->unit_count * sizeof *edge_cur);
  if (stack == NULL || edge_cur == NULL) {
    for (uint32_t i = 0; i < scnt; i++) {
      if (sorted[i].u->ast->decl->kind == kind) out_indices[oc++] = sorted[i].u->idx;
    }
    *out_count = oc;
    free(state);
    free(in_set);
    free(stack);
    free(edge_cur);
    return;
  }
  for (uint32_t i = 0; i < img->unit_count; i++) {
    edge_cur[i] = (img->adj_head != NULL && i < img->adj_cap) ? img->adj_head[i] : UINT32_MAX;
  }
  for (uint32_t i = 0; i < scnt; i++) {
    uint32_t root = sorted[i].u->idx;
    if (!in_set[root] || state[root] != 0) continue;
    uint32_t sp = 0;
    stack[sp++] = root;
    state[root] = 1;
    while (sp > 0) {
      uint32_t cur = stack[sp - 1];
      bool pushed = false;
      while (edge_cur[cur] != UINT32_MAX) {
        uint32_t e = edge_cur[cur];
        edge_cur[cur] = img->edge_next[e];
        uint32_t dep = img->edges[e].to;
        if (dep < img->unit_count && in_set[dep] && state[dep] == 0) {
          state[dep] = 1;
          stack[sp++] = dep;
          pushed = true;
          break;
        }
      }
      if (!pushed) {
        state[cur] = 2;
        out_indices[oc++] = cur;
        sp--;
      }
    }
  }
  *out_count = oc;
  free(state);
  free(in_set);
  free(stack);
  free(edge_cur);
}

static void emit_handler_state_typedef(EmitBuf *b, const StigiImage *img,
                                       const StigiImageUnit *u, const StigiDecl *d) {
  eb_line_dir(b, d->span.line, u->name, u->name_len);
  eb_cstr(b, "typedef struct {\n");
  uint32_t field_cnt = 0;
  for (uint32_t i = 0; i < d->u.handler_decl.val_param_count; i++) {
    if (ast_type_is_void(img, u->idx, d->u.handler_decl.val_params[i].type)) continue;
    eb_cstr(b, "  ");
    emit_c_type(b, img, u->idx, d->u.handler_decl.val_params[i].type, 0);
    eb_cstr(b, " ");
    eb_c_ident(b, d->u.handler_decl.val_params[i].name);
    eb_cstr(b, ";\n");
    field_cnt++;
  }
  for (uint32_t m = 0; m < d->u.handler_decl.var_count; m++) {
    const StigiExpr *ve = d->u.handler_decl.vars[m];
    if (ve->kind == STIGI_EXPR_VAR &&
        (ve->u.var_stmt.type == NULL || !ast_type_is_void(img, u->idx, ve->u.var_stmt.type))) {
      eb_cstr(b, "  ");
      emit_c_type(b, img, u->idx, ve->u.var_stmt.type, 0);
      eb_cstr(b, " ");
      eb_c_ident(b, ve->u.var_stmt.name);
      eb_cstr(b, ";\n");
      field_cnt++;
    }
  }
  if (field_cnt == 0) eb_cstr(b, "  uint8_t _empty;\n");
  eb_cstr(b, "} ");
  eb_mangle_name(b, u->name, u->name_len);
  eb_cstr(b, "_State;\n");
}

static void emit_cap_handle_typedef(EmitBuf *b, const StigiImage *img, const StigiImageUnit *cu) {
  if (!cap_has_runtime_handle(img, cu->idx)) return;
  eb_cstr(b, "typedef struct {\n  uint8_t tag;\n  union {\n");
  int32_t tag = 0;
  for (uint32_t i = 0; i < img->unit_count; i++) {
    if (!img->units[i].reachable) continue;
    const StigiDecl *hd = img->units[i].ast->decl;
    if (hd->kind != STIGI_DECL_HANDLER) continue;
    int32_t fc = stigi_image_resolve_name(img, i, hd->u.handler_decl.for_cap.ptr,
                                          hd->u.handler_decl.for_cap.len);
    if (fc == (int32_t)cu->idx) {
      eb_cstr(b, "    ");
      eb_mangle_name(b, img->units[i].name, img->units[i].name_len);
      eb_fmt(b, "_State *h%d;\n", tag++);
    }
  }
  eb_cstr(b, "  } ptr;\n} ");
  eb_mangle_name(b, cu->name, cu->name_len);
  eb_cstr(b, "_Handle;\n");
}

static bool expr_calls_cap_op(const StigiImage *img, uint32_t uidx, const StigiExpr *e,
                              StigiStr binder, StigiStr cap_name, StigiStr op_name) {
  if (e == NULL) return false;
  switch (e->kind) {
    case STIGI_EXPR_CALL: {
      if (e->u.call.callee != NULL &&
          (e->u.call.callee->kind == STIGI_EXPR_LOCAL || e->u.call.callee->kind == STIGI_EXPR_GLOBAL)) {
        StigiStr cn = e->u.call.callee->u.ident.name;
        const char *dot = memchr(cn.ptr, '.', cn.len);
        if (dot != NULL) {
          StigiStr bn = {cn.ptr, (size_t)(dot - cn.ptr)};
          StigiStr on = {dot + 1, cn.len - bn.len - 1U};
          if (str_eq_ss(on, op_name) &&
              ((binder.len > 0 && str_eq_ss(bn, binder)) || str_eq_ss(bn, cap_name))) {
            return true;
          }
          /* Qualified Cap.op when Cap itself has dots */
          const char *ldot = NULL;
          for (size_t k = 0; k < cn.len; k++) if (cn.ptr[k] == '.') ldot = cn.ptr + k;
          if (ldot != NULL) {
            StigiStr qbn = {cn.ptr, (size_t)(ldot - cn.ptr)};
            StigiStr qon = {ldot + 1, cn.len - qbn.len - 1U};
            if (str_eq_ss(qon, op_name) && str_eq_ss(qbn, cap_name)) return true;
          }
        }
      }
      if (expr_calls_cap_op(img, uidx, e->u.call.callee, binder, cap_name, op_name)) return true;
      for (uint32_t i = 0; i < e->u.call.arg_count; i++) {
        if (expr_calls_cap_op(img, uidx, e->u.call.args[i], binder, cap_name, op_name)) return true;
      }
      return false;
    }
    case STIGI_EXPR_UNOP:
      return expr_calls_cap_op(img, uidx, e->u.unop.operand, binder, cap_name, op_name);
    case STIGI_EXPR_BINOP:
      return expr_calls_cap_op(img, uidx, e->u.binop.lhs, binder, cap_name, op_name) ||
             expr_calls_cap_op(img, uidx, e->u.binop.rhs, binder, cap_name, op_name);
    case STIGI_EXPR_IF:
      return expr_calls_cap_op(img, uidx, e->u.if_expr.cond, binder, cap_name, op_name) ||
             expr_calls_cap_op(img, uidx, e->u.if_expr.then_br, binder, cap_name, op_name) ||
             expr_calls_cap_op(img, uidx, e->u.if_expr.else_br, binder, cap_name, op_name);
    case STIGI_EXPR_WHILE:
      return expr_calls_cap_op(img, uidx, e->u.while_expr.cond, binder, cap_name, op_name) ||
             expr_calls_cap_op(img, uidx, e->u.while_expr.body, binder, cap_name, op_name);
    case STIGI_EXPR_MATCH:
      if (expr_calls_cap_op(img, uidx, e->u.match_expr.scrutinee, binder, cap_name, op_name)) return true;
      for (uint32_t i = 0; i < e->u.match_expr.arm_count; i++) {
        if (expr_calls_cap_op(img, uidx, e->u.match_expr.arms[i].guard, binder, cap_name, op_name) ||
            expr_calls_cap_op(img, uidx, e->u.match_expr.arms[i].body, binder, cap_name, op_name)) {
          return true;
        }
      }
      return false;
    case STIGI_EXPR_BLOCK:
      for (uint32_t i = 0; i < e->u.block.count; i++) {
        if (expr_calls_cap_op(img, uidx, e->u.block.stmts[i], binder, cap_name, op_name)) return true;
      }
      return false;
    case STIGI_EXPR_VAL:
      return expr_calls_cap_op(img, uidx, e->u.val_stmt.init, binder, cap_name, op_name);
    case STIGI_EXPR_VAR:
      return expr_calls_cap_op(img, uidx, e->u.var_stmt.init, binder, cap_name, op_name);
    case STIGI_EXPR_ASSIGN:
      return expr_calls_cap_op(img, uidx, e->u.assign_stmt.value, binder, cap_name, op_name);
    case STIGI_EXPR_INDEX_READ:
    case STIGI_EXPR_INDEX_WRITE:
      return expr_calls_cap_op(img, uidx, e->u.index_expr.target, binder, cap_name, op_name) ||
             expr_calls_cap_op(img, uidx, e->u.index_expr.index, binder, cap_name, op_name) ||
             expr_calls_cap_op(img, uidx, e->u.index_expr.value, binder, cap_name, op_name);
    case STIGI_EXPR_HANDLE:
      return expr_calls_cap_op(img, uidx, e->u.handle_stmt.body, binder, cap_name, op_name);
    case STIGI_EXPR_ASSERT:
      return expr_calls_cap_op(img, uidx, e->u.assert_expr.expr, binder, cap_name, op_name);
    default:
      return false;
  }
}

static bool cap_op_has_proto_caller(const StigiImage *img, uint32_t cap_uidx, StigiStr op_name) {
  StigiStr cname = {img->units[cap_uidx].name, img->units[cap_uidx].name_len};
  for (uint32_t i = 0; i < img->unit_count; i++) {
    if (!img->units[i].reachable) continue;
    const StigiDecl *d = img->units[i].ast->decl;
    const StigiDefHead *h = NULL;
    const StigiExpr *body = NULL;
    if (d->kind == STIGI_DECL_DEF) {
      h = &d->u.def_decl.head;
      body = stigi_def_body(d);
    } else if (d->kind == STIGI_DECL_EXTERN && !d->u.extern_decl.is_val &&
               d->u.extern_decl.body != NULL) {
      h = &d->u.extern_decl.head;
      body = d->u.extern_decl.body;
    }
    if (h == NULL || body == NULL) continue;
    for (uint32_t g = 0; g < h->pre_group_count; g++) {
      if (h->pre_groups[g].is_fact) continue;
      for (uint32_t c = 0; c < h->pre_groups[g].cap_count; c++) {
        const StigiType *ct = h->pre_groups[g].caps[c].cap_type;
        if (ct == NULL) continue;
        int32_t cidx = stigi_image_resolve_name(img, i, ct->name.ptr, ct->name.len);
        if (cidx == (int32_t)cap_uidx) {
          if (expr_calls_cap_op(img, i, body, h->pre_groups[g].caps[c].name, cname, op_name)) {
            return true;
          }
        }
      }
    }
  }
  return false;
}

static void emit_cap_proto_ops(EmitBuf *b, const StigiImage *img, const StigiImageUnit *cu) {
  if (!cap_has_runtime_handle(img, cu->idx)) return;
  const StigiDecl *cd = cu->ast->decl;
  for (uint32_t m = 0; m < cd->u.cap_decl.member_count; m++) {
    const StigiDecl *md = cd->u.cap_decl.members[m];
    if (md->kind != STIGI_DECL_DEF) continue;
    if (!cap_op_has_proto_caller(img, cu->idx, md->name)) continue;
    const StigiDefHead *oph = &md->u.def_decl.head;
    bool is_void = ast_type_is_void(img, cu->idx, oph->ret_type);
    eb_cstr(b, "static inline ");
    emit_c_type(b, img, cu->idx, oph->ret_type, 0);
    eb_cstr(b, " ");
    eb_mangle_name(b, cu->name, cu->name_len);
    eb_cstr(b, "_proto_");
    eb_str(b, md->name);
    eb_cstr(b, "(");
    eb_mangle_name(b, cu->name, cu->name_len);
    eb_cstr(b, "_Handle _h");
    for (uint32_t p = 0; p < oph->param_count; p++) {
      eb_cstr(b, ", ");
      emit_c_type(b, img, cu->idx, oph->params[p].type, 0);
      eb_cstr(b, " ");
      eb_c_ident(b, oph->params[p].name);
    }
    eb_cstr(b, ") {\n");
    int32_t tag = 0;
    for (uint32_t i = 0; i < img->unit_count; i++) {
      if (!img->units[i].reachable) continue;
      const StigiDecl *hd = img->units[i].ast->decl;
      if (hd->kind != STIGI_DECL_HANDLER) continue;
      int32_t fc = stigi_image_resolve_name(img, i, hd->u.handler_decl.for_cap.ptr,
                                            hd->u.handler_decl.for_cap.len);
      if (fc != (int32_t)cu->idx) continue;
      eb_fmt(b, "  %s(_h.tag == %dU) { ", tag == 0 ? "if " : "else if ", tag);
      if (!is_void) eb_cstr(b, "return ");
      eb_mangle_name(b, img->units[i].name, img->units[i].name_len);
      eb_cstr(b, "_op_");
      eb_str(b, md->name);
      eb_fmt(b, "(_h.ptr.h%d", tag);
      for (uint32_t p = 0; p < oph->param_count; p++) {
        eb_cstr(b, ", ");
        eb_c_ident(b, oph->params[p].name);
      }
      eb_cstr(b, ");");
      if (is_void) eb_cstr(b, " return;");
      eb_cstr(b, " }\n");
      tag++;
    }
    if (!is_void) {
      eb_cstr(b, "  ");
      emit_c_type(b, img, cu->idx, oph->ret_type, 0);
      eb_cstr(b, " _z = {0};\n  return _z;\n");
    }
    eb_cstr(b, "}\n\n");
  }
}

static void emit_handler_unit(EmitBuf *b, const StigiImage *img, const StigiImageUnit *u,
                              const StigiDecl *d, StigiEmitStats *stats,
                              const UnitMaskInfo *mask_infos) {
  eb_line_dir(b, d->span.line, u->name, u->name_len);

  /* Initializer function */
  eb_cstr(b, "static inline ");
  eb_mangle_name(b, u->name, u->name_len);
  eb_cstr(b, "_State ");
  eb_mangle_name(b, u->name, u->name_len);
  eb_cstr(b, "_init(");
  if (d->u.handler_decl.val_param_count == 0) {
    eb_cstr(b, "void");
  } else {
    for (uint32_t i = 0; i < d->u.handler_decl.val_param_count; i++) {
      if (i > 0) eb_cstr(b, ", ");
      emit_c_type(b, img, u->idx, d->u.handler_decl.val_params[i].type, 0);
      eb_cstr(b, " ");
      eb_c_ident(b, d->u.handler_decl.val_params[i].name);
    }
  }
  eb_cstr(b, ") {\n  ");
  eb_mangle_name(b, u->name, u->name_len);
  eb_cstr(b, "_State _st = {0};\n");
  EmitCtx ictx;
  memset(&ictx, 0, sizeof ictx);
  ictx.img = img;
  ictx.uidx = u->idx;
  ictx.b = b;
  ictx.stats = stats;
  for (uint32_t i = 0; i < d->u.handler_decl.val_param_count; i++) {
    EmitVar *pv = emit_register_var(&ictx, d->u.handler_decl.val_params[i].name,
                                    d->u.handler_decl.val_params[i].type, u->idx, true);
    if (pv != NULL && !ast_type_is_void(img, u->idx, d->u.handler_decl.val_params[i].type)) {
      eb_cstr(b, "  _st.");
      eb_c_ident(b, d->u.handler_decl.val_params[i].name);
      eb_cstr(b, " = ");
      emit_var_cname(b, pv);
      eb_cstr(b, ";\n");
    }
  }
  for (uint32_t m = 0; m < d->u.handler_decl.var_count; m++) {
    const StigiExpr *ve = d->u.handler_decl.vars[m];
    if (ve->kind == STIGI_EXPR_VAR &&
        (ve->u.var_stmt.type == NULL || !ast_type_is_void(img, u->idx, ve->u.var_stmt.type))) {
      EmitScalarTy vh = ast_type_to_scalar(img, u->idx, ve->u.var_stmt.type, 0);
      EmitBuf ibuf;
      emit_expr_with_pre(&ictx, ve->u.var_stmt.init, vh, &ibuf);
      eb_cstr(b, "  _st.");
      eb_c_ident(b, ve->u.var_stmt.name);
      eb_cstr(b, " = ");
      eb_mem(b, ibuf.buf, ibuf.len);
      eb_cstr(b, ";\n");
      free(ibuf.buf);
    }
  }
  emit_ctx_free(&ictx);
  eb_cstr(b, "  return _st;\n}\n");

  /* Handler operation functions */
  for (uint32_t m = 0; m < d->u.handler_decl.def_count; m++) {
    const StigiDecl *md = d->u.handler_decl.defs[m];
    if (md->kind != STIGI_DECL_DEF) continue;
    const StigiDefHead *oph = &md->u.def_decl.head;
    EmitBuf extra_p;
    memset(&extra_p, 0, sizeof extra_p);
    eb_mangle_name(&extra_p, u->name, u->name_len);
    eb_cstr(&extra_p, "_State *_hstate");

    eb_cstr(b, "static inline ");
    emit_c_type(b, img, u->idx, oph->ret_type, 0);
    eb_cstr(b, " ");
    eb_mangle_name(b, u->name, u->name_len);
    eb_cstr(b, "_op_");
    eb_str(b, md->name);
    eb_fmt(b, "(%s", extra_p.buf);
    for (uint32_t p = 0; p < oph->param_count; p++) {
      eb_cstr(b, ", ");
      emit_c_type(b, img, u->idx, oph->params[p].type, 0);
      eb_cstr(b, " ");
      eb_c_ident(b, oph->params[p].name);
    }
    eb_cstr(b, ") {\n  (void)_hstate;\n");
    for (uint32_t p = 0; p < oph->param_count; p++) {
      eb_cstr(b, "  (void)");
      eb_c_ident(b, oph->params[p].name);
      eb_cstr(b, ";\n");
    }

    EmitCtx octx;
    memset(&octx, 0, sizeof octx);
    octx.img = img;
    octx.uidx = u->idx;
    octx.cur_head = oph;
    octx.b = b;
    octx.stats = stats;
    octx.mask_info = mask_infos != NULL ? &mask_infos[u->idx] : NULL;
    octx.ret_scalar = ast_type_to_scalar(img, u->idx, oph->ret_type, 0);
    octx.ret_ast_type = oph->ret_type;
    octx.ret_type_uidx = u->idx;

    for (uint32_t i = 0; i < d->u.handler_decl.val_param_count; i++) {
      EmitVar *sv = emit_register_var(&octx, d->u.handler_decl.val_params[i].name,
                                      d->u.handler_decl.val_params[i].type, u->idx, true);
      if (sv != NULL) sv->is_handler_state_field = true;
    }
    for (uint32_t k = 0; k < d->u.handler_decl.var_count; k++) {
      const StigiExpr *ve = d->u.handler_decl.vars[k];
      if (ve->kind == STIGI_EXPR_VAR) {
        EmitVar *sv = emit_register_var(&octx, ve->u.var_stmt.name, ve->u.var_stmt.type, u->idx, true);
        if (sv != NULL) sv->is_handler_state_field = true;
      }
    }
    for (uint32_t p = 0; p < oph->param_count; p++) {
      (void)emit_register_var(&octx, oph->params[p].name, oph->params[p].type, u->idx, true);
    }
    bool is_void = ast_type_is_void(img, u->idx, oph->ret_type);
    emit_stmt_or_ret(&octx, stigi_def_body(md), true, is_void);
    emit_ctx_free(&octx);
    eb_cstr(b, "}\n");
    free(extra_p.buf);
  }
  eb_cstr(b, "\n");
}

static Status emit_c_reached(const StigiDoor *door, char **out_c, size_t *out_len,
                             StigiEmitStats *stats, Diag *diag);

/* S163 + T14: a copy of the image units whose `reachable` bit is the profile's unit set: TEST keeps
 * every root's closure, RELEASE re-roots at `def main` alone over the image edges */
static Status profile_units(const StigiImage *img, StigiEmitProfile profile, StigiImageUnit **out,
                            Diag *diag) {
  StigiImageUnit *units = malloc(((size_t)img->unit_count + 1U) * sizeof *units);
  uint32_t *queue = malloc(((size_t)img->unit_count + 1U) * sizeof *queue);
  if (units == NULL || queue == NULL) {
    free(units);
    free(queue);
    return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
  }
  if (img->unit_count > 0) memcpy(units, img->units, (size_t)img->unit_count * sizeof *units);
  if (profile == STIGI_EMIT_TEST) {
    free(queue);
    *out = units;
    return STIGI_OK;
  }
  uint32_t qh = 0, qt = 0;
  for (uint32_t i = 0; i < img->unit_count; i++) {
    units[i].reachable = img->units[i].reachable && units[i].ast->decl->kind == STIGI_DECL_DEF &&
                         strcmp(units[i].name, "main") == 0;
    if (units[i].reachable) queue[qt++] = i;
  }
  if (qt == 0) {
    free(units);
    free(queue);
    return diag_set(diag, STIGI_ERR_INVALID, -1, "release profile needs main");
  }
  while (qh < qt) {
    uint32_t cur = queue[qh++];
    for (uint32_t e = img->adj_head[cur]; e != UINT32_MAX; e = img->edge_next[e]) {
      uint32_t to = img->edges[e].to;
      if (!units[to].reachable) {
        units[to].reachable = true;
        queue[qt++] = to;
      }
    }
  }
  free(queue);
  *out = units;
  return STIGI_OK;
}

/* the emitter reads a read-only copy of the door whose units carry the profile's unit set */
Status stigi_emit_c(const StigiDoor *door, StigiEmitProfile profile, char **out_c, size_t *out_len,
                    StigiEmitStats *stats, Diag *diag) {
  if (door == NULL || out_c == NULL) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_emit_c: invalid arguments");
  }
  StigiImageUnit *units = NULL;
  Status st = profile_units(&door->image, profile, &units, diag);
  if (st != STIGI_OK) return st;
  StigiDoor *rd = malloc(sizeof *rd);
  if (rd == NULL) {
    free(units);
    return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
  }
  *rd = *door;
  rd->image.units = units;
  st = emit_c_reached(rd, out_c, out_len, stats, diag);
  free(units);
  free(rd);
  return st;
}

static Status emit_c_reached(const StigiDoor *door, char **out_c, size_t *out_len,
                             StigiEmitStats *stats, Diag *diag) {
  if (door == NULL || out_c == NULL) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_emit_c: invalid arguments");
  }
  StigiEmitStats local_stats;
  memset(&local_stats, 0, sizeof local_stats);
  if (stats == NULL) stats = &local_stats;
  else memset(stats, 0, sizeof *stats);

  const StigiImage *img = &door->image;
  UnitRef *sorted = NULL;
  uint32_t scnt = 0;
  if (img->unit_count > 0) {
    sorted = malloc((size_t)img->unit_count * sizeof *sorted);
    if (sorted == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
    for (uint32_t i = 0; i < img->unit_count; i++) {
      if (img->units[i].reachable) {
        sorted[scnt++].u = &img->units[i];
      }
    }
    qsort(sorted, scnt, sizeof *sorted, cmp_unit_by_id);
  }

  UnitMaskInfo *mask_infos = NULL;
  uint32_t total_dl_masks = 0;
  Status mst = compute_datalog_masks(door, &mask_infos, &total_dl_masks, stats, diag);
  if (mst != STIGI_OK) {
    free(sorted);
    return mst;
  }
  bool need_mask = (total_dl_masks > 0);
  bool need_mix = false, need_rotr = false, need_alloc = false;
  bool *enum_needs_eq = img->unit_count > 0 ? calloc(img->unit_count, sizeof *enum_needs_eq) : NULL;
  for (uint32_t i = 0; i < scnt; i++) {
    const StigiImageUnit *u = sorted[i].u;
    const StigiDecl *d = u->ast->decl;
    if (d->kind == STIGI_DECL_DEF) {
      scan_expr_helpers(img, u->idx, stigi_def_body(d), mask_infos, &need_mix, &need_rotr,
                        &need_alloc, enum_needs_eq, &stats->parallel_safe_sites);
    } else if (d->kind == STIGI_DECL_VAL || d->kind == STIGI_DECL_TEST) {
      scan_expr_helpers(img, u->idx, stigi_val_body(d), mask_infos, &need_mix, &need_rotr,
                        &need_alloc, enum_needs_eq, &stats->parallel_safe_sites);
    } else if (d->kind == STIGI_DECL_EXTERN && !d->u.extern_decl.is_val) {
      scan_expr_helpers(img, u->idx, d->u.extern_decl.body, mask_infos, &need_mix, &need_rotr,
                        &need_alloc, enum_needs_eq, &stats->parallel_safe_sites);
    } else if (d->kind == STIGI_DECL_HANDLER) {
      for (uint32_t m = 0; m < d->u.handler_decl.var_count; m++) {
        scan_expr_helpers(img, u->idx, d->u.handler_decl.vars[m], mask_infos, &need_mix,
                          &need_rotr, &need_alloc, enum_needs_eq, &stats->parallel_safe_sites);
      }
      for (uint32_t m = 0; m < d->u.handler_decl.def_count; m++) {
        const StigiDecl *md = d->u.handler_decl.defs[m];
        if (md->kind == STIGI_DECL_DEF) {
          scan_expr_helpers(img, u->idx, stigi_def_body(md), mask_infos, &need_mix, &need_rotr,
                            &need_alloc, enum_needs_eq, &stats->parallel_safe_sites);
        }
      }
    } else if (d->kind == STIGI_DECL_GIVEN) {
      for (uint32_t m = 0; m < d->u.given_decl.def_count; m++) {
        const StigiDecl *md = d->u.given_decl.defs[m];
        if (md->kind == STIGI_DECL_DEF) {
          scan_expr_helpers(img, u->idx, stigi_def_body(md), mask_infos, &need_mix, &need_rotr,
                            &need_alloc, enum_needs_eq, &stats->parallel_safe_sites);
        }
      }
    }
  }

  EmitBuf b;
  memset(&b, 0, sizeof b);

  bool need_posix_headers = false;
  for (uint32_t i = 0; i < scnt; i++) {
    const StigiDecl *d = sorted[i].u->ast->decl;
    if (d->kind == STIGI_DECL_EXTERN && !d->u.extern_decl.is_val &&
        d->u.extern_decl.body == NULL) {
      need_posix_headers = true;
      break;
    }
  }

  /* Portable ISO C11 preamble + S118 runtime + X22-g Spectre barrier mask (ZERO abort/exit/builtins) */
  eb_cstr(&b,
          "#define _GNU_SOURCE\n"
          "#include <stdint.h>\n"
          "#include <stdbool.h>\n"
          "#include <stddef.h>\n"
          "#include <signal.h>\n"
          "#include <sys/mman.h>\n"
          "#include <unistd.h>\n");
  if (need_posix_headers) {
    eb_cstr(&b,
            "#include <fcntl.h>\n"
            "#include <sys/socket.h>\n"
            "#include <sys/un.h>\n"
            "#include <sys/resource.h>\n"
            "#include <time.h>\n");
  }
  if (need_alloc) eb_cstr(&b, "#include <stdlib.h>\n#include <string.h>\n");
  eb_cstr(&b,
          "\n"
          "typedef enum {\n"
          "  STIGI_RT_OK = 0,\n"
          "  STIGI_RT_ERR_STACK = 1,\n"
          "  STIGI_RT_ERR_MEM_POPULATE = 2,\n"
          "  STIGI_RT_ERR_MEM_LOCK = 3,\n"
          "  STIGI_RT_ERR_SIGPIPE = 4\n"
          "} StigiRtStatus;\n\n"
          "typedef struct {\n"
          "  void *stack_raw;\n"
          "  size_t stack_total;\n"
          "  void *pool_raw;\n"
          "  size_t pool_total;\n"
          "  void *pool;\n"
          "  StigiRtStatus status;\n"
          "} StigiRtState;\n\n");

  if (need_mask) {
    eb_cstr(&b,
            "static inline uint64_t Stigi_mask_index(uint64_t idx, uint64_t len) {\n"
            "#if defined(__x86_64__)\n"
            "  uint64_t mask;\n"
            "  __asm__ __volatile__(\n"
            "    \"cmpq %2, %1\\n\\t\"\n"
            "    \"sbbq %0, %0\\n\\t\"\n"
            "    : \"=r\"(mask)\n"
            "    : \"r\"(idx), \"r\"(len)\n"
            "    : \"cc\"\n"
            "  );\n"
            "  return idx & mask;\n"
            "#elif defined(__aarch64__)\n"
            "  uint64_t mask;\n"
            "  __asm__ __volatile__(\n"
            "    \"cmp %1, %2\\n\\t\"\n"
            "    \"csetm %0, lo\\n\\t\"\n"
            "    \"csdb\\n\\t\"\n"
            "    : \"=r\"(mask)\n"
            "    : \"r\"(idx), \"r\"(len)\n"
            "    : \"cc\"\n"
            "  );\n"
            "  return idx & mask;\n"
            "#else\n"
            "  volatile uint64_t v_idx = idx;\n"
            "  volatile uint64_t v_len = len;\n"
            "  uint64_t c_idx = v_idx;\n"
            "  uint64_t c_len = v_len;\n"
            "  uint64_t mask = (uint64_t)0 - (uint64_t)(c_idx < c_len);\n"
            "  volatile uint64_t v_mask = mask;\n"
            "  return c_idx & v_mask;\n"
            "#endif\n"
            "}\n\n");
  }
  if (need_mix) {
    eb_cstr(&b,
            "static inline uint64_t stigi_u64_mix(uint64_t x) {\n"
            "  x += UINT64_C(0x9e3779b97f4a7c15);\n"
            "  x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);\n"
            "  x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);\n"
            "  return x ^ (x >> 31);\n"
            "}\n\n");
  }
  if (need_rotr) {
    eb_cstr(&b,
            "static inline uint32_t stigi_u32_rotr(uint32_t x, uint32_t n) {\n"
            "  uint32_t s = n & 31U;\n"
            "  return s == 0U ? x : ((x >> s) | (x << (32U - s)));\n"
            "}\n\n");
  }
  if (need_alloc) {
    /* memory.md 8 + S47 Fil-C test builds: per-object allocation, 64-byte cache-line aligned >= 64B */
    eb_cstr(&b,
            "static inline void *stigi_rt_try_alloc(uint64_t count, size_t elem_size) {\n"
            "  if (elem_size == 0U) return NULL;\n"
            "  if (count == 0U) return calloc(1U, 1U);\n"
            "  if (count > ((uint64_t)SIZE_MAX) / elem_size) return NULL;\n"
            "  size_t total = (size_t)count * elem_size;\n"
            "  if (total >= 64U) {\n"
            "    void *ptr = NULL;\n"
            "    if (posix_memalign(&ptr, 64U, total) != 0 || ptr == NULL) return NULL;\n"
            "    memset(ptr, 0, total);\n"
            "    return ptr;\n"
            "  }\n"
            "  return calloc((size_t)count, elem_size);\n"
            "}\n\n");
  }

  /* S118 startup order with sysconf(_SC_PAGESIZE) + real guard-paged stack execution */
  eb_cstr(&b,
          "static void stigi_rt_shutdown(StigiRtState *st) {\n"
          "  if (st->pool_raw != NULL) {\n"
          "    (void)munmap(st->pool_raw, st->pool_total);\n"
          "    st->pool_raw = NULL;\n"
          "    st->pool = NULL;\n"
          "  }\n"
          "  if (st->stack_raw != NULL) {\n"
          "    (void)munmap(st->stack_raw, st->stack_total);\n"
          "    st->stack_raw = NULL;\n"
          "  }\n"
          "}\n\n"
          "static StigiRtStatus stigi_rt_init(StigiRtState *st) {\n"
          "  st->stack_raw = NULL;\n"
          "  st->stack_total = 0U;\n"
          "  st->pool_raw = NULL;\n"
          "  st->pool_total = 0U;\n"
          "  st->pool = NULL;\n"
          "  st->status = STIGI_RT_OK;\n"
          "  long ps = sysconf(_SC_PAGESIZE);\n"
          "  size_t page = ps > 0 ? (size_t)ps : 4096U;\n"
          "  /* S118 (1): Map main stack (1 MiB) + 1 guard page below it */\n"
          "  size_t stack_sz = 1048576U;\n"
          "  size_t stack_tot = page + stack_sz;\n"
          "  void *sraw = mmap(NULL, stack_tot, PROT_READ | PROT_WRITE,\n"
          "                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);\n"
          "  if (sraw == MAP_FAILED) {\n"
          "    st->status = STIGI_RT_ERR_STACK;\n"
          "    return st->status;\n"
          "  }\n"
          "  if (mprotect(sraw, page, PROT_NONE) != 0) {\n"
          "    (void)munmap(sraw, stack_tot);\n"
          "    st->status = STIGI_RT_ERR_STACK;\n"
          "    return st->status;\n"
          "  }\n"
          "  st->stack_raw = sraw;\n"
          "  st->stack_total = stack_tot;\n"
          "  /* S118 (2): Pre-reserve guard-paged pool + MADV_POPULATE_WRITE or mlockall */\n"
          "  size_t pool_sz = 65536U;\n"
          "  size_t pool_tot = page + pool_sz;\n"
          "  void *praw = mmap(NULL, pool_tot, PROT_READ | PROT_WRITE,\n"
          "                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);\n"
          "  if (praw == MAP_FAILED) {\n"
          "    stigi_rt_shutdown(st);\n"
          "    st->status = STIGI_RT_ERR_MEM_POPULATE;\n"
          "    return st->status;\n"
          "  }\n"
          "  if (mprotect(praw, page, PROT_NONE) != 0) {\n"
          "    (void)munmap(praw, pool_tot);\n"
          "    stigi_rt_shutdown(st);\n"
          "    st->status = STIGI_RT_ERR_MEM_POPULATE;\n"
          "    return st->status;\n"
          "  }\n"
          "  st->pool_raw = praw;\n"
          "  st->pool_total = pool_tot;\n"
          "  st->pool = (char *)praw + page;\n"
          "#ifdef STIGI_PROFILE_RT_LOCKED\n"
          "  if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {\n"
          "    stigi_rt_shutdown(st);\n"
          "    st->status = STIGI_RT_ERR_MEM_LOCK;\n"
          "    return st->status;\n"
          "  }\n"
          "#elif defined(MADV_POPULATE_WRITE)\n"
          "  if (madvise((char *)sraw + page, stack_sz, MADV_POPULATE_WRITE) != 0 ||\n"
          "      madvise(st->pool, pool_sz, MADV_POPULATE_WRITE) != 0) {\n"
          "    stigi_rt_shutdown(st);\n"
          "    st->status = STIGI_RT_ERR_MEM_POPULATE;\n"
          "    return st->status;\n"
          "  }\n"
          "#else\n"
          "  volatile unsigned char *vs = (volatile unsigned char *)((char *)sraw + page);\n"
          "  for (size_t off = 0U; off < stack_sz; off += page) {\n"
          "    vs[off] = 0U;\n"
          "  }\n"
          "  volatile unsigned char *vp = (volatile unsigned char *)st->pool;\n"
          "  for (size_t off = 0U; off < pool_sz; off += page) {\n"
          "    vp[off] = 0U;\n"
          "  }\n"
          "#endif\n"
          "  /* S118 (3): Set SIGPIPE disposition to SIG_IGN */\n"
          "  struct sigaction sa;\n"
          "  sa.sa_handler = SIG_IGN;\n"
          "  sa.sa_flags = 0;\n"
          "  sigemptyset(&sa.sa_mask);\n"
          "  if (sigaction(SIGPIPE, &sa, NULL) != 0) {\n"
          "    stigi_rt_shutdown(st);\n"
          "    st->status = STIGI_RT_ERR_SIGPIPE;\n"
          "    return st->status;\n"
          "  }\n"
          "  return STIGI_RT_OK;\n"
          "}\n\n");

  /* 1. Emit Enums in topological order + _Static_assert on sizeOf / alignOf / offsetof */
  uint32_t *topo_buf = scnt > 0 ? malloc((size_t)scnt * sizeof *topo_buf) : NULL;
  uint32_t topo_cnt = 0;
  if (topo_buf != NULL) {
    topo_order_kind(img, sorted, scnt, STIGI_DECL_ENUM, topo_buf, &topo_cnt);
    for (uint32_t i = 0; i < topo_cnt; i++) {
      const StigiImageUnit *u = &img->units[topo_buf[i]];
      stats->enums_emitted++;
      stats->units_emitted++;
      emit_enum_unit(&b, img, u, u->ast->decl, enum_needs_eq != NULL && enum_needs_eq[u->idx]);
    }
  }
  free(enum_needs_eq);

  /* 1b. Emit body-less extern C declarations (skip standard POSIX symbols already declared by headers) */
  for (uint32_t i = 0; i < scnt; i++) {
    const StigiImageUnit *u = sorted[i].u;
    const StigiDecl *d = u->ast->decl;
    if (d->kind != STIGI_DECL_EXTERN) continue;
    StigiStr csym = extern_c_sym(d->name);
    if (d->u.extern_decl.is_val) {
      eb_line_dir(&b, d->span.line, u->name, u->name_len);
      eb_cstr(&b, "extern ");
      emit_c_type(&b, img, u->idx, d->u.extern_decl.val_type, 0);
      eb_cstr(&b, " ");
      eb_str(&b, csym);
      eb_cstr(&b, ";\n");
    } else if (d->u.extern_decl.body == NULL) {
      if (str_eq_c(csym, "openat") || str_eq_c(csym, "read") || str_eq_c(csym, "write") ||
          str_eq_c(csym, "close") || str_eq_c(csym, "lseek") || str_eq_c(csym, "socket") ||
          str_eq_c(csym, "bind") || str_eq_c(csym, "listen") || str_eq_c(csym, "accept") ||
          str_eq_c(csym, "unlink") || str_eq_c(csym, "mmap") || str_eq_c(csym, "munmap") ||
          str_eq_c(csym, "mprotect") || str_eq_c(csym, "madvise") || str_eq_c(csym, "mlockall") ||
          str_eq_c(csym, "sigaction") || str_eq_c(csym, "getrusage") ||
          str_eq_c(csym, "clock_gettime")) {
        continue;
      }
      const StigiDefHead *eh = &d->u.extern_decl.head;
      eb_line_dir(&b, d->span.line, u->name, u->name_len);
      eb_cstr(&b, "extern ");
      emit_c_type(&b, img, u->idx, eh->ret_type, 0);
      eb_cstr(&b, " ");
      eb_str(&b, csym);
      eb_cstr(&b, "(");
      if (eh->param_count == 0) {
        eb_cstr(&b, "void");
      } else {
        for (uint32_t p = 0; p < eh->param_count; p++) {
          if (p > 0) eb_cstr(&b, ", ");
          emit_c_type(&b, img, u->idx, eh->params[p].type, 0);
          eb_cstr(&b, " ");
          eb_c_ident(&b, eh->params[p].name);
        }
      }
      eb_cstr(&b, ");\n");
    }
  }

  /* 1b. Emit Handler State & Capability Protocol Handle typedefs before forward decls */
  for (uint32_t i = 0; i < scnt; i++) {
    const StigiImageUnit *u = sorted[i].u;
    const StigiDecl *d = u->ast->decl;
    if (d->kind == STIGI_DECL_HANDLER) {
      emit_handler_state_typedef(&b, img, u, d);
    }
  }
  for (uint32_t i = 0; i < scnt; i++) {
    const StigiImageUnit *u = sorted[i].u;
    const StigiDecl *d = u->ast->decl;
    if (d->kind == STIGI_DECL_CAPABILITY) {
      emit_cap_handle_typedef(&b, img, u);
    }
  }

  /* 2. Forward-declare all reached defs, body-ful C-DSL externs, and Tests */
  for (uint32_t i = 0; i < scnt; i++) {
    const StigiImageUnit *u = sorted[i].u;
    const StigiDecl *d = u->ast->decl;
    if (d->kind == STIGI_DECL_LEMMA_DEF) {
      stats->lemmas_erased++;
      continue;
    }
    if (d->kind == STIGI_DECL_DEF) {
      if (d->u.def_decl.head.type_param_count > 0) continue;
      emit_def_sig_head(&b, img, u->idx, &d->u.def_decl.head, u->name, u->name_len,
                        strcmp(u->name, "main") != 0, false, NULL);
      eb_cstr(&b, ";\n");
    } else if (d->kind == STIGI_DECL_EXTERN && !d->u.extern_decl.is_val &&
               d->u.extern_decl.body != NULL) {
      if (d->u.extern_decl.head.type_param_count > 0) continue;
      emit_def_sig_head(&b, img, u->idx, &d->u.extern_decl.head, u->name, u->name_len,
                        true, true, NULL);
      eb_cstr(&b, ";\n");
    } else if (d->kind == STIGI_DECL_TEST) {
      eb_cstr(&b, "static int32_t ");
      eb_mangle_name(&b, u->name, u->name_len);
      eb_cstr(&b, "(void);\n");
    } else if (d->kind == STIGI_DECL_GIVEN && d->u.given_decl.type_param_count == 0) {
      for (uint32_t m = 0; m < d->u.given_decl.def_count; m++) {
        const StigiDecl *md = d->u.given_decl.defs[m];
        if (md->kind != STIGI_DECL_DEF) continue;
        const StigiDefHead *mh = &md->u.def_decl.head;
        eb_cstr(&b, "static inline ");
        emit_c_type(&b, img, u->idx, mh->ret_type, 0);
        eb_cstr(&b, " ");
        eb_mangle_name(&b, u->name, u->name_len);
        eb_cstr(&b, "_");
        eb_str(&b, md->name);
        eb_cstr(&b, "(");
        if (mh->param_count == 0) {
          eb_cstr(&b, "void");
        } else {
          for (uint32_t p = 0; p < mh->param_count; p++) {
            if (p > 0) eb_cstr(&b, ", ");
            emit_c_type(&b, img, u->idx, mh->params[p].type, 0);
            eb_cstr(&b, " ");
            eb_c_ident(&b, mh->params[p].name);
          }
        }
        eb_cstr(&b, ");\n");
      }
    }
  }
  eb_cstr(&b, "\n");

  /* 3. Emit top-level vals in topological order (comptime-folded into C11 constant initializers!) */
  if (topo_buf != NULL) {
    topo_order_kind(img, sorted, scnt, STIGI_DECL_VAL, topo_buf, &topo_cnt);
    Arena ev_arena;
    arena_init(&ev_arena);
    for (uint32_t i = 0; i < topo_cnt; i++) {
      const StigiImageUnit *u = &img->units[topo_buf[i]];
      const StigiDecl *d = u->ast->decl;
      if (ast_type_is_void(img, u->idx, d->u.val_decl.type)) continue;
      if (!val_used_at_runtime(img, u->idx)) continue;
      stats->units_emitted++;
      eb_line_dir(&b, d->span.line, u->name, u->name_len);
      eb_cstr(&b, "static ");
      emit_c_type(&b, img, u->idx, d->u.val_decl.type, 0);
      /* the binding is const, the pointed bytes are not: a text val reaches defs as `uint8_t *`
       * (the write-to-static rule keeps it read-only), so `const uint8_t *` discarded the qualifier */
      eb_cstr(&b, " const ");
      eb_mangle_name(&b, u->name, u->name_len);
      eb_cstr(&b, " = ");
      EmitScalarTy vhint = ast_type_to_scalar(img, u->idx, d->u.val_decl.type, 0);
      StigiEvalResult er = stigi_eval_unit(img, u->idx, STIGI_DEFAULT_FUEL, &ev_arena);
      if (er.status == STIGI_OK && er.value.kind == STIGI_VAL_INT) {
        if (er.value.is_unsigned) {
          emit_typed_int_lit(&b, er.value.u.u, false, vhint);
        } else if (er.value.u.i < 0) {
          uint64_t mag = (er.value.u.i == INT64_MIN) ? (9223372036854775808ULL)
                                                     : (uint64_t)(-er.value.u.i);
          emit_typed_int_lit(&b, mag, true, vhint);
        } else {
          emit_typed_int_lit(&b, (uint64_t)er.value.u.i, false, vhint);
        }
      } else if (er.status == STIGI_OK && er.value.kind == STIGI_VAL_BOOL) {
        eb_cstr(&b, er.value.u.b ? "true" : "false");
      } else {
        EmitCtx ctx;
        memset(&ctx, 0, sizeof ctx);
        ctx.img = img;
        ctx.uidx = u->idx;
        ctx.b = &b;
        ctx.stats = stats;
        ctx.mask_info = mask_infos != NULL ? &mask_infos[u->idx] : NULL;
        emit_expr_typed(&ctx, stigi_val_body(d), vhint);
        emit_ctx_free(&ctx);
      }
      eb_cstr(&b, ";\n\n");
    }
    arena_free(&ev_arena);
  }
  free(topo_buf);

  /* 3b. Emit Handlers (init + ops) */
  for (uint32_t i = 0; i < scnt; i++) {
    const StigiImageUnit *u = sorted[i].u;
    const StigiDecl *d = u->ast->decl;
    if (d->kind != STIGI_DECL_HANDLER) continue;
    stats->units_emitted++;
    emit_handler_unit(&b, img, u, d, stats, mask_infos);
  }

  /* 3c. Emit Capability Protocol Dispatchers (only when referenced by a protocol-param caller) */
  for (uint32_t i = 0; i < scnt; i++) {
    const StigiImageUnit *u = sorted[i].u;
    const StigiDecl *d = u->ast->decl;
    if (d->kind != STIGI_DECL_CAPABILITY) continue;
    if (!cap_has_proto_caller(img, u->idx)) continue;
    emit_cap_proto_ops(&b, img, u);
  }

  /* 3d. Emit Concrete Given Methods (zero dictionary, direct static C11 functions, S125) */
  for (uint32_t i = 0; i < scnt; i++) {
    const StigiImageUnit *u = sorted[i].u;
    const StigiDecl *d = u->ast->decl;
    if (d->kind != STIGI_DECL_GIVEN || d->u.given_decl.type_param_count > 0) continue;
    stats->units_emitted++;
    for (uint32_t m = 0; m < d->u.given_decl.def_count; m++) {
      const StigiDecl *md = d->u.given_decl.defs[m];
      if (md->kind != STIGI_DECL_DEF) continue;
      const StigiDefHead *mh = &md->u.def_decl.head;
      const StigiExpr *mbody = stigi_def_body(md);
      EmitCtx ctx;
      memset(&ctx, 0, sizeof ctx);
      ctx.img = img;
      ctx.uidx = u->idx;
      ctx.cur_def = md;
      ctx.cur_head = mh;
      ctx.b = &b;
      ctx.stats = stats;
      ctx.mask_info = mask_infos != NULL ? &mask_infos[u->idx] : NULL;
      ctx.ret_scalar = ast_type_to_scalar(img, u->idx, mh->ret_type, 0);
      ctx.ret_ast_type = mh->ret_type;
      ctx.ret_type_uidx = u->idx;
      for (uint32_t p = 0; p < mh->param_count; p++) {
        (void)emit_register_var(&ctx, mh->params[p].name, mh->params[p].type, u->idx, true);
      }
      eb_line_dir(&b, md->span.line, u->name, u->name_len);
      eb_cstr(&b, "static inline ");
      emit_c_type(&b, img, u->idx, mh->ret_type, 0);
      eb_cstr(&b, " ");
      eb_mangle_name(&b, u->name, u->name_len);
      eb_cstr(&b, "_");
      eb_str(&b, md->name);
      eb_cstr(&b, "(");
      if (mh->param_count == 0) {
        eb_cstr(&b, "void");
      } else {
        for (uint32_t p = 0; p < mh->param_count; p++) {
          if (p > 0) eb_cstr(&b, ", ");
          emit_c_type(&b, img, u->idx, mh->params[p].type, 0);
          eb_cstr(&b, " ");
          eb_c_ident(&b, mh->params[p].name);
        }
      }
      eb_cstr(&b, ") {\n");
      for (uint32_t p = 0; p < mh->param_count; p++) {
        eb_cstr(&b, "  (void)");
        eb_c_ident(&b, mh->params[p].name);
        eb_cstr(&b, ";\n");
      }
      bool is_void = ast_type_is_void(img, u->idx, mh->ret_type);
      emit_stmt_or_ret(&ctx, mbody, true, is_void);
      eb_cstr(&b, "}\n\n");
      emit_ctx_free(&ctx);
    }
  }

  /* 4. Emit body-ful C-DSL externs, defs, Tests, and monomorphized generic instances */
  bool has_main = false;
  bool main_is_void = false;
  bool any_fail_closed = false;
  char fail_msg[256] = {0};
  EmitBuf bodies_b = {NULL, 0, 0, false};
  MonoList mono_list;
  memset(&mono_list, 0, sizeof mono_list);
  Arena mono_arena;
  arena_init(&mono_arena);

  for (uint32_t i = 0; i < scnt; i++) {
    const StigiImageUnit *u = sorted[i].u;
    const StigiDecl *d = u->ast->decl;
    if (d->kind == STIGI_DECL_DEF ||
        (d->kind == STIGI_DECL_EXTERN && !d->u.extern_decl.is_val &&
         d->u.extern_decl.body != NULL)) {
      bool is_extern_dsl = (d->kind == STIGI_DECL_EXTERN);
      const StigiDefHead *h = is_extern_dsl ? &d->u.extern_decl.head : &d->u.def_decl.head;
      if (h->type_param_count > 0) continue;
      const StigiExpr *body = is_extern_dsl ? d->u.extern_decl.body : stigi_def_body(d);
      if (!is_extern_dsl && strcmp(u->name, "main") == 0) {
        has_main = true;
        main_is_void = ast_type_is_void(img, u->idx, h->ret_type);
      }
      stats->defs_emitted++;
      stats->units_emitted++;

      EmitCtx ctx;
      memset(&ctx, 0, sizeof ctx);
      ctx.img = img;
      ctx.uidx = u->idx;
      ctx.cur_def = is_extern_dsl ? NULL : d;
      ctx.cur_head = h;
      ctx.b = &bodies_b;
      ctx.stats = stats;
      ctx.mask_info = mask_infos != NULL ? &mask_infos[u->idx] : NULL;
      ctx.is_tail_loop = !is_extern_dsl && has_self_tail_call(&ctx, body);
      ctx.is_never_def = (ast_type_to_scalar(img, u->idx, h->ret_type, 0) == ETY_NEVER);
      ctx.in_extern_dsl = is_extern_dsl;
      ctx.ret_scalar = ast_type_to_scalar(img, u->idx, h->ret_type, 0);
      ctx.ret_ast_type = h->ret_type;
      ctx.ret_type_uidx = u->idx;
      ctx.mono_list = &mono_list;
      ctx.mono_arena = &mono_arena;
      if (ctx.is_tail_loop) stats->tail_loops++;

      uint32_t cap_param_seq = 0;
      for (uint32_t g = 0; g < h->pre_group_count; g++) {
        if (h->pre_groups[g].is_fact) continue;
        for (uint32_t c = 0; c < h->pre_groups[g].cap_count; c++) {
          const StigiType *ct = h->pre_groups[g].caps[c].cap_type;
          if (ct == NULL) continue;
          int32_t cidx = stigi_image_resolve_name(img, u->idx, ct->name.ptr, ct->name.len);
          if (cidx >= 0 && cap_has_runtime_handle(img, (uint32_t)cidx)) {
            char cpname[32];
            (void)snprintf(cpname, sizeof cpname, "_cap%u", cap_param_seq++);
            EmitVar *cv = emit_push_var_raw(&ctx, h->pre_groups[g].caps[c].name,
                                            h->pre_groups[g].caps[c].name.len > 0);
            if (cv != NULL) {
              cv->cap_uidx = cidx;
              size_t cl = strlen(cpname);
              cv->cap_handle_var = malloc(cl + 1U);
              if (cv->cap_handle_var != NULL) memcpy(cv->cap_handle_var, cpname, cl + 1U);
            }
          }
        }
      }

      if (h->param_count > 0) {
        ctx.param_cnames = calloc(h->param_count, sizeof *ctx.param_cnames);
        ctx.param_cname_count = h->param_count;
      }
      for (uint32_t p = 0; p < h->param_count; p++) {
        EmitVar *pv = emit_register_var(&ctx, h->params[p].name, h->params[p].type, u->idx, true);
        if (pv != NULL && pv->c_name != NULL && ctx.param_cnames != NULL) {
          size_t cl = strlen(pv->c_name);
          ctx.param_cnames[p] = malloc(cl + 1U);
          if (ctx.param_cnames[p] != NULL) memcpy(ctx.param_cnames[p], pv->c_name, cl + 1U);
        }
      }

      eb_line_dir(&bodies_b, d->span.line, u->name, u->name_len);
      emit_def_sig_head(&bodies_b, img, u->idx, h, u->name, u->name_len,
                        is_extern_dsl || strcmp(u->name, "main") != 0, is_extern_dsl, NULL);
      eb_cstr(&bodies_b, " {\n");
      for (uint32_t cp = 0; cp < cap_param_seq; cp++) {
        eb_fmt(&bodies_b, "  (void)_cap%u;\n", cp);
      }
      for (uint32_t p = 0; p < h->param_count; p++) {
        eb_cstr(&bodies_b, "  (void)");
        eb_c_ident(&bodies_b, h->params[p].name);
        eb_cstr(&bodies_b, ";\n");
      }
      bool is_void = ast_type_is_void(img, u->idx, h->ret_type);
      if (ctx.is_tail_loop) {
        eb_cstr(&bodies_b, "  while (1) {\n");
        emit_stmt_or_ret(&ctx, body, true, is_void);
        eb_cstr(&bodies_b, "  }\n");
      } else {
        emit_stmt_or_ret(&ctx, body, true, is_void);
      }
      eb_cstr(&bodies_b, "}\n\n");
      if (ctx.fail_closed && !any_fail_closed) {
        any_fail_closed = true;
        (void)snprintf(fail_msg, sizeof fail_msg, "%s", ctx.fail_msg);
      }
      emit_ctx_free(&ctx);
    } else if (d->kind == STIGI_DECL_TEST) {
      stats->units_emitted++;
      EmitCtx ctx;
      memset(&ctx, 0, sizeof ctx);
      ctx.img = img;
      ctx.uidx = u->idx;
      ctx.b = &bodies_b;
      ctx.stats = stats;
      ctx.mask_info = mask_infos != NULL ? &mask_infos[u->idx] : NULL;
      ctx.ret_scalar = ETY_I32;
      ctx.mono_list = &mono_list;
      ctx.mono_arena = &mono_arena;
      eb_line_dir(&bodies_b, d->span.line, u->name, u->name_len);
      eb_cstr(&bodies_b, "static int32_t ");
      eb_mangle_name(&bodies_b, u->name, u->name_len);
      eb_cstr(&bodies_b, "(void) {\n");
      emit_stmt_or_ret(&ctx, stigi_val_body(d), true, false);
      eb_cstr(&bodies_b, "}\n\n");
      if (ctx.fail_closed && !any_fail_closed) {
        any_fail_closed = true;
        (void)snprintf(fail_msg, sizeof fail_msg, "%s", ctx.fail_msg);
      }
      emit_ctx_free(&ctx);
    }
  }

  /* 4b. Emit monomorphized generic def and C-DSL specializations (fixed-point worklist, S125; emitter.md 3) */
  for (uint32_t mi = 0; mi < mono_list.count; mi++) {
    uint32_t m_uidx = mono_list.items[mi].uidx;
    const StigiImageUnit *u = &img->units[m_uidx];
    const StigiDecl *d = u->ast->decl;
    bool is_extern_dsl = d->kind == STIGI_DECL_EXTERN;
    const StigiDefHead *h = is_extern_dsl ? &d->u.extern_decl.head : &d->u.def_decl.head;
    const StigiExpr *body = is_extern_dsl ? d->u.extern_decl.body : stigi_def_body(d);
    const StigiStr *tp_names = mono_list.items[mi].tp_names;
    const StigiType *const *tp_vals = mono_list.items[mi].tp_vals;
    uint32_t tp_count = mono_list.items[mi].tp_count;
    const char *mkey = mono_list.items[mi].key;

    /* Forward declaration in `b` before `bodies_b` */
    emit_def_sig_head_mono(&b, img, u->idx, h, u->name, u->name_len,
                           true, is_extern_dsl, NULL, mkey, tp_names, tp_vals, tp_count, &mono_arena);
    eb_cstr(&b, ";\n");

    const StigiType *rty = mono_subst_type(&mono_arena, tp_names, tp_vals, tp_count, h->ret_type);
    EmitCtx ctx;
    memset(&ctx, 0, sizeof ctx);
    ctx.img = img;
    ctx.uidx = u->idx;
    ctx.cur_def = is_extern_dsl ? NULL : d;
    ctx.in_extern_dsl = is_extern_dsl;
    ctx.cur_head = h;
    ctx.b = &bodies_b;
    ctx.stats = stats;
    ctx.mask_info = mask_infos != NULL ? &mask_infos[u->idx] : NULL;
    ctx.is_never_def = (ast_type_to_scalar(img, u->idx, rty, 0) == ETY_NEVER);
    ctx.ret_scalar = ast_type_to_scalar(img, u->idx, rty, 0);
    ctx.ret_ast_type = rty;
    ctx.ret_type_uidx = u->idx;
    ctx.mono_tp_names = tp_names;
    ctx.mono_tp_vals = tp_vals;
    ctx.mono_tp_count = tp_count;
    ctx.mono_list = &mono_list;
    ctx.mono_arena = &mono_arena;

    uint32_t cap_param_seq = 0;
    for (uint32_t g = 0; g < h->pre_group_count; g++) {
      if (h->pre_groups[g].is_fact) continue;
      for (uint32_t c = 0; c < h->pre_groups[g].cap_count; c++) {
        const StigiType *ct = h->pre_groups[g].caps[c].cap_type;
        if (ct == NULL) continue;
        int32_t cidx = stigi_image_resolve_name(img, u->idx, ct->name.ptr, ct->name.len);
        if (cidx >= 0 && cap_has_runtime_handle(img, (uint32_t)cidx)) {
          char cpname[32];
          (void)snprintf(cpname, sizeof cpname, "_cap%u", cap_param_seq++);
          EmitVar *cv = emit_push_var_raw(&ctx, h->pre_groups[g].caps[c].name,
                                          h->pre_groups[g].caps[c].name.len > 0);
          if (cv != NULL) {
            cv->cap_uidx = cidx;
            size_t cl = strlen(cpname);
            cv->cap_handle_var = malloc(cl + 1U);
            if (cv->cap_handle_var != NULL) memcpy(cv->cap_handle_var, cpname, cl + 1U);
          }
        }
      }
    }
    for (uint32_t p = 0; p < h->param_count; p++) {
      const StigiType *pty = mono_subst_type(&mono_arena, tp_names, tp_vals, tp_count, h->params[p].type);
      (void)emit_register_var(&ctx, h->params[p].name, pty, u->idx, true);
    }

    eb_line_dir(&bodies_b, d->span.line, u->name, u->name_len);
    emit_def_sig_head_mono(&bodies_b, img, u->idx, h, u->name, u->name_len,
                           true, is_extern_dsl, NULL, mkey, tp_names, tp_vals, tp_count, &mono_arena);
    eb_cstr(&bodies_b, " {\n");
    for (uint32_t cp = 0; cp < cap_param_seq; cp++) {
      eb_fmt(&bodies_b, "  (void)_cap%u;\n", cp);
    }
    for (uint32_t p = 0; p < h->param_count; p++) {
      eb_cstr(&bodies_b, "  (void)");
      eb_c_ident(&bodies_b, h->params[p].name);
      eb_cstr(&bodies_b, ";\n");
    }
    bool is_void = ast_type_is_void(img, u->idx, rty);
    emit_stmt_or_ret(&ctx, body, true, is_void);
    eb_cstr(&bodies_b, "}\n\n");
    if (ctx.fail_closed && !any_fail_closed) {
      any_fail_closed = true;
      (void)snprintf(fail_msg, sizeof fail_msg, "%s", ctx.fail_msg);
    }
    emit_ctx_free(&ctx);
  }
  if (mono_list.count > 0) eb_cstr(&b, "\n");
  if (bodies_b.oom) b.oom = true;
  if (bodies_b.buf != NULL && bodies_b.len > 0) eb_mem(&b, bodies_b.buf, bodies_b.len);
  free(bodies_b.buf);
  mono_list_free(&mono_list);
  arena_free(&mono_arena);

  /* 5. C entry point: execute Tests + Stigi_main ON the mapped 1 MiB guard-paged stack (S118) */
  eb_cstr(&b,
          "#line 1 \"<stigi-runtime-main>\"\n"
          "static int stigi_run_on_stack(StigiRtState *rt_state) {\n"
          "  (void)rt_state;\n");
  for (uint32_t i = 0; i < scnt; i++) {
    const StigiImageUnit *u = sorted[i].u;
    if (u->ast->decl->kind == STIGI_DECL_TEST) {
      eb_cstr(&b, "  if (");
      eb_mangle_name(&b, u->name, u->name_len);
      eb_cstr(&b, "() != 0) return 100;\n");
    }
  }
  if (has_main) {
    if (main_is_void) {
      eb_cstr(&b, "  Stigi_main();\n  return 0;\n");
    } else {
      eb_cstr(&b, "  return (int)Stigi_main();\n");
    }
  } else {
    eb_cstr(&b, "  return 0;\n");
  }
  eb_cstr(&b,
          "}\n\n"
          "#if defined(__x86_64__) && !defined(__FILC__)\n"
          "static int stigi_call_on_stack(void *stack_top, int (*fn)(StigiRtState *), StigiRtState *arg) {\n"
          "  uintptr_t top = (((uintptr_t)stack_top) & ~(uintptr_t)15U) - 16U;\n"
          "  int (*f)(StigiRtState *) = fn;\n"
          "  StigiRtState *a = arg;\n"
          "  int rc = 0;\n"
          "  __asm__ __volatile__(\n"
          "    \"movq %%rsp, (%1)\\n\\t\"\n"
          "    \"movq %1, %%rsp\\n\\t\"\n"
          "    \"call *%2\\n\\t\"\n"
          "    \"movq (%%rsp), %%rsp\\n\\t\"\n"
          "    : \"=a\"(rc), \"+r\"(top), \"+S\"(f), \"+D\"(a)\n"
          "    :\n"
          "    : \"rcx\", \"rdx\", \"r8\", \"r9\", \"r10\", \"r11\", \"memory\", \"cc\"\n"
          "  );\n"
          "  return rc;\n"
          "}\n"
          "#elif defined(__aarch64__) && !defined(__FILC__)\n"
          "static int stigi_call_on_stack(void *stack_top, int (*fn)(StigiRtState *), StigiRtState *arg) {\n"
          "  uintptr_t top = (((uintptr_t)stack_top) & ~(uintptr_t)15U) - 16U;\n"
          "  register int rc __asm__(\"w0\");\n"
          "  register StigiRtState *x0 __asm__(\"x0\") = arg;\n"
          "  register int (*x9)(StigiRtState *) __asm__(\"x9\") = fn;\n"
          "  register uintptr_t x10 __asm__(\"x10\") = top;\n"
          "  __asm__ __volatile__(\n"
          "    \"mov x11, sp\\n\\t\"\n"
          "    \"str x11, [x10]\\n\\t\"\n"
          "    \"mov sp, x10\\n\\t\"\n"
          "    \"blr x9\\n\\t\"\n"
          "    \"ldr x11, [sp]\\n\\t\"\n"
          "    \"mov sp, x11\\n\\t\"\n"
          "    : \"=r\"(rc), \"+r\"(x0), \"+r\"(x9), \"+r\"(x10)\n"
          "    :\n"
          "    : \"x1\", \"x2\", \"x3\", \"x4\", \"x5\", \"x6\", \"x7\", \"x8\",\n"
          "      \"x11\", \"x12\", \"x13\", \"x14\", \"x15\", \"x16\", \"x17\", \"x30\", \"memory\", \"cc\"\n"
          "  );\n"
          "  return rc;\n"
          "}\n"
          "#else\n"
          "static int stigi_call_on_stack(void *stack_top, int (*fn)(StigiRtState *), StigiRtState *arg) {\n"
          "  (void)stack_top;\n"
          "  return fn(arg);\n"
          "}\n"
          "#endif\n\n"
          "int main(void) {\n"
          "  StigiRtState rt_state;\n"
          "  StigiRtStatus rt = stigi_rt_init(&rt_state);\n"
          "  if (rt != STIGI_RT_OK) {\n"
          "    stigi_rt_shutdown(&rt_state);\n"
          "    return (int)rt;\n"
          "  }\n"
          "  void *stack_top = (char *)rt_state.stack_raw + rt_state.stack_total;\n"
          "  int rc = stigi_call_on_stack(stack_top, stigi_run_on_stack, &rt_state);\n"
          "  stigi_rt_shutdown(&rt_state);\n"
          "  return rc;\n"
          "}\n");

  free_unit_masks(mask_infos, img->unit_count);
  free(sorted);
  if (any_fail_closed) {
    free(b.buf);
    return diag_set(diag, STIGI_ERR_INVALID, -1, "%s", fail_msg);
  }
  if (b.oom) {
    free(b.buf);
    return diag_set(diag, STIGI_ERR_OOM, -1, "stigi_emit_c: OOM");
  }
  *out_c = b.buf;
  if (out_len != NULL) *out_len = b.len;
  return STIGI_OK;
}

/* Dynamic line reader for .su files (no fixed line truncation; detects embedded NULs) */
static int read_dynamic_line(FILE *f, char **buf, size_t *cap, size_t *out_len) {
  size_t len = 0;
  int ch = 0;
  while ((ch = fgetc(f)) != EOF) {
    if (len + 2U > *cap) {
      size_t ncap = *cap == 0 ? 256U : (*cap) * 2U;
      char *nb = realloc(*buf, ncap);
      if (nb == NULL) return -1;
      *buf = nb;
      *cap = ncap;
    }
    (*buf)[len++] = (char)ch;
    if (ch == '\n') break;
  }
  if (len == 0 && ch == EOF) return 0;
  if (*buf != NULL) (*buf)[len] = '\0';
  if (out_len != NULL) *out_len = len;
  return 1;
}

/* Parse GCC/Clang -fstack-usage .su file: `file:line:col:func\tbytes\tqualifier` */
Status stigi_emit_check_su(const char *su_path, const StigiImage *img, StigiEmitProfile profile,
                           char *err_buf, size_t err_cap) {
  if (su_path == NULL || img == NULL) return STIGI_ERR_INVALID;
  /* count the same unit set the emitter emitted for this profile */
  StigiImageUnit *units = NULL;
  Diag pd;
  diag_clear(&pd);
  Status pst = profile_units(img, profile, &units, &pd);
  if (pst != STIGI_OK) {
    if (err_buf != NULL && err_cap > 0) (void)snprintf(err_buf, err_cap, "%s", pd.msg);
    return pst;
  }
  uint32_t expected_funcs = 0;
  for (uint32_t i = 0; i < img->unit_count; i++) {
    if (!units[i].reachable) continue;
    StigiDeclKind k = units[i].ast->decl->kind;
    if (k == STIGI_DECL_DEF || k == STIGI_DECL_TEST) expected_funcs++;
  }
  free(units);

  FILE *f = fopen(su_path, "r");
  if (f == NULL) {
    if (err_buf != NULL && err_cap > 0) {
      (void)snprintf(err_buf, err_cap, "cannot open .su file '%s'", su_path);
    }
    return STIGI_ERR_IO;
  }

  char *line = NULL;
  size_t line_cap = 0;
  size_t line_len = 0;
  uint32_t matched = 0;
  Status result = STIGI_OK;

  for (;;) {
    int rr = read_dynamic_line(f, &line, &line_cap, &line_len);
    if (rr < 0) {
      if (err_buf != NULL && err_cap > 0) {
        (void)snprintf(err_buf, err_cap, "OOM reading .su file '%s'", su_path);
      }
      result = STIGI_ERR_OOM;
      break;
    }
    if (rr == 0) break;

    if (memchr(line, '\0', line_len) != NULL) {
      if (err_buf != NULL && err_cap > 0) {
        (void)snprintf(err_buf, err_cap, "embedded NUL byte in .su file '%s'", su_path);
      }
      result = STIGI_ERR_INVALID;
      break;
    }

    while (line_len > 0 && (line[line_len - 1] == '\n' || line[line_len - 1] == '\r')) {
      line[--line_len] = '\0';
    }
    if (line_len == 0) continue;

    char *t1 = strchr(line, '\t');
    if (t1 == NULL) {
      if (err_buf != NULL && err_cap > 0) {
        (void)snprintf(err_buf, err_cap, "malformed .su line (missing first tab)");
      }
      result = STIGI_ERR_INVALID;
      break;
    }
    *t1 = '\0';
    char *t2 = strchr(t1 + 1, '\t');
    if (t2 == NULL) {
      if (err_buf != NULL && err_cap > 0) {
        (void)snprintf(err_buf, err_cap, "malformed .su line (missing second tab)");
      }
      result = STIGI_ERR_INVALID;
      break;
    }
    *t2 = '\0';
    const char *bytes_str = t1 + 1;
    const char *qual = t2 + 1;

    if (*bytes_str == '\0' || *bytes_str == '-' || *bytes_str == '+') {
      if (err_buf != NULL && err_cap > 0) {
        (void)snprintf(err_buf, err_cap, "malformed .su byte count '%s'", bytes_str);
      }
      result = STIGI_ERR_INVALID;
      break;
    }
    errno = 0;
    char *endptr = NULL;
    unsigned long long parsed_bytes = strtoull(bytes_str, &endptr, 10);
    if (errno == ERANGE || endptr == bytes_str || *endptr != '\0') {
      if (err_buf != NULL && err_cap > 0) {
        (void)snprintf(err_buf, err_cap, "invalid .su byte count '%s'", bytes_str);
      }
      result = STIGI_ERR_INVALID;
      break;
    }
    uint64_t bytes = (uint64_t)parsed_bytes;

    const char *colon = strrchr(line, ':');
    const char *fn = colon != NULL ? colon + 1 : line;

    if (strcmp(qual, "static") != 0) {
      if (err_buf != NULL && err_cap > 0) {
        (void)snprintf(err_buf, err_cap, "function '%s' has non-static stack usage '%s'", fn, qual);
      }
      result = STIGI_ERR_INVALID;
      break;
    }

    if (strncmp(fn, "Stigi_", 6) == 0) {
      for (uint32_t i = 0; i < img->unit_count; i++) {
        StigiDeclKind k = img->units[i].ast->decl->kind;
        if (k != STIGI_DECL_DEF && k != STIGI_DECL_TEST) continue;
        size_t mlen = 6U + img->units[i].name_len;
        char *mangled = malloc(mlen + 1U);
        if (mangled == NULL) {
          result = STIGI_ERR_OOM;
          break;
        }
        memcpy(mangled, "Stigi_", 6);
        for (size_t j = 0; j < img->units[i].name_len; j++) {
          mangled[6U + j] = img->units[i].name[j] == '.' ? '_' : img->units[i].name[j];
        }
        mangled[mlen] = '\0';
        bool eq = (strcmp(fn, mangled) == 0);
        free(mangled);
        if (eq) {
          matched++;
          uint64_t fb = 0;
          if (stigi_def_frame_bytes(img, i, &fb) != STIGI_OK) {
            if (err_buf != NULL && err_cap > 0) {
              (void)snprintf(err_buf, err_cap, "cannot compute frameBytes for '%s'", fn);
            }
            result = STIGI_ERR_INVALID;
          } else if (bytes > fb) {
            if (err_buf != NULL && err_cap > 0) {
              (void)snprintf(err_buf, err_cap,
                             "function '%s' stack usage %llu exceeds frameBytes %llu",
                             fn, (unsigned long long)bytes, (unsigned long long)fb);
            }
            result = STIGI_ERR_INVALID;
          }
          break;
        }
      }
      if (result != STIGI_OK) break;
    }
  }

  free(line);
  (void)fclose(f);

  if (result == STIGI_OK && expected_funcs > 0 && matched == 0) {
    if (err_buf != NULL && err_cap > 0) {
      (void)snprintf(err_buf, err_cap,
                     "0 emitted Stigi_* functions matched in '%s' (expected >= 1 of %u)",
                     su_path, expected_funcs);
    }
    return STIGI_ERR_INVALID;
  }

  return result;
}
