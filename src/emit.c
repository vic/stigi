#include "emit.h"
#include "datalog.h"
#include "extract.h"
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
  if (add > SIZE_MAX - b->len - 1U) { b->oom = true; return; }
  if (b->len + add + 1U <= b->cap) return;
  size_t ncap = b->cap == 0 ? 4096U : b->cap;
  while (ncap < b->len + add + 1U) ncap *= 2U;
  char *nb = realloc(b->buf, ncap);
  if (nb == NULL) { b->oom = true; return; }
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

static void eb_fmt(EmitBuf *b, const char *fmt, ...) {
  char tmp[512];
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(tmp, sizeof tmp, fmt, ap);
  va_end(ap);
  if (n > 0) {
    size_t len = (size_t)n < sizeof(tmp) ? (size_t)n : sizeof(tmp) - 1U;
    eb_mem(b, tmp, len);
  }
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

static bool str_eq_c(StigiStr s, const char *lit) {
  size_t n = strlen(lit);
  return s.len == n && memcmp(s.ptr, lit, n) == 0;
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

static bool ast_type_is_never(const StigiImage *img, uint32_t from_idx, const StigiType *ty) {
  return ast_type_to_scalar(img, from_idx, ty, 0) == ETY_NEVER;
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
      StigiTy ety;
      memset(&ety, 0, sizeof ety);
      ety.kind = STIGI_TY_ENUM;
      ety.unit_idx = (uint32_t)tgt;
      return stigi_ty_layout(img, &ety);
    }
  }
  return (StigiLayout){8, 8};
}

static void emit_c_type(EmitBuf *b, const StigiImage *img, uint32_t from_idx,
                        const StigiType *ty, int depth) {
  if (ty == NULL || depth > 16) { eb_cstr(b, "uint64_t"); return; }
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
  }
  return "+";
}

/* ---- Datalog maskIndex decision set (Defect 7) ---- */

typedef struct {
  bool *access_masked;
  uint32_t access_count;
  uint32_t access_cap;
} UnitMaskInfo;

static int cmp_cstr_ptr(const void *pa, const void *pb) {
  const char *a = *(const char *const *)pa;
  const char *b = *(const char *const *)pb;
  return strcmp(a, b);
}

static bool sorted_cstr_contains(const char *const *arr, uint32_t n, const char *key) {
  uint32_t lo = 0, hi = n;
  while (lo < hi) {
    uint32_t mid = lo + (hi - lo) / 2U;
    int c = strcmp(arr[mid], key);
    if (c == 0) return true;
    if (c < 0) lo = mid + 1U;
    else hi = mid;
  }
  return false;
}

static bool is_taint_cap_name(StigiStr cn) {
  return str_eq_c(cn, "Stigi.File") || str_eq_c(cn, "Stigi.Socket") || str_eq_c(cn, "Stigi.Clock");
}

static void free_unit_masks(UnitMaskInfo *info, uint32_t count) {
  if (info == NULL) return;
  for (uint32_t i = 0; i < count; i++) {
    free(info[i].access_masked);
  }
  free(info);
}

static Status compute_datalog_masks(const StigiDoor *door, UnitMaskInfo **out_info,
                                    uint32_t *out_total_masks, Diag *diag) {
  const StigiImage *img = &door->image;
  *out_info = NULL;
  *out_total_masks = 0;
  if (img->unit_count == 0) return STIGI_OK;

  UnitMaskInfo *info = calloc(img->unit_count, sizeof *info);
  if (info == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "compute_datalog_masks: OOM");

  if (door->rules_dl == NULL || door->rules_dl_len == 0) {
    *out_info = info;
    return STIGI_OK;
  }

  StigiFactSet fs;
  stigi_facts_init(&fs);
  Status st = stigi_extract_image(img, &fs, diag);
  if (st != STIGI_OK) {
    stigi_facts_free(&fs);
    free_unit_masks(info, img->unit_count);
    return st;
  }

  StigiDl dl;
  memset(&dl, 0, sizeof dl);
  st = stigi_dl_open(&dl, door->rules_dl, door->rules_dl_len, "emit_mask.dl",
                     door->row_budget, 1, diag);
  if (st != STIGI_OK) {
    stigi_facts_free(&fs);
    free_unit_masks(info, img->unit_count);
    return st;
  }

  st = stigi_facts_load_into_dl(&fs, &dl, diag);
  if (st != STIGI_OK) {
    stigi_dl_close(&dl);
    stigi_facts_free(&fs);
    free_unit_masks(info, img->unit_count);
    return st;
  }

  /* Seed taint source operationCall + result for defs holding Stigi.File/Socket/Clock capabilities */
  int rel_opcall = stigi_dl_rel_id(&dl, "operationCall");
  int rel_result = stigi_dl_rel_id(&dl, "result");
  if (rel_opcall >= 0 && rel_result >= 0) {
    for (uint32_t i = 0; i < img->unit_count; i++) {
      const StigiImageUnit *u = &img->units[i];
      const StigiDecl *d = u->ast->decl;
      if (d->kind != STIGI_DECL_DEF) continue;
      const StigiDefHead *h = &d->u.def_decl.head;
      int32_t taint_cap_idx = -1;
      uint32_t cap_ord = 0;
      for (uint32_t g = 0; g < h->pre_group_count; g++) {
        if (h->pre_groups[g].is_fact) continue;
        for (uint32_t c = 0; c < h->pre_groups[g].cap_count; c++) {
          if (is_taint_cap_name(h->pre_groups[g].caps[c].cap_type->name) && taint_cap_idx < 0) {
            taint_cap_idx = (int32_t)cap_ord;
          }
          cap_ord++;
        }
      }
      if (taint_cap_idx < 0) continue;

      char cap_sym[256], pt_sym[256];
      (void)snprintf(cap_sym, sizeof cap_sym, "%s#0:cap%d", u->name, taint_cap_idx);
      (void)snprintf(pt_sym, sizeof pt_sym, "%s#0:p0", u->name);
      uint32_t cap_id = 0, opname_id = 0, pt_id = 0;
      if (stigi_dl_intern(&dl, cap_sym, strlen(cap_sym), &cap_id, diag) != STIGI_OK ||
          stigi_dl_intern(&dl, "read", 4, &opname_id, diag) != STIGI_OK ||
          stigi_dl_intern(&dl, pt_sym, strlen(pt_sym), &pt_id, diag) != STIGI_OK) {
        stigi_dl_close(&dl);
        stigi_facts_free(&fs);
        free_unit_masks(info, img->unit_count);
        return STIGI_ERR_OOM;
      }

      for (uint32_t p = 0; p < h->param_count; p++) {
        uint32_t from_tmp = i;
        const StigiType *pty = resolve_typedef_type(img, &from_tmp, h->params[p].type, 0);
        if (pty->kind == STIGI_TYPE_ARRAY) continue;
        char op_sym[256], val_sym[256];
        (void)snprintf(op_sym, sizeof op_sym, "%s#0:taint_seed_%u", u->name, p);
        (void)snprintf(val_sym, sizeof val_sym, "%s#0:p:%.*s", u->name,
                       (int)h->params[p].name.len, h->params[p].name.ptr);
        uint32_t op_id = 0, val_id = 0;
        if (stigi_dl_intern(&dl, op_sym, strlen(op_sym), &op_id, diag) != STIGI_OK ||
            stigi_dl_intern(&dl, val_sym, strlen(val_sym), &val_id, diag) != STIGI_OK) {
          stigi_dl_close(&dl);
          stigi_facts_free(&fs);
          free_unit_masks(info, img->unit_count);
          return STIGI_ERR_OOM;
        }
        int64_t oc_row[4] = {(int64_t)op_id, (int64_t)cap_id, (int64_t)opname_id, (int64_t)pt_id};
        int64_t res_row[2] = {(int64_t)op_id, (int64_t)val_id};
        (void)stigi_dl_add(&dl, rel_opcall, oc_row, 4, diag);
        (void)stigi_dl_add(&dl, rel_result, res_row, 2, diag);
      }
    }
  }

  st = stigi_dl_eval(&dl, diag);
  if (st != STIGI_OK) {
    stigi_dl_close(&dl);
    stigi_facts_free(&fs);
    free_unit_masks(info, img->unit_count);
    return st;
  }

  char **mask_ops = NULL;
  uint32_t mask_op_count = 0;
  int rel_mask = stigi_dl_rel_id(&dl, "maskIndex");
  if (rel_mask >= 0) {
    uint32_t cnt = stigi_dl_count(&dl, rel_mask);
    if (cnt > 0) {
      mask_ops = calloc(cnt, sizeof *mask_ops);
      if (mask_ops == NULL) {
        stigi_dl_close(&dl);
        stigi_facts_free(&fs);
        free_unit_masks(info, img->unit_count);
        return diag_set(diag, STIGI_ERR_OOM, -1, "mask_ops OOM");
      }
      for (uint32_t r = 0; r < cnt; r++) {
        int64_t row[1] = {0};
        if (stigi_dl_read(&dl, rel_mask, r, row, 1, diag) == STIGI_OK) {
          size_t slen = 0;
          const char *s = stigi_dl_sym_str(&dl, (uint32_t)row[0], &slen);
          if (s != NULL) {
            char *cp = malloc(slen + 1U);
            if (cp == NULL) {
              for (uint32_t k = 0; k < mask_op_count; k++) free(mask_ops[k]);
              free(mask_ops);
              stigi_dl_close(&dl);
              stigi_facts_free(&fs);
              free_unit_masks(info, img->unit_count);
              return diag_set(diag, STIGI_ERR_OOM, -1, "mask_ops strdup OOM");
            }
            memcpy(cp, s, slen);
            cp[slen] = '\0';
            mask_ops[mask_op_count++] = cp;
          }
        }
      }
      qsort(mask_ops, mask_op_count, sizeof *mask_ops, cmp_cstr_ptr);
    }
  }

  /* Map each indexRead/indexWrite fact row in extraction order to its unit's access ordinal */
  uint32_t total_masked = 0;
  for (uint32_t r = 0; r < fs.count; r++) {
    const StigiFactRow *fr = &fs.rows[r];
    if (strcmp(fr->rel, "indexRead") != 0 && strcmp(fr->rel, "indexWrite") != 0) continue;
    if (fr->ncols < 1 || fr->cols[0].is_num || fr->cols[0].sym == NULL) continue;
    const char *op_sym = fr->cols[0].sym;
    const char *hash = strstr(op_sym, "#0:");
    if (hash == NULL) continue;
    size_t ulen = (size_t)(hash - op_sym);
    int32_t uidx = stigi_image_find(img, op_sym, ulen);
    if (uidx < 0) continue;

    UnitMaskInfo *ui = &info[uidx];
    if (ui->access_count == ui->access_cap) {
      uint32_t ncap = ui->access_cap == 0 ? 8U : ui->access_cap * 2U;
      bool *nb = realloc(ui->access_masked, (size_t)ncap * sizeof *nb);
      if (nb == NULL) {
        for (uint32_t k = 0; k < mask_op_count; k++) free(mask_ops[k]);
        free(mask_ops);
        stigi_dl_close(&dl);
        stigi_facts_free(&fs);
        free_unit_masks(info, img->unit_count);
        return diag_set(diag, STIGI_ERR_OOM, -1, "access_masked OOM");
      }
      ui->access_masked = nb;
      ui->access_cap = ncap;
    }
    bool is_m = (mask_op_count > 0) &&
                sorted_cstr_contains((const char *const *)mask_ops, mask_op_count, op_sym);
    ui->access_masked[ui->access_count++] = is_m;
    if (is_m) total_masked++;
  }

  for (uint32_t k = 0; k < mask_op_count; k++) free(mask_ops[k]);
  free(mask_ops);
  stigi_dl_close(&dl);
  stigi_facts_free(&fs);
  *out_info = info;
  *out_total_masks = total_masked;
  return STIGI_OK;
}

/* ---- Emitter context & deterministic enum resolution (Defect 1) ---- */

typedef struct {
  StigiStr name;
  const StigiType *ast_type;
  const StigiExpr *len_expr; /* for Array[A](n): any expr (LOCAL, LIT_INT, GLOBAL, BINOP) */
  uint32_t type_uidx;
  EmitScalarTy scalar_ty;
  EmitScalarTy elem_scalar_ty;
  bool is_array;
  bool is_param_array;
  int32_t enum_uidx; /* >= 0 if enum type */
} EmitVar;

typedef struct {
  const StigiImage *img;
  uint32_t uidx;
  const StigiDecl *cur_def;
  EmitBuf *b;
  EmitBuf *pre; /* statement-level hoist buffer for left-to-right call args */
  StigiEmitStats *stats;
  const UnitMaskInfo *mask_info;
  uint32_t access_seq;
  EmitVar vars[128];
  uint32_t var_count;
  uint32_t tmp_id;
  bool is_tail_loop;
  bool is_never_def;
  EmitScalarTy ret_scalar;
} EmitCtx;

static const EmitVar *emit_find_var(const EmitCtx *ctx, StigiStr name) {
  for (uint32_t i = ctx->var_count; i > 0; i--) {
    if (ctx->vars[i - 1].name.len == name.len &&
        memcmp(ctx->vars[i - 1].name.ptr, name.ptr, name.len) == 0) {
      return &ctx->vars[i - 1];
    }
  }
  return NULL;
}

static bool ctx_next_access_masked(EmitCtx *ctx) {
  uint32_t seq = ctx->access_seq++;
  if (ctx->mask_info != NULL && seq < ctx->mask_info->access_count) {
    return ctx->mask_info->access_masked[seq];
  }
  return false;
}

static void emit_register_var(EmitCtx *ctx, StigiStr name, const StigiType *ty,
                              uint32_t type_uidx, bool is_param) {
  if (ctx->var_count >= 128) return;
  EmitVar *v = &ctx->vars[ctx->var_count++];
  memset(v, 0, sizeof *v);
  v->name = name;
  v->ast_type = ty;
  v->type_uidx = type_uidx;
  v->enum_uidx = -1;
  if (ty == NULL) return;

  uint32_t r_uidx = type_uidx;
  const StigiType *rty = resolve_typedef_type(ctx->img, &r_uidx, ty, 0);
  v->scalar_ty = ast_type_to_scalar(ctx->img, type_uidx, ty, 0);
  if (rty != NULL && rty->kind == STIGI_TYPE_ARRAY) {
    v->is_array = true;
    v->is_param_array = is_param;
    if (rty->len_arg_count > 0) {
      v->len_expr = rty->len_args[0];
    }
    if (rty->type_arg_count > 0) {
      v->elem_scalar_ty = ast_type_to_scalar(ctx->img, r_uidx, rty->type_args[0], 0);
    }
  } else if (rty != NULL && rty->kind == STIGI_TYPE_NAMED) {
    int32_t et = stigi_image_resolve_name(ctx->img, r_uidx, rty->name.ptr, rty->name.len);
    if (et >= 0 && ctx->img->units[et].ast->decl->kind == STIGI_DECL_ENUM) {
      v->enum_uidx = et;
    }
  }
}

static size_t unit_ns_prefix_len(const char *name, size_t len) {
  size_t last = 0;
  for (size_t i = 0; i < len; i++) {
    if (name[i] == '.') last = i;
  }
  return last;
}

/* Search ONLY inside a specific resolved enum declaration (Defect 1) */
static bool find_case_in_enum(const StigiDecl *ed, StigiStr case_name, uint32_t *out_cidx) {
  if (ed == NULL || ed->kind != STIGI_DECL_ENUM) return false;
  for (uint32_t c = 0; c < ed->u.enum_decl.case_count; c++) {
    if (ed->u.enum_decl.cases[c].name.len == case_name.len &&
        memcmp(ed->u.enum_decl.cases[c].name.ptr, case_name.ptr, case_name.len) == 0) {
      if (out_cidx != NULL) *out_cidx = c;
      return true;
    }
  }
  return false;
}

/* Deterministic enum case lookup: visibility -> same namespace -> 32-byte unit_id -> name (Defect 1) */
static int32_t find_enum_case_owner_det(const StigiImage *img, uint32_t from_idx,
                                        StigiStr case_name, uint32_t *out_cidx) {
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

/* Defect 4: recurse into MATCH arms and HANDLE bodies */
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
      }
      return ETY_UNKNOWN;
    }
    case STIGI_EXPR_CALL: {
      if (e->u.call.callee->kind == STIGI_EXPR_LOCAL && e->u.call.arg_count == 1) {
        const EmitVar *av = emit_find_var(ctx, e->u.call.callee->u.ident.name);
        if (av != NULL && av->is_array) return av->elem_scalar_ty;
      }
      if (e->u.call.callee->kind == STIGI_EXPR_LOCAL ||
          e->u.call.callee->kind == STIGI_EXPR_GLOBAL) {
        if (str_eq_c(e->u.call.callee->u.ident.name, "Stigi.Test.expect")) return ETY_I32;
        int32_t tgt = stigi_image_resolve_name(ctx->img, ctx->uidx,
                                               e->u.call.callee->u.ident.name.ptr,
                                               e->u.call.callee->u.ident.name.len);
        if (tgt >= 0) {
          const StigiDecl *d = ctx->img->units[tgt].ast->decl;
          if (d->kind == STIGI_DECL_DEF) {
            return ast_type_to_scalar(ctx->img, (uint32_t)tgt, d->u.def_decl.head.ret_type, 0);
          }
        }
      }
      return ETY_UNKNOWN;
    }
    case STIGI_EXPR_INDEX_READ: {
      if (e->u.index_expr.target->kind == STIGI_EXPR_LOCAL) {
        const EmitVar *av = emit_find_var(ctx, e->u.index_expr.target->u.ident.name);
        if (av != NULL && av->is_array) return av->elem_scalar_ty;
      }
      return ETY_UNKNOWN;
    }
    case STIGI_EXPR_UNOP:
      if (e->u.unop.op == STIGI_OP_NOT) return ETY_BOOL;
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
    case STIGI_EXPR_ASSERT:
      return infer_expr_scalar(ctx, e->u.assert_expr.expr);
    case STIGI_EXPR_BLOCK:
      if (e->u.block.count > 0) {
        return infer_expr_scalar(ctx, e->u.block.stmts[e->u.block.count - 1]);
      }
      return ETY_UNIT;
    case STIGI_EXPR_HANDLE:
      return infer_expr_scalar(ctx, e->u.handle_stmt.body);
    default:
      return ETY_UNKNOWN;
  }
}

static int32_t infer_expr_enum_uidx(const EmitCtx *ctx, const StigiExpr *e) {
  if (e == NULL) return -1;
  if (e->kind == STIGI_EXPR_LOCAL) {
    const EmitVar *sv = emit_find_var(ctx, e->u.ident.name);
    if (sv != NULL && sv->enum_uidx >= 0) return sv->enum_uidx;
    return find_enum_case_owner_det(ctx->img, ctx->uidx, e->u.ident.name, NULL);
  }
  if (e->kind == STIGI_EXPR_GLOBAL) {
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
    }
  }
  if (e->kind == STIGI_EXPR_CALL &&
      (e->u.call.callee->kind == STIGI_EXPR_LOCAL || e->u.call.callee->kind == STIGI_EXPR_GLOBAL)) {
    int32_t tgt = stigi_image_resolve_name(ctx->img, ctx->uidx,
                                           e->u.call.callee->u.ident.name.ptr,
                                           e->u.call.callee->u.ident.name.len);
    if (tgt >= 0) {
      const StigiDecl *d = ctx->img->units[tgt].ast->decl;
      if (d->kind == STIGI_DECL_DEF && d->u.def_decl.head.ret_type != NULL) {
        uint32_t ru = (uint32_t)tgt;
        const StigiType *rt = resolve_typedef_type(ctx->img, &ru, d->u.def_decl.head.ret_type, 0);
        if (rt != NULL && rt->kind == STIGI_TYPE_NAMED) {
          int32_t et = stigi_image_resolve_name(ctx->img, ru, rt->name.ptr, rt->name.len);
          if (et >= 0 && ctx->img->units[et].ast->decl->kind == STIGI_DECL_ENUM) return et;
        }
      }
    } else if (e->u.call.callee->kind == STIGI_EXPR_LOCAL) {
      return find_enum_case_owner_det(ctx->img, ctx->uidx, e->u.call.callee->u.ident.name, NULL);
    }
  }
  return -1;
}

static bool expr_is_complex_stmt(const StigiExpr *e) {
  if (e == NULL) return false;
  switch (e->kind) {
    case STIGI_EXPR_MATCH:
    case STIGI_EXPR_BLOCK:
    case STIGI_EXPR_HANDLE:
      return true;
    case STIGI_EXPR_IF:
      return expr_is_complex_stmt(e->u.if_expr.then_br) ||
             expr_is_complex_stmt(e->u.if_expr.else_br);
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

/* Helper: hoist a single argument into ctx->pre for strict left-to-right call evaluation (Defect 5) */
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
    EmitScalarTy st = hint != ETY_UNKNOWN ? hint : infer_expr_scalar(ctx, arg);
    if (st == ETY_UNKNOWN) {
      int32_t euidx = infer_expr_enum_uidx(ctx, arg);
      if (euidx >= 0) {
        eb_mangle_name(ctx->pre, ctx->img->units[euidx].name, ctx->img->units[euidx].name_len);
      } else {
        eb_cstr(ctx->pre, "uint64_t");
      }
    } else {
      emit_scalar_c_type(ctx->pre, st);
    }
  }
  eb_fmt(ctx->pre, " _a%u = ", tid);
  eb_mem(ctx->pre, val_buf.buf, val_buf.len);
  eb_cstr(ctx->pre, ";\n");
  if (val_buf.oom) ctx->pre->oom = true;
  free(val_buf.buf);
  return tid;
}

/* Lower value-producing complex expressions (MATCH/BLOCK/HANDLE/IF) into an assignment to lhs_var */
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
    ctx->var_count = saved;
    eb_cstr(ctx->b, "  }\n");
    return;
  }
  if (e->kind == STIGI_EXPR_HANDLE) {
    uint32_t saved = ctx->var_count;
    eb_cstr(ctx->b, "  {\n");
    emit_assign_to(ctx, e->u.handle_stmt.body, lhs_var, hint);
    ctx->var_count = saved;
    eb_cstr(ctx->b, "  }\n");
    return;
  }
  if (e->kind == STIGI_EXPR_IF) {
    EmitBuf cond_buf;
    emit_expr_with_pre(ctx, e->u.if_expr.cond, ETY_BOOL, &cond_buf);
    eb_line_dir(ctx->b, e->span.line, u->name, u->name_len);
    eb_cstr(ctx->b, "  if (");
    eb_mem(ctx->b, cond_buf.buf, cond_buf.len);
    eb_cstr(ctx->b, ") {\n");
    free(cond_buf.buf);
    uint32_t saved = ctx->var_count;
    emit_assign_to(ctx, e->u.if_expr.then_br, lhs_var, hint);
    ctx->var_count = saved;
    eb_cstr(ctx->b, "  } else {\n");
    emit_assign_to(ctx, e->u.if_expr.else_br, lhs_var, hint);
    ctx->var_count = saved;
    eb_cstr(ctx->b, "  }\n");
    return;
  }
  if (e->kind == STIGI_EXPR_MATCH) {
    int32_t enum_uidx = infer_expr_enum_uidx(ctx, e->u.match_expr.scrutinee);
    if (enum_uidx < 0 && e->u.match_expr.arm_count > 0 &&
        e->u.match_expr.arms[0].pat.kind == STIGI_PAT_CTOR) {
      enum_uidx = find_enum_case_owner_det(ctx->img, ctx->uidx,
                                           e->u.match_expr.arms[0].pat.name, NULL);
    }
    if (enum_uidx >= 0) {
      const StigiDecl *ed = ctx->img->units[enum_uidx].ast->decl;
      uint32_t sid = ctx->tmp_id++;
      EmitBuf scr_buf;
      emit_expr_with_pre(ctx, e->u.match_expr.scrutinee, ETY_UNKNOWN, &scr_buf);
      eb_line_dir(ctx->b, e->span.line, u->name, u->name_len);
      eb_cstr(ctx->b, "  ");
      eb_mangle_name(ctx->b, ctx->img->units[enum_uidx].name, ctx->img->units[enum_uidx].name_len);
      eb_fmt(ctx->b, " _sc%u = ", sid);
      eb_mem(ctx->b, scr_buf.buf, scr_buf.len);
      free(scr_buf.buf);
      eb_fmt(ctx->b, ";\n  switch (_sc%u.tag) {\n", sid);
      for (uint32_t a = 0; a < e->u.match_expr.arm_count; a++) {
        const StigiPat *pat = &e->u.match_expr.arms[a].pat;
        uint32_t cidx = 0;
        if (!find_case_in_enum(ed, pat->name, &cidx)) continue;
        eb_cstr(ctx->b, "    case ");
        eb_mangle_name(ctx->b, ctx->img->units[enum_uidx].name, ctx->img->units[enum_uidx].name_len);
        eb_cstr(ctx->b, "_");
        eb_str(ctx->b, pat->name);
        eb_cstr(ctx->b, ": {\n");
        const StigiEnumCase *ec = &ed->u.enum_decl.cases[cidx];
        uint32_t saved = ctx->var_count;
        for (uint32_t f = 0; f < pat->field_count && f < ec->field_count; f++) {
          if (pat->fields[f].len == 1 && pat->fields[f].ptr[0] == '_') continue;
          if (ast_type_is_void(ctx->img, (uint32_t)enum_uidx, ec->fields[f].type)) continue;
          eb_cstr(ctx->b, "      ");
          emit_c_type(ctx->b, ctx->img, (uint32_t)enum_uidx, ec->fields[f].type, 0);
          eb_cstr(ctx->b, " ");
          eb_str(ctx->b, pat->fields[f]);
          eb_fmt(ctx->b, " = _sc%u.u.", sid);
          eb_str(ctx->b, ec->name);
          eb_cstr(ctx->b, ".");
          eb_str(ctx->b, ec->fields[f].name);
          eb_cstr(ctx->b, ";\n");
          emit_register_var(ctx, pat->fields[f], ec->fields[f].type, (uint32_t)enum_uidx, false);
        }
        emit_assign_to(ctx, e->u.match_expr.arms[a].body, lhs_var, hint);
        ctx->var_count = saved;
        eb_cstr(ctx->b, "      break;\n    }\n");
      }
      eb_cstr(ctx->b, "  }\n");
      return;
    }
  }
  EmitBuf rhs_buf;
  emit_expr_with_pre(ctx, e, hint, &rhs_buf);
  eb_fmt(ctx->b, "  %s = ", lhs_var);
  eb_mem(ctx->b, rhs_buf.buf, rhs_buf.len);
  eb_cstr(ctx->b, ";\n");
  free(rhs_buf.buf);
}

static void emit_match_stmt(EmitCtx *ctx, const StigiExpr *e, bool is_tail, bool is_void) {
  const StigiImageUnit *u = &ctx->img->units[ctx->uidx];
  int32_t enum_uidx = infer_expr_enum_uidx(ctx, e->u.match_expr.scrutinee);
  if (enum_uidx < 0 && e->u.match_expr.arm_count > 0 &&
      e->u.match_expr.arms[0].pat.kind == STIGI_PAT_CTOR) {
    enum_uidx = find_enum_case_owner_det(ctx->img, ctx->uidx,
                                         e->u.match_expr.arms[0].pat.name, NULL);
  }
  if (enum_uidx < 0) return;

  const StigiDecl *ed = ctx->img->units[enum_uidx].ast->decl;
  uint32_t sid = ctx->tmp_id++;
  EmitBuf scr_buf;
  emit_expr_with_pre(ctx, e->u.match_expr.scrutinee, ETY_UNKNOWN, &scr_buf);
  eb_line_dir(ctx->b, e->span.line, u->name, u->name_len);
  eb_cstr(ctx->b, "  ");
  eb_mangle_name(ctx->b, ctx->img->units[enum_uidx].name, ctx->img->units[enum_uidx].name_len);
  eb_fmt(ctx->b, " _sc%u = ", sid);
  eb_mem(ctx->b, scr_buf.buf, scr_buf.len);
  free(scr_buf.buf);
  eb_fmt(ctx->b, ";\n  switch (_sc%u.tag) {\n", sid);

  /* Every enum case, NO default (emitter.md 2), lookup strictly within resolved ed (Defect 1) */
  for (uint32_t a = 0; a < e->u.match_expr.arm_count; a++) {
    const StigiPat *pat = &e->u.match_expr.arms[a].pat;
    uint32_t cidx = 0;
    if (!find_case_in_enum(ed, pat->name, &cidx)) continue;
    eb_cstr(ctx->b, "    case ");
    eb_mangle_name(ctx->b, ctx->img->units[enum_uidx].name, ctx->img->units[enum_uidx].name_len);
    eb_cstr(ctx->b, "_");
    eb_str(ctx->b, pat->name);
    eb_cstr(ctx->b, ": {\n");
    const StigiEnumCase *ec = &ed->u.enum_decl.cases[cidx];
    uint32_t saved = ctx->var_count;
    for (uint32_t f = 0; f < pat->field_count && f < ec->field_count; f++) {
      if (pat->fields[f].len == 1 && pat->fields[f].ptr[0] == '_') continue;
      if (ast_type_is_void(ctx->img, (uint32_t)enum_uidx, ec->fields[f].type)) continue;
      eb_cstr(ctx->b, "      ");
      emit_c_type(ctx->b, ctx->img, (uint32_t)enum_uidx, ec->fields[f].type, 0);
      eb_cstr(ctx->b, " ");
      eb_str(ctx->b, pat->fields[f]);
      eb_fmt(ctx->b, " = _sc%u.u.", sid);
      eb_str(ctx->b, ec->name);
      eb_cstr(ctx->b, ".");
      eb_str(ctx->b, ec->fields[f].name);
      eb_cstr(ctx->b, ";\n");
      emit_register_var(ctx, pat->fields[f], ec->fields[f].type, (uint32_t)enum_uidx, false);
    }
    emit_stmt_or_ret(ctx, e->u.match_expr.arms[a].body, is_tail, is_void);
    ctx->var_count = saved;
    /* Defect 4: always break on void / non-tail match arms to prevent fallthrough */
    if (is_void || !is_tail) {
      eb_cstr(ctx->b, "      break;\n");
    }
    eb_cstr(ctx->b, "    }\n");
  }
  eb_cstr(ctx->b, "  }\n");
  if (is_tail && !is_void) {
    eb_cstr(ctx->b, "  __builtin_unreachable();\n");
  } else if (is_tail && is_void && ctx->is_tail_loop) {
    eb_cstr(ctx->b, "  return;\n");
  }
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

  /* Defect 4: save/restore var_count across block scope */
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
    ctx->var_count = saved;
    eb_cstr(ctx->b, "  }\n");
    return;
  }

  /* Defect 4: handle HANDLE blocks with lexical scope save/restore */
  if (e->kind == STIGI_EXPR_HANDLE) {
    uint32_t saved = ctx->var_count;
    eb_cstr(ctx->b, "  {\n");
    if (e->u.handle_stmt.body != NULL) {
      emit_stmt_or_ret(ctx, e->u.handle_stmt.body, is_tail, is_void);
    } else if (is_tail && is_void && ctx->is_tail_loop) {
      eb_cstr(ctx->b, "  return;\n");
    }
    ctx->var_count = saved;
    eb_cstr(ctx->b, "  }\n");
    return;
  }

  /* Tail self-call -> update params left-to-right & continue (emitter.md 2) */
  if (is_tail && ctx->is_tail_loop && is_self_tail_call(ctx, e)) {
    const StigiDefHead *h = &ctx->cur_def->u.def_decl.head;
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
      eb_str(ctx->b, h->params[i].name);
      eb_fmt(ctx->b, " = _t%u;\n", i);
    }
    eb_cstr(ctx->b, "    continue;\n  }\n");
    return;
  }

  /* Handle IF in both tail and statement positions (Defect 4) */
  if (e->kind == STIGI_EXPR_IF) {
    EmitBuf cond_buf;
    emit_expr_with_pre(ctx, e->u.if_expr.cond, ETY_BOOL, &cond_buf);
    eb_line_dir(ctx->b, e->span.line, u->name, u->name_len);
    eb_cstr(ctx->b, "  if (");
    eb_mem(ctx->b, cond_buf.buf, cond_buf.len);
    eb_cstr(ctx->b, ") {\n");
    free(cond_buf.buf);
    uint32_t saved = ctx->var_count;
    emit_stmt_or_ret(ctx, e->u.if_expr.then_br, is_tail, is_void);
    ctx->var_count = saved;
    if (e->u.if_expr.else_br != NULL) {
      eb_cstr(ctx->b, "  } else {\n");
      emit_stmt_or_ret(ctx, e->u.if_expr.else_br, is_tail, is_void);
      ctx->var_count = saved;
      eb_cstr(ctx->b, "  }\n");
    } else if (is_tail && is_void && ctx->is_tail_loop) {
      eb_cstr(ctx->b, "  } else {\n    return;\n  }\n");
    } else {
      eb_cstr(ctx->b, "  }\n");
    }
    return;
  }

  /* Handle MATCH in both tail and non-tail statement positions (Defect 4) */
  if (e->kind == STIGI_EXPR_MATCH) {
    emit_match_stmt(ctx, e, is_tail, is_void);
    return;
  }

  eb_line_dir(ctx->b, e->span.line, u->name, u->name_len);
  if (e->kind == STIGI_EXPR_VAL) {
    if (ast_type_is_void(ctx->img, ctx->uidx, e->u.val_stmt.type)) {
      emit_stmt_or_ret(ctx, e->u.val_stmt.init, false, true);
      if (ast_type_is_never(ctx->img, ctx->uidx, e->u.val_stmt.type)) {
        eb_cstr(ctx->b, "  __builtin_unreachable();\n");
      }
      if (is_tail && is_void && ctx->is_tail_loop) eb_cstr(ctx->b, "  return;\n");
      return;
    }
    EmitScalarTy vhint = ast_type_to_scalar(ctx->img, ctx->uidx, e->u.val_stmt.type, 0);
    if (expr_is_complex_stmt(e->u.val_stmt.init)) {
      uint32_t tid = ctx->tmp_id++;
      char tname[32];
      (void)snprintf(tname, sizeof tname, "_v%u", tid);
      eb_cstr(ctx->b, "  ");
      emit_c_type(ctx->b, ctx->img, ctx->uidx, e->u.val_stmt.type, 0);
      eb_fmt(ctx->b, " %s;\n", tname);
      emit_assign_to(ctx, e->u.val_stmt.init, tname, vhint);
      eb_cstr(ctx->b, "  const ");
      emit_c_type(ctx->b, ctx->img, ctx->uidx, e->u.val_stmt.type, 0);
      eb_cstr(ctx->b, " ");
      eb_str(ctx->b, e->u.val_stmt.pat.name);
      eb_fmt(ctx->b, " = %s;\n", tname);
    } else {
      EmitBuf init_buf;
      emit_expr_with_pre(ctx, e->u.val_stmt.init, vhint, &init_buf);
      eb_cstr(ctx->b, "  const ");
      emit_c_type(ctx->b, ctx->img, ctx->uidx, e->u.val_stmt.type, 0);
      eb_cstr(ctx->b, " ");
      eb_str(ctx->b, e->u.val_stmt.pat.name);
      eb_cstr(ctx->b, " = ");
      eb_mem(ctx->b, init_buf.buf, init_buf.len);
      eb_cstr(ctx->b, ";\n");
      free(init_buf.buf);
    }
    emit_register_var(ctx, e->u.val_stmt.pat.name, e->u.val_stmt.type, ctx->uidx, false);
    if (is_tail && is_void && ctx->is_tail_loop) eb_cstr(ctx->b, "  return;\n");
    return;
  }
  if (e->kind == STIGI_EXPR_VAR) {
    if (ast_type_is_void(ctx->img, ctx->uidx, e->u.var_stmt.type)) {
      emit_stmt_or_ret(ctx, e->u.var_stmt.init, false, true);
      if (is_tail && is_void && ctx->is_tail_loop) eb_cstr(ctx->b, "  return;\n");
      return;
    }
    EmitScalarTy vhint = ast_type_to_scalar(ctx->img, ctx->uidx, e->u.var_stmt.type, 0);
    if (expr_is_complex_stmt(e->u.var_stmt.init)) {
      char vname[128];
      (void)snprintf(vname, sizeof vname, "%.*s", (int)e->u.var_stmt.name.len,
                     e->u.var_stmt.name.ptr);
      eb_cstr(ctx->b, "  ");
      emit_c_type(ctx->b, ctx->img, ctx->uidx, e->u.var_stmt.type, 0);
      eb_fmt(ctx->b, " %s;\n", vname);
      emit_assign_to(ctx, e->u.var_stmt.init, vname, vhint);
    } else {
      EmitBuf init_buf;
      emit_expr_with_pre(ctx, e->u.var_stmt.init, vhint, &init_buf);
      eb_cstr(ctx->b, "  ");
      emit_c_type(ctx->b, ctx->img, ctx->uidx, e->u.var_stmt.type, 0);
      eb_cstr(ctx->b, " ");
      eb_str(ctx->b, e->u.var_stmt.name);
      eb_cstr(ctx->b, " = ");
      eb_mem(ctx->b, init_buf.buf, init_buf.len);
      eb_cstr(ctx->b, ";\n");
      free(init_buf.buf);
    }
    emit_register_var(ctx, e->u.var_stmt.name, e->u.var_stmt.type, ctx->uidx, false);
    if (is_tail && is_void && ctx->is_tail_loop) eb_cstr(ctx->b, "  return;\n");
    return;
  }
  if (e->kind == STIGI_EXPR_ASSIGN) {
    const EmitVar *av = emit_find_var(ctx, e->u.assign_stmt.name);
    EmitScalarTy vhint = av != NULL ? av->scalar_ty : ETY_UNKNOWN;
    if (expr_is_complex_stmt(e->u.assign_stmt.value)) {
      char vname[128];
      (void)snprintf(vname, sizeof vname, "%.*s", (int)e->u.assign_stmt.name.len,
                     e->u.assign_stmt.name.ptr);
      emit_assign_to(ctx, e->u.assign_stmt.value, vname, vhint);
    } else {
      EmitBuf val_buf;
      emit_expr_with_pre(ctx, e->u.assign_stmt.value, vhint, &val_buf);
      eb_cstr(ctx->b, "  ");
      eb_str(ctx->b, e->u.assign_stmt.name);
      eb_cstr(ctx->b, " = ");
      eb_mem(ctx->b, val_buf.buf, val_buf.len);
      eb_cstr(ctx->b, ";\n");
      free(val_buf.buf);
    }
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
    } else {
      eb_cstr(ctx->b, "  while (");
      eb_mem(ctx->b, cbuf.buf, cbuf.len);
      eb_cstr(ctx->b, ") {\n");
    }
    free(cpre.buf);
    free(cbuf.buf);
    uint32_t saved = ctx->var_count;
    emit_stmt_or_ret(ctx, e->u.while_expr.body, false, true);
    ctx->var_count = saved;
    eb_cstr(ctx->b, "  }\n");
    if (is_tail && is_void && ctx->is_tail_loop) eb_cstr(ctx->b, "  return;\n");
    return;
  }
  /* Defect 6 & 7: mask tainted INDEX_WRITE through Stigi_mask_index using Datalog maskIndex */
  if (e->kind == STIGI_EXPR_INDEX_WRITE) {
    EmitScalarTy elem_ty = ETY_UNKNOWN;
    const StigiExpr *len_expr = find_array_len_expr(ctx, e->u.index_expr.target, &elem_ty);
    EmitBuf tgt_buf, idx_buf, val_buf;
    emit_expr_with_pre(ctx, e->u.index_expr.target, ETY_UNKNOWN, &tgt_buf);
    emit_expr_with_pre(ctx, e->u.index_expr.index, ETY_U64, &idx_buf);
    emit_expr_with_pre(ctx, e->u.index_expr.value, elem_ty, &val_buf);
    bool do_mask = ctx_next_access_masked(ctx);
    eb_cstr(ctx->b, "  ");
    eb_mem(ctx->b, tgt_buf.buf, tgt_buf.len);
    eb_cstr(ctx->b, "[");
    if (do_mask && len_expr != NULL) {
      EmitBuf len_buf;
      emit_expr_with_pre(ctx, len_expr, ETY_U64, &len_buf);
      ctx->stats->spectre_masks++;
      eb_cstr(ctx->b, "Stigi_mask_index((uint64_t)(");
      eb_mem(ctx->b, idx_buf.buf, idx_buf.len);
      eb_cstr(ctx->b, "), (uint64_t)(");
      eb_mem(ctx->b, len_buf.buf, len_buf.len);
      eb_cstr(ctx->b, "))");
      free(len_buf.buf);
    } else {
      eb_mem(ctx->b, idx_buf.buf, idx_buf.len);
    }
    eb_cstr(ctx->b, "] = ");
    eb_mem(ctx->b, val_buf.buf, val_buf.len);
    eb_cstr(ctx->b, ";\n");
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
      if (infer_expr_scalar(ctx, e) == ETY_UNIT || infer_expr_scalar(ctx, e) == ETY_NEVER) {
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
    /* Defect 4: void tail-loop base case MUST return; (never return expr;) */
    if (is_tail && is_void && ctx->is_tail_loop) {
      eb_cstr(ctx->b, "  return;\n");
    }
  }
}

static void emit_expr_typed(EmitCtx *ctx, const StigiExpr *e, EmitScalarTy hint) {
  if (e == NULL) { eb_cstr(ctx->b, "UINT64_C(0)"); return; }
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
      if (emit_find_var(ctx, e->u.ident.name) != NULL) {
        eb_str(ctx->b, e->u.ident.name);
        break;
      }
      int32_t tgt = stigi_image_resolve_name(ctx->img, ctx->uidx,
                                             e->u.ident.name.ptr, e->u.ident.name.len);
      if (tgt >= 0) {
        eb_mangle_name(ctx->b, ctx->img->units[tgt].name, ctx->img->units[tgt].name_len);
        break;
      }
      /* Nullary enum constructor: deterministic lookup (Defect 1) */
      uint32_t cidx = 0;
      int32_t eidx = find_enum_case_owner_det(ctx->img, ctx->uidx, e->u.ident.name, &cidx);
      if (eidx >= 0) {
        eb_cstr(ctx->b, "((");
        eb_mangle_name(ctx->b, ctx->img->units[eidx].name, ctx->img->units[eidx].name_len);
        eb_cstr(ctx->b, "){ .tag = ");
        eb_mangle_name(ctx->b, ctx->img->units[eidx].name, ctx->img->units[eidx].name_len);
        eb_cstr(ctx->b, "_");
        eb_str(ctx->b, e->u.ident.name);
        eb_cstr(ctx->b, " })");
        break;
      }
      eb_str(ctx->b, e->u.ident.name);
      break;
    }
    case STIGI_EXPR_UNOP:
      /* Defect 2: typed negative integer literal (-1 for I32/I64, never -1ULL) */
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
      eb_cstr(ctx->b, "(");
      emit_expr_typed(ctx, e->u.binop.lhs, op_hint);
      eb_fmt(ctx->b, " %s ", c_op_str(e->u.binop.op));
      /* Do not hoist side-effecting calls out of short-circuit RHS of && / || */
      if (e->u.binop.op == STIGI_OP_AND || e->u.binop.op == STIGI_OP_OR) {
        EmitBuf *saved_pre = ctx->pre;
        ctx->pre = NULL;
        emit_expr_typed(ctx, e->u.binop.rhs, op_hint);
        ctx->pre = saved_pre;
      } else {
        emit_expr_typed(ctx, e->u.binop.rhs, op_hint);
      }
      eb_cstr(ctx->b, ")");
      break;
    }
    case STIGI_EXPR_IF: {
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
      const StigiExpr *len_expr = find_array_len_expr(ctx, e->u.index_expr.target, NULL);
      EmitBuf idx_buf;
      memset(&idx_buf, 0, sizeof idx_buf);
      EmitBuf *saved_b = ctx->b;
      ctx->b = &idx_buf;
      emit_expr_typed(ctx, e->u.index_expr.index, ETY_U64);
      ctx->b = saved_b;
      bool do_mask = ctx_next_access_masked(ctx);
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
      /* Array element read: arr(idx) with Datalog maskIndex (Defects 6 & 7) */
      if (e->u.call.callee->kind == STIGI_EXPR_LOCAL && e->u.call.arg_count == 1) {
        const EmitVar *av = emit_find_var(ctx, e->u.call.callee->u.ident.name);
        if (av != NULL && av->is_array) {
          EmitBuf idx_buf;
          memset(&idx_buf, 0, sizeof idx_buf);
          EmitBuf *saved_b = ctx->b;
          ctx->b = &idx_buf;
          emit_expr_typed(ctx, e->u.call.args[0], ETY_U64);
          ctx->b = saved_b;
          bool do_mask = av->is_param_array ? ctx_next_access_masked(ctx) : false;
          eb_str(ctx->b, av->name);
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
      /* Stigi.Test.expect(got, want, reason) -> left-to-right eval into temps */
      if (e->u.call.callee->kind == STIGI_EXPR_GLOBAL &&
          str_eq_c(e->u.call.callee->u.ident.name, "Stigi.Test.expect") &&
          e->u.call.arg_count == 3) {
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
      /* Enum constructor call: Case(a0, a1) with deterministic lookup & L-to-R eval */
      if (e->u.call.callee->kind == STIGI_EXPR_LOCAL) {
        uint32_t cidx = 0;
        int32_t eidx = find_enum_case_owner_det(ctx->img, ctx->uidx,
                                                e->u.call.callee->u.ident.name, &cidx);
        if (eidx >= 0) {
          const StigiDecl *ed = ctx->img->units[eidx].ast->decl;
          const StigiEnumCase *ec = &ed->u.enum_decl.cases[cidx];
          uint32_t nfields = ec->field_count < e->u.call.arg_count ? ec->field_count
                                                                   : e->u.call.arg_count;
          uint32_t tids[64];
          bool hoisted = (ctx->pre != NULL && nfields >= 2 && nfields <= 64);
          if (hoisted) {
            for (uint32_t f = 0; f < nfields; f++) {
              EmitScalarTy fh = ast_type_to_scalar(ctx->img, (uint32_t)eidx, ec->fields[f].type, 0);
              tids[f] = hoist_arg(ctx, e->u.call.args[f], ec->fields[f].type, (uint32_t)eidx, fh);
            }
          }
          eb_cstr(ctx->b, "((");
          eb_mangle_name(ctx->b, ctx->img->units[eidx].name, ctx->img->units[eidx].name_len);
          eb_cstr(ctx->b, "){ ");
          if (nfields > 0) {
            eb_cstr(ctx->b, ".u = { .");
            eb_str(ctx->b, ec->name);
            eb_cstr(ctx->b, " = { ");
            bool first_f = true;
            for (uint32_t f = 0; f < nfields; f++) {
              if (ast_type_is_void(ctx->img, (uint32_t)eidx, ec->fields[f].type)) continue;
              if (!first_f) eb_cstr(ctx->b, ", ");
              first_f = false;
              eb_cstr(ctx->b, ".");
              eb_str(ctx->b, ec->fields[f].name);
              eb_cstr(ctx->b, " = ");
              if (hoisted) {
                eb_fmt(ctx->b, "_a%u", tids[f]);
              } else {
                EmitScalarTy fh = ast_type_to_scalar(ctx->img, (uint32_t)eidx, ec->fields[f].type, 0);
                emit_expr_typed(ctx, e->u.call.args[f], fh);
              }
            }
            eb_cstr(ctx->b, " } }, ");
          }
          eb_cstr(ctx->b, ".tag = ");
          eb_mangle_name(ctx->b, ctx->img->units[eidx].name, ctx->img->units[eidx].name_len);
          eb_cstr(ctx->b, "_");
          eb_str(ctx->b, ec->name);
          eb_cstr(ctx->b, " })");
          break;
        }
      }
      /* Regular def call: evaluate arguments left-to-right into temporaries (Defect 5) */
      int32_t tgt = -1;
      const StigiDefHead *callee_h = NULL;
      if (e->u.call.callee->kind == STIGI_EXPR_LOCAL ||
          e->u.call.callee->kind == STIGI_EXPR_GLOBAL) {
        tgt = stigi_image_resolve_name(ctx->img, ctx->uidx,
                                       e->u.call.callee->u.ident.name.ptr,
                                       e->u.call.callee->u.ident.name.len);
        if (tgt >= 0 && ctx->img->units[tgt].ast->decl->kind == STIGI_DECL_DEF) {
          callee_h = &ctx->img->units[tgt].ast->decl->u.def_decl.head;
        }
      }
      uint32_t argc = e->u.call.arg_count;
      uint32_t tids[64];
      bool hoisted = (ctx->pre != NULL && argc >= 2 && argc <= 64);
      if (hoisted) {
        for (uint32_t i = 0; i < argc; i++) {
          const StigiType *pty = (callee_h != NULL && i < callee_h->param_count)
                                     ? callee_h->params[i].type
                                     : NULL;
          uint32_t puidx = tgt >= 0 ? (uint32_t)tgt : ctx->uidx;
          EmitScalarTy ph = pty != NULL ? ast_type_to_scalar(ctx->img, puidx, pty, 0) : ETY_UNKNOWN;
          tids[i] = hoist_arg(ctx, e->u.call.args[i], pty, puidx, ph);
        }
      }
      if (tgt >= 0) {
        eb_mangle_name(ctx->b, ctx->img->units[tgt].name, ctx->img->units[tgt].name_len);
      } else if (e->u.call.callee->kind == STIGI_EXPR_LOCAL ||
                 e->u.call.callee->kind == STIGI_EXPR_GLOBAL) {
        eb_str(ctx->b, e->u.call.callee->u.ident.name);
      } else {
        emit_expr_typed(ctx, e->u.call.callee, ETY_UNKNOWN);
      }
      eb_cstr(ctx->b, "(");
      for (uint32_t i = 0; i < argc; i++) {
        if (i > 0) eb_cstr(ctx->b, ", ");
        if (hoisted) {
          eb_fmt(ctx->b, "_a%u", tids[i]);
        } else {
          const StigiType *pty = (callee_h != NULL && i < callee_h->param_count)
                                     ? callee_h->params[i].type
                                     : NULL;
          uint32_t puidx = tgt >= 0 ? (uint32_t)tgt : ctx->uidx;
          EmitScalarTy ph = pty != NULL ? ast_type_to_scalar(ctx->img, puidx, pty, 0) : ETY_UNKNOWN;
          emit_expr_typed(ctx, e->u.call.args[i], ph);
        }
      }
      eb_cstr(ctx->b, ")");
      break;
    }
    case STIGI_EXPR_HANDLE:
      if (e->u.handle_stmt.body != NULL) {
        emit_expr_typed(ctx, e->u.handle_stmt.body, hint);
      } else {
        eb_cstr(ctx->b, "UINT64_C(0)");
      }
      break;
    case STIGI_EXPR_BLOCK:
      if (e->u.block.count == 0) {
        eb_cstr(ctx->b, "UINT64_C(0)");
      } else if (e->u.block.count == 1) {
        emit_expr_typed(ctx, e->u.block.stmts[0], hint);
      } else {
        eb_cstr(ctx->b, "(");
        for (uint32_t i = 0; i < e->u.block.count; i++) {
          if (i > 0) eb_cstr(ctx->b, ", ");
          emit_expr_typed(ctx, e->u.block.stmts[i],
                          (i + 1 == e->u.block.count) ? hint : ETY_UNKNOWN);
        }
        eb_cstr(ctx->b, ")");
      }
      break;
    case STIGI_EXPR_MATCH:
    case STIGI_EXPR_WHILE:
    case STIGI_EXPR_VAL:
    case STIGI_EXPR_VAR:
    case STIGI_EXPR_ASSIGN:
    case STIGI_EXPR_INDEX_WRITE:
    case STIGI_EXPR_DENY:
    case STIGI_EXPR_LOCAL_DECL:
      eb_cstr(ctx->b, "UINT64_C(0)");
      break;
  }
}

typedef struct {
  const StigiImageUnit *u;
} UnitRef;

/* Deterministic ordering strictly by 32-byte runtime_id (emitter.md 1, NEVER by pointer!) */
static int cmp_unit_by_id(const void *pa, const void *pb) {
  const UnitRef *a = pa;
  const UnitRef *b = pb;
  int c = memcmp(a->u->unit_id, b->u->unit_id, 32);
  if (c != 0) return c;
  return strcmp(a->u->name, b->u->name);
}

static void emit_def_sig(EmitBuf *b, const StigiImage *img, uint32_t uidx, const StigiDecl *d) {
  const StigiDefHead *h = &d->u.def_decl.head;
  if (strcmp(img->units[uidx].name, "main") != 0) eb_cstr(b, "static ");
  emit_c_type(b, img, uidx, h->ret_type, 0);
  eb_cstr(b, " ");
  eb_mangle_name(b, img->units[uidx].name, img->units[uidx].name_len);
  eb_cstr(b, "(");
  if (h->param_count == 0) {
    eb_cstr(b, "void");
  } else {
    for (uint32_t i = 0; i < h->param_count; i++) {
      if (i > 0) eb_cstr(b, ", ");
      emit_c_type(b, img, uidx, h->params[i].type, 0);
      eb_cstr(b, " ");
      eb_str(b, h->params[i].name);
    }
  }
  eb_cstr(b, ")");
}

/* Emit alignment-sorted enum + tail tag + _Static_assert on sizeOf/alignOf/offsetof (Defect 3) */
static void emit_enum_unit(EmitBuf *b, const StigiImage *img, const StigiImageUnit *u,
                           const StigiDecl *d) {
  uint32_t case_count = d->u.enum_decl.case_count;
  uint64_t tag_size = (case_count <= 256U) ? 1ULL : (case_count <= 65536U ? 2ULL : 4ULL);
  const char *tag_ctype = (case_count <= 256U) ? "uint8_t"
                          : (case_count <= 65536U ? "uint16_t" : "uint32_t");

  eb_line_dir(b, d->span.line, u->name, u->name_len);
  eb_cstr(b, "typedef enum {\n");
  bool has_any_fields = false;
  for (uint32_t c = 0; c < case_count; c++) {
    const StigiEnumCase *ec = &d->u.enum_decl.cases[c];
    for (uint32_t f = 0; f < ec->field_count; f++) {
      if (ast_type_layout(img, u->idx, ec->fields[f].type, 0).size > 0) {
        has_any_fields = true;
      }
    }
    eb_cstr(b, "  ");
    eb_mangle_name(b, u->name, u->name_len);
    eb_cstr(b, "_");
    eb_str(b, ec->name);
    eb_fmt(b, " = %u%s\n", c, (c + 1 < case_count) ? "," : "");
  }
  eb_cstr(b, "} ");
  eb_mangle_name(b, u->name, u->name_len);
  eb_cstr(b, "_Tag;\n");

  /* Track field offsets for _Static_assert(offsetof(...)) */
  EmitBuf asserts_buf;
  memset(&asserts_buf, 0, sizeof asserts_buf);

  uint64_t union_align = 1;
  uint64_t union_size = 0;

  eb_cstr(b, "typedef struct {\n");
  if (has_any_fields) {
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

      eb_cstr(b, "    struct {\n");
      uint64_t c_align = 1;
      uint64_t off = 0;
      for (uint32_t i = 0; i < ec->field_count; i++) {
        uint32_t fi = order[i];
        StigiLayout fl = flays[fi];
        if (fl.size == 0) continue;
        if (fl.align > c_align) c_align = fl.align;
        off = emit_align_up(off, fl.align);
        uint64_t field_off = off;
        off += fl.size;

        eb_cstr(b, "      ");
        emit_c_type(b, img, u->idx, ec->fields[fi].type, 0);
        eb_cstr(b, " ");
        eb_str(b, ec->fields[fi].name);
        eb_cstr(b, ";\n");

        eb_cstr(&asserts_buf, "_Static_assert(offsetof(");
        eb_mangle_name(&asserts_buf, u->name, u->name_len);
        eb_cstr(&asserts_buf, ", u.");
        eb_str(&asserts_buf, ec->name);
        eb_cstr(&asserts_buf, ".");
        eb_str(&asserts_buf, ec->fields[fi].name);
        eb_fmt(&asserts_buf, ") == %lluULL, \"enum field offset mismatch\");\n",
               (unsigned long long)field_off);
      }
      eb_cstr(b, "    } ");
      eb_str(b, ec->name);
      eb_cstr(b, ";\n");

      uint64_t c_size = emit_align_up(off, c_align);
      if (c_align > union_align) union_align = c_align;
      if (c_size > union_size) union_size = c_size;

      free(flays);
      free(order);
    }
    eb_cstr(b, "  } u;\n");
  }
  eb_fmt(b, "  %s tag;\n} ", tag_ctype);
  eb_mangle_name(b, u->name, u->name_len);
  eb_cstr(b, ";\n");

  uint64_t tag_off = has_any_fields ? emit_align_up(emit_align_up(union_size, union_align), tag_size)
                                    : 0ULL;

  StigiTy ety;
  memset(&ety, 0, sizeof ety);
  ety.kind = STIGI_TY_ENUM;
  ety.unit_idx = u->idx;
  StigiLayout lay = stigi_ty_layout(img, &ety);

  eb_cstr(b, "_Static_assert(sizeof(");
  eb_mangle_name(b, u->name, u->name_len);
  eb_fmt(b, ") == %lluULL, \"enum sizeOf mismatch\");\n", (unsigned long long)lay.size);
  eb_cstr(b, "_Static_assert(_Alignof(");
  eb_mangle_name(b, u->name, u->name_len);
  eb_fmt(b, ") == %lluULL, \"enum alignOf mismatch\");\n", (unsigned long long)lay.align);
  eb_cstr(b, "_Static_assert(offsetof(");
  eb_mangle_name(b, u->name, u->name_len);
  eb_fmt(b, ", tag) == %lluULL, \"enum tag offset mismatch\");\n", (unsigned long long)tag_off);
  if (asserts_buf.len > 0) {
    eb_mem(b, asserts_buf.buf, asserts_buf.len);
  }
  if (asserts_buf.oom) b->oom = true;
  free(asserts_buf.buf);
  eb_cstr(b, "\n");
}

Status stigi_emit_c(const StigiDoor *door, char **out_c, size_t *out_len,
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

  /* Defect 7: consume Datalog maskIndex relation rather than an ad-hoc local heuristic */
  UnitMaskInfo *mask_infos = NULL;
  uint32_t total_dl_masks = 0;
  Status mst = compute_datalog_masks(door, &mask_infos, &total_dl_masks, diag);
  if (mst != STIGI_OK) {
    free(sorted);
    return mst;
  }
  bool need_mask = (total_dl_masks > 0);

  EmitBuf b;
  memset(&b, 0, sizeof b);

  /* Portable ISO C11 preamble + S118 runtime + X22-g Spectre barrier mask (ZERO abort/exit!) */
  eb_cstr(&b,
          "#define _GNU_SOURCE\n"
          "#include <stdint.h>\n"
          "#include <stdbool.h>\n"
          "#include <stddef.h>\n"
          "#include <signal.h>\n"
          "#include <sys/mman.h>\n\n"
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
  /* Defects 9, 10, 11: exact S118 startup order, retain guard-paged stack/pool across Stigi_main,
   * fail closed, zero unused warnings under -DSTIGI_PROFILE_RT_LOCKED */
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
          "  size_t page = 4096U;\n"
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
          "  if (madvise(st->pool, pool_sz, MADV_POPULATE_WRITE) != 0) {\n"
          "    stigi_rt_shutdown(st);\n"
          "    st->status = STIGI_RT_ERR_MEM_POPULATE;\n"
          "    return st->status;\n"
          "  }\n"
          "#else\n"
          "  volatile unsigned char *vp = (volatile unsigned char *)st->pool;\n"
          "  for (size_t off = 0U; off < pool_sz; off += page) {\n"
          "    vp[off] = 0U;\n"
          "  }\n"
          "#endif\n"
          "  /* S118 (3): Ignore SIGPIPE */\n"
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

  /* 1. Emit Enums + _Static_assert on sizeOf / alignOf / offsetof (Defect 3) */
  for (uint32_t i = 0; i < scnt; i++) {
    const StigiImageUnit *u = sorted[i].u;
    const StigiDecl *d = u->ast->decl;
    if (d->kind != STIGI_DECL_ENUM) continue;
    stats->enums_emitted++;
    stats->units_emitted++;
    emit_enum_unit(&b, img, u, d);
  }

  /* 2. Forward-declare all reached defs and Tests */
  for (uint32_t i = 0; i < scnt; i++) {
    const StigiImageUnit *u = sorted[i].u;
    const StigiDecl *d = u->ast->decl;
    if (d->kind == STIGI_DECL_LEMMA_DEF) {
      stats->lemmas_erased++;
      continue;
    }
    if (d->kind == STIGI_DECL_DEF) {
      emit_def_sig(&b, img, u->idx, d);
      eb_cstr(&b, ";\n");
    } else if (d->kind == STIGI_DECL_TEST) {
      eb_cstr(&b, "static int32_t ");
      eb_mangle_name(&b, u->name, u->name_len);
      eb_cstr(&b, "(void);\n");
    }
  }
  eb_cstr(&b, "\n");

  /* 3. Emit top-level vals */
  for (uint32_t i = 0; i < scnt; i++) {
    const StigiImageUnit *u = sorted[i].u;
    const StigiDecl *d = u->ast->decl;
    if (d->kind != STIGI_DECL_VAL) continue;
    if (ast_type_is_void(img, u->idx, d->u.val_decl.type)) continue;
    stats->units_emitted++;
    eb_line_dir(&b, d->span.line, u->name, u->name_len);
    eb_cstr(&b, "static const ");
    emit_c_type(&b, img, u->idx, d->u.val_decl.type, 0);
    eb_cstr(&b, " ");
    eb_mangle_name(&b, u->name, u->name_len);
    eb_cstr(&b, " = ");
    EmitCtx ctx;
    memset(&ctx, 0, sizeof ctx);
    ctx.img = img;
    ctx.uidx = u->idx;
    ctx.b = &b;
    ctx.stats = stats;
    ctx.mask_info = mask_infos != NULL ? &mask_infos[u->idx] : NULL;
    EmitScalarTy vhint = ast_type_to_scalar(img, u->idx, d->u.val_decl.type, 0);
    emit_expr_typed(&ctx, d->u.val_decl.body, vhint);
    eb_cstr(&b, ";\n\n");
  }

  /* 4. Emit defs and Tests */
  bool has_main = false;
  bool main_is_void = false;
  for (uint32_t i = 0; i < scnt; i++) {
    const StigiImageUnit *u = sorted[i].u;
    const StigiDecl *d = u->ast->decl;
    if (d->kind == STIGI_DECL_DEF) {
      if (strcmp(u->name, "main") == 0) {
        has_main = true;
        main_is_void = ast_type_is_void(img, u->idx, d->u.def_decl.head.ret_type);
      }
      stats->defs_emitted++;
      stats->units_emitted++;

      EmitCtx ctx;
      memset(&ctx, 0, sizeof ctx);
      ctx.img = img;
      ctx.uidx = u->idx;
      ctx.cur_def = d;
      ctx.b = &b;
      ctx.stats = stats;
      ctx.mask_info = mask_infos != NULL ? &mask_infos[u->idx] : NULL;
      ctx.is_tail_loop = has_self_tail_call(&ctx, d->u.def_decl.body);
      ctx.is_never_def = ast_type_is_never(img, u->idx, d->u.def_decl.head.ret_type);
      ctx.ret_scalar = ast_type_to_scalar(img, u->idx, d->u.def_decl.head.ret_type, 0);
      if (ctx.is_tail_loop) stats->tail_loops++;

      const StigiDefHead *h = &d->u.def_decl.head;
      for (uint32_t p = 0; p < h->param_count && ctx.var_count < 128; p++) {
        emit_register_var(&ctx, h->params[p].name, h->params[p].type, u->idx, true);
      }

      eb_line_dir(&b, d->span.line, u->name, u->name_len);
      emit_def_sig(&b, img, u->idx, d);
      eb_cstr(&b, " {\n");
      /* Suppress unused parameter warnings for length-only parameters (e.g. count in Array[U64](count)) */
      for (uint32_t p = 0; p < h->param_count; p++) {
        eb_cstr(&b, "  (void)");
        eb_str(&b, h->params[p].name);
        eb_cstr(&b, ";\n");
      }
      bool is_void = ast_type_is_void(img, u->idx, h->ret_type);
      if (ctx.is_tail_loop) {
        eb_cstr(&b, "  while (1) {\n");
        emit_stmt_or_ret(&ctx, d->u.def_decl.body, true, is_void);
        eb_cstr(&b, "  }\n");
      } else {
        emit_stmt_or_ret(&ctx, d->u.def_decl.body, true, is_void);
      }
      if (ctx.is_never_def) {
        eb_cstr(&b, "  __builtin_unreachable();\n");
      }
      eb_cstr(&b, "}\n\n");
    } else if (d->kind == STIGI_DECL_TEST) {
      stats->units_emitted++;
      EmitCtx ctx;
      memset(&ctx, 0, sizeof ctx);
      ctx.img = img;
      ctx.uidx = u->idx;
      ctx.b = &b;
      ctx.stats = stats;
      ctx.mask_info = mask_infos != NULL ? &mask_infos[u->idx] : NULL;
      ctx.ret_scalar = ETY_I32;
      eb_line_dir(&b, d->span.line, u->name, u->name_len);
      eb_cstr(&b, "static int32_t ");
      eb_mangle_name(&b, u->name, u->name_len);
      eb_cstr(&b, "(void) {\n");
      emit_stmt_or_ret(&ctx, d->u.val_decl.body, true, false);
      eb_cstr(&b, "}\n\n");
    }
  }

  /* 5. C entry point: reset #line (Defect 5), retain guard-paged stack/pool across Stigi_main (Defect 10) */
  eb_cstr(&b,
          "#line 1 \"<stigi-runtime-main>\"\n"
          "int main(void) {\n"
          "  StigiRtState rt_state;\n"
          "  StigiRtStatus rt = stigi_rt_init(&rt_state);\n"
          "  if (rt != STIGI_RT_OK) {\n"
          "    stigi_rt_shutdown(&rt_state);\n"
          "    return (int)rt;\n"
          "  }\n");
  for (uint32_t i = 0; i < scnt; i++) {
    const StigiImageUnit *u = sorted[i].u;
    if (u->ast->decl->kind == STIGI_DECL_TEST) {
      eb_cstr(&b, "  if (");
      eb_mangle_name(&b, u->name, u->name_len);
      eb_cstr(&b, "() != 0) {\n"
                  "    stigi_rt_shutdown(&rt_state);\n"
                  "    return 100;\n"
                  "  }\n");
    }
  }
  if (has_main) {
    if (main_is_void) {
      eb_cstr(&b,
              "  Stigi_main();\n"
              "  stigi_rt_shutdown(&rt_state);\n"
              "  return 0;\n");
    } else {
      eb_cstr(&b,
              "  int rc = (int)Stigi_main();\n"
              "  stigi_rt_shutdown(&rt_state);\n"
              "  return rc;\n");
    }
  } else {
    eb_cstr(&b,
            "  stigi_rt_shutdown(&rt_state);\n"
            "  return 0;\n");
  }
  eb_cstr(&b, "}\n");

  free_unit_masks(mask_infos, img->unit_count);
  free(sorted);
  if (b.oom) {
    free(b.buf);
    return diag_set(diag, STIGI_ERR_OOM, -1, "stigi_emit_c: OOM");
  }
  *out_c = b.buf;
  if (out_len != NULL) *out_len = b.len;
  return STIGI_OK;
}

/* Dynamic line reader for .su files (no fixed line[512] truncation, Defect 13) */
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

/* Parse GCC/Clang -fstack-usage .su file: `file:line:col:func\tbytes\tqualifier`
 * Defects 12 & 13: dynamic lines, strict bytes <= fb, validate strtoull endptr,
 * include DECL_TEST, fail closed if 0 emitted Stigi_* functions matched. */
Status stigi_emit_check_su(const char *su_path, const StigiImage *img,
                           char *err_buf, size_t err_cap) {
  if (su_path == NULL || img == NULL) return STIGI_ERR_INVALID;
  FILE *f = fopen(su_path, "r");
  if (f == NULL) {
    if (err_buf != NULL && err_cap > 0) {
      (void)snprintf(err_buf, err_cap, "cannot open .su file '%s'", su_path);
    }
    return STIGI_ERR_IO;
  }

  uint32_t expected_funcs = 0;
  for (uint32_t i = 0; i < img->unit_count; i++) {
    if (!img->units[i].reachable) continue;
    StigiDeclKind k = img->units[i].ast->decl->kind;
    if (k == STIGI_DECL_DEF || k == STIGI_DECL_TEST) expected_funcs++;
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

    /* Validate strtoull endptr and overflow (Defect 13) */
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

    /* Extract function name after last ':' in first column */
    const char *colon = strrchr(line, ':');
    const char *fn = colon != NULL ? colon + 1 : line;

    if (strcmp(qual, "static") != 0) {
      if (err_buf != NULL && err_cap > 0) {
        (void)snprintf(err_buf, err_cap, "function '%s' has non-static stack usage '%s'", fn, qual);
      }
      result = STIGI_ERR_INVALID;
      break;
    }

    /* Match against Stigi_<mangled> DEF and TEST units; enforce STRICT bytes <= fb */
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

  /* Defect 12: fail closed if 0 emitted Stigi_* functions matched in .su */
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
