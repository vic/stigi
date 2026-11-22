#include "names.h"
#include "blake3.h"
#include "parse.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void stigi_name_diags_init(StigiNameDiags *d) {
  if (d == NULL) return;
  d->items = NULL;
  d->count = 0;
  d->cap = 0;
}

void stigi_name_diags_free(StigiNameDiags *d) {
  if (d == NULL) return;
  free(d->items);
  d->items = NULL;
  d->count = 0;
  d->cap = 0;
}

const char *stigi_name_code_name(StigiNameCode code) {
  switch (code) {
    case STIGI_NAME_DUPLICATE: return "duplicate";
    case STIGI_NAME_UNRESOLVED: return "unresolved-name";
    case STIGI_NAME_PRIVATE_REF: return "private-reference";
    case STIGI_NAME_PUB_ON_TEST: return "pub-on-test";
    case STIGI_NAME_PUB_UNUSED: return "pub-unused";
    case STIGI_NAME_NO_READER: return "no-reader";
    case STIGI_NAME_CASE_PAIR: return "case-pair";
    case STIGI_NAME_AMBIGUOUS_GROUP: return "ambiguous-group";
  }
  return "unknown";
}

static Status add_ndiag(StigiNameDiags *d, StigiNameCode code, StigiSpan span,
                        Diag *diag, const char *fmt, ...);
static int32_t find_enum_by_case(const StigiImage *img, uint32_t from_idx,
                                 const char *case_name, size_t len);

/* ---- Canonical de Bruijn + dependency-hash BLAKE3 encoder (identity.md) ---- */

typedef struct {
  uint8_t *buf;
  size_t len;
  size_t cap;
  bool oom;
} CanonBuf;

static void cb_grow(CanonBuf *b, size_t add) {
  if (b->oom) return;
  if (add > SIZE_MAX - b->len) { b->oom = true; return; }
  size_t need = b->len + add;
  if (need <= b->cap) return;
  size_t ncap = b->cap == 0 ? 256U : b->cap;
  while (ncap < need) {
    if (ncap > SIZE_MAX / 2U) { ncap = need; break; }
    ncap *= 2U;
  }
  uint8_t *nb = realloc(b->buf, ncap);
  if (nb == NULL) { b->oom = true; return; }
  b->buf = nb;
  b->cap = ncap;
}

static void cb_u8(CanonBuf *b, uint8_t v) {
  cb_grow(b, 1);
  if (!b->oom) b->buf[b->len++] = v;
}

static void cb_bytes(CanonBuf *b, const void *p, size_t n) {
  if (n == 0) return;
  cb_grow(b, n);
  if (!b->oom) {
    memcpy(b->buf + b->len, p, n);
    b->len += n;
  }
}

static void cb_uleb(CanonBuf *b, uint64_t v) {
  do {
    uint8_t byte = (uint8_t)(v & 0x7FU);
    v >>= 7;
    if (v != 0) byte |= 0x80U;
    cb_u8(b, byte);
  } while (v != 0);
}

static void cb_str(CanonBuf *b, const char *s, size_t n) {
  cb_uleb(b, (uint64_t)n);
  cb_bytes(b, s, n);
}

static void cb_stigi_str(CanonBuf *b, StigiStr s) {
  cb_str(b, s.ptr != NULL ? s.ptr : "", s.len);
}

typedef struct {
  const StigiImage *img;
  uint32_t from_idx;
  const int32_t *scc_pos; /* -1 if not in current SCC, else member index (or -2 for placeholder) */
  bool placeholder_pass;
  bool runtime_only; /* S119: true = post-erasure runtime_id; false = pre-erasure verdict_key */
  StigiStr binders[256];
  uint32_t binder_count;
  StigiStr marker_binders[64];
  uint32_t marker_binder_count;
  CanonBuf *out;
} HashCtx;

static bool is_marker_cap(const StigiImage *img, uint32_t from_idx, const StigiType *cap_ty) {
  if (cap_ty == NULL) return true;
  const char *ref = cap_ty->name.ptr;
  size_t ref_len = cap_ty->name.len;
  /* Resolve unit-local alias first (Fix #2) */
  if (from_idx < img->unit_count) {
    const StigiUnit *u = img->units[from_idx].ast;
    for (uint32_t i = 0; i < u->alias_count; i++) {
      if (u->aliases[i].name.len == ref_len && memcmp(u->aliases[i].name.ptr, ref, ref_len) == 0) {
        ref = u->aliases[i].target.ptr;
        ref_len = u->aliases[i].target.len;
        break;
      }
    }
  }
  if ((ref_len == 9 && memcmp(ref, "Stigi.Mut", 9) == 0) ||
      (ref_len == 10 && memcmp(ref, "Stigi.Test", 10) == 0) ||
      (ref_len == 13 && memcmp(ref, "Stigi.NegTest", 13) == 0)) {
    return true;
  }
  int32_t tgt = stigi_image_resolve_name(img, from_idx, ref, ref_len);
  if (tgt >= 0) {
    const StigiDecl *d = img->units[tgt].ast->decl;
    if (d->kind == STIGI_DECL_CAPABILITY) {
      for (uint32_t i = 0; i < d->u.cap_decl.member_count; i++) {
        if (d->u.cap_decl.members[i]->kind == STIGI_DECL_DEF) return false;
      }
      return true; /* zero operations -> marker capability */
    }
  }
  return false; /* runtime capability */
}

static bool hc_is_marker_binder(const HashCtx *hc, StigiStr s) {
  for (uint32_t i = 0; i < hc->marker_binder_count; i++) {
    if (hc->marker_binders[i].len == s.len &&
        memcmp(hc->marker_binders[i].ptr, s.ptr, s.len) == 0) {
      return true;
    }
  }
  return false;
}

static bool is_lemma_call(const HashCtx *hc, const StigiExpr *e) {
  if (e == NULL || e->kind != STIGI_EXPR_CALL || e->u.call.callee == NULL) return false;
  if (e->u.call.callee->kind != STIGI_EXPR_LOCAL && e->u.call.callee->kind != STIGI_EXPR_GLOBAL) {
    return false;
  }
  StigiStr cn = e->u.call.callee->u.ident.name;
  int32_t tgt = stigi_image_resolve_name(hc->img, hc->from_idx, cn.ptr, cn.len);
  return tgt >= 0 && hc->img->units[tgt].ast->decl->kind == STIGI_DECL_LEMMA_DEF;
}

static bool is_erased_stmt(const HashCtx *hc, const StigiExpr *e) {
  if (e == NULL) return false;
  if (e->kind == STIGI_EXPR_ASSERT && e->u.assert_expr.expr == NULL) return true;
  if (is_lemma_call(hc, e)) return true;
  return false;
}

static bool hc_encode_enum_case(HashCtx *hc, StigiStr case_name) {
  int32_t eidx = find_enum_by_case(hc->img, hc->from_idx, case_name.ptr, case_name.len);
  if (eidx < 0) return false;
  const StigiDecl *ed = hc->img->units[eidx].ast->decl;
  uint32_t cidx = 0;
  for (uint32_t c = 0; c < ed->u.enum_decl.case_count; c++) {
    if (ed->u.enum_decl.cases[c].name.len == case_name.len &&
        memcmp(ed->u.enum_decl.cases[c].name.ptr, case_name.ptr, case_name.len) == 0) {
      cidx = c;
      break;
    }
  }
  cb_u8(hc->out, 0x08); /* EnumCase(enum_unit_id, case_idx) */
  if (hc->scc_pos != NULL && hc->scc_pos[eidx] != -1) {
    if (hc->placeholder_pass) cb_u8(hc->out, 0x04);
    else { cb_u8(hc->out, 0x05); cb_uleb(hc->out, (uint64_t)hc->scc_pos[eidx]); }
  } else {
    cb_u8(hc->out, 0x06);
    cb_bytes(hc->out, hc->img->units[eidx].unit_id, 32);
  }
  cb_uleb(hc->out, cidx);
  return true;
}

static int32_t hc_find_binder(const HashCtx *hc, StigiStr s) {
  for (uint32_t i = hc->binder_count; i > 0; i--) {
    if (hc->binders[i - 1].len == s.len &&
        memcmp(hc->binders[i - 1].ptr, s.ptr, s.len) == 0) {
      return (int32_t)(i - 1);
    }
  }
  return -1;
}

static void hc_push_binder(HashCtx *hc, StigiStr s) {
  if (hc->binder_count < 256) {
    hc->binders[hc->binder_count++] = s;
  }
}

static void hc_encode_ref(HashCtx *hc, StigiStr name) {
  int32_t lvl = hc_find_binder(hc, name);
  if (lvl >= 0) {
    cb_u8(hc->out, 0x01); /* Local(level) */
    cb_uleb(hc->out, (uint64_t)lvl);
    return;
  }
  if (name.len == 6 && memcmp(name.ptr, "result", 6) == 0) {
    cb_u8(hc->out, 0x02); /* Result */
    return;
  }
  if (name.len == 4 && memcmp(name.ptr, "self", 4) == 0) {
    cb_u8(hc->out, 0x03); /* Self */
    return;
  }
  int32_t tgt = stigi_image_resolve_name(hc->img, hc->from_idx, name.ptr, name.len);
  if (tgt >= 0) {
    if (hc->scc_pos != NULL && hc->scc_pos[tgt] != -1) {
      if (hc->placeholder_pass) {
        cb_u8(hc->out, 0x04); /* SelfGroup placeholder */
      } else {
        cb_u8(hc->out, 0x05); /* SelfGroup(member) */
        cb_uleb(hc->out, (uint64_t)hc->scc_pos[tgt]);
      }
      return;
    }
    cb_u8(hc->out, 0x06); /* External(32-byte BLAKE3 digest) */
    cb_bytes(hc->out,
             hc->runtime_only ? hc->img->units[tgt].unit_id : hc->img->units[tgt].verdict_key,
             32);
    return;
  }
  if (hc_encode_enum_case(hc, name)) return;
  cb_u8(hc->out, 0x07); /* Builtin / unresolved literal tag */
  cb_stigi_str(hc->out, name);
}

static void hc_encode_expr(HashCtx *hc, const StigiExpr *e);
static void hc_encode_type(HashCtx *hc, const StigiType *ty);
static void hc_encode_decl(HashCtx *hc, const StigiDecl *d);

static void hc_encode_fact_item(HashCtx *hc, const StigiFactItem *it) {
  cb_u8(hc->out, (uint8_t)it->kind);
  cb_u8(hc->out, it->is_negated ? 1 : 0);
  cb_u8(hc->out, it->evidence_name.len > 0 ? 1 : 0);
  if (it->kind == STIGI_FACT_ATOM) {
    hc_encode_ref(hc, it->atom_name);
    cb_uleb(hc->out, it->arg_count);
    for (uint32_t i = 0; i < it->arg_count; i++) {
      hc_encode_expr(hc, it->args[i]);
    }
  } else {
    cb_u8(hc->out, (uint8_t)it->op);
    if (it->kind == STIGI_FACT_ARITH) hc_encode_ref(hc, it->lhs_var);
    hc_encode_expr(hc, it->lhs);
    hc_encode_expr(hc, it->rhs);
  }
}

static void hc_encode_fact_group(HashCtx *hc, const StigiFactGroup *fg) {
  cb_uleb(hc->out, fg->count);
  for (uint32_t i = 0; i < fg->count; i++) {
    hc_encode_fact_item(hc, &fg->items[i]);
  }
}

static void hc_encode_type(HashCtx *hc, const StigiType *ty) {
  if (ty == NULL) { cb_u8(hc->out, 0x00); return; }
  cb_u8(hc->out, (uint8_t)(0x10 + ty->kind));
  hc_encode_ref(hc, ty->name);
  cb_uleb(hc->out, ty->type_arg_count);
  for (uint32_t i = 0; i < ty->type_arg_count; i++) {
    hc_encode_type(hc, ty->type_args[i]);
  }
  cb_uleb(hc->out, ty->len_arg_count);
  for (uint32_t i = 0; i < ty->len_arg_count; i++) {
    hc_encode_expr(hc, ty->len_args[i]);
  }
  if (ty->cap_arg.len > 0) {
    cb_u8(hc->out, 1);
    hc_encode_ref(hc, ty->cap_arg);
  } else {
    cb_u8(hc->out, 0);
  }
}

static void hc_encode_pat(HashCtx *hc, const StigiPat *pat) {
  cb_u8(hc->out, (uint8_t)(0x20 + pat->kind));
  if (pat->kind == STIGI_PAT_INT) {
    cb_uleb(hc->out, pat->int_val);
  } else if (pat->kind == STIGI_PAT_VAR) {
    hc_push_binder(hc, pat->name);
  } else if (pat->kind == STIGI_PAT_CTOR) {
    if (!hc_encode_enum_case(hc, pat->name)) {
      cb_stigi_str(hc->out, pat->name);
    }
    cb_uleb(hc->out, pat->field_count);
    for (uint32_t i = 0; i < pat->field_count; i++) {
      if (pat->fields[i].len == 1 && pat->fields[i].ptr[0] == '_') {
        cb_u8(hc->out, 0);
      } else {
        cb_u8(hc->out, 1);
        hc_push_binder(hc, pat->fields[i]);
      }
    }
  }
}

static void hc_encode_expr(HashCtx *hc, const StigiExpr *e) {
  if (e == NULL) { cb_u8(hc->out, 0x00); return; }
  /* S119: unwrap expr@<..> and erase lemma calls in runtime_only mode */
  if (hc->runtime_only && e->kind == STIGI_EXPR_ASSERT) {
    if (e->u.assert_expr.expr != NULL) {
      hc_encode_expr(hc, e->u.assert_expr.expr);
    }
    return;
  }
  if (hc->runtime_only && is_lemma_call(hc, e)) {
    return;
  }
  /* S119: in runtime_only mode, collapse single-effective-statement block */
  if (hc->runtime_only && e->kind == STIGI_EXPR_BLOCK) {
    uint32_t eff = 0;
    const StigiExpr *only = NULL;
    for (uint32_t i = 0; i < e->u.block.count; i++) {
      if (!is_erased_stmt(hc, e->u.block.stmts[i])) {
        eff++;
        only = e->u.block.stmts[i];
      }
    }
    if (eff == 1 && only->kind != STIGI_EXPR_VAL && only->kind != STIGI_EXPR_VAR) {
      hc_encode_expr(hc, only);
      return;
    }
    uint32_t saved = hc->binder_count;
    cb_u8(hc->out, (uint8_t)(0x30 + STIGI_EXPR_BLOCK));
    cb_uleb(hc->out, eff);
    for (uint32_t i = 0; i < e->u.block.count; i++) {
      if (!is_erased_stmt(hc, e->u.block.stmts[i])) {
        hc_encode_expr(hc, e->u.block.stmts[i]);
      }
    }
    hc->binder_count = saved;
    return;
  }
  cb_u8(hc->out, (uint8_t)(0x30 + e->kind));
  switch (e->kind) {
    case STIGI_EXPR_LOCAL:
    case STIGI_EXPR_GLOBAL:
      hc_encode_ref(hc, e->u.ident.name);
      break;
    case STIGI_EXPR_LIT_INT:
      cb_uleb(hc->out, e->u.lit_int.value);
      break;
    case STIGI_EXPR_LIT_CHAR:
      cb_u8(hc->out, e->u.lit_char.value);
      break;
    case STIGI_EXPR_LIT_STR:
      cb_stigi_str(hc->out, e->u.lit_str.raw);
      break;
    case STIGI_EXPR_LIT_BOOL:
      cb_u8(hc->out, e->u.lit_bool.value ? 1 : 0);
      break;
    case STIGI_EXPR_LIT_UNIT:
      break;
    case STIGI_EXPR_CALL:
      hc_encode_expr(hc, e->u.call.callee);
      cb_uleb(hc->out, e->u.call.type_arg_count);
      for (uint32_t i = 0; i < e->u.call.type_arg_count; i++) {
        hc_encode_type(hc, e->u.call.type_args[i]);
      }
      if (hc->runtime_only) {
        uint32_t rt_cargs = 0;
        for (uint32_t i = 0; i < e->u.call.cap_arg_count; i++) {
          if (!hc_is_marker_binder(hc, e->u.call.cap_args[i])) rt_cargs++;
        }
        cb_uleb(hc->out, rt_cargs);
        for (uint32_t i = 0; i < e->u.call.cap_arg_count; i++) {
          if (!hc_is_marker_binder(hc, e->u.call.cap_args[i])) {
            hc_encode_ref(hc, e->u.call.cap_args[i]);
          }
        }
      } else {
        cb_uleb(hc->out, e->u.call.cap_arg_count);
        for (uint32_t i = 0; i < e->u.call.cap_arg_count; i++) {
          hc_encode_ref(hc, e->u.call.cap_args[i]);
        }
      }
      cb_uleb(hc->out, e->u.call.arg_count);
      for (uint32_t i = 0; i < e->u.call.arg_count; i++) {
        hc_encode_expr(hc, e->u.call.args[i]);
      }
      break;
    case STIGI_EXPR_BINOP:
      cb_u8(hc->out, (uint8_t)e->u.binop.op);
      hc_encode_expr(hc, e->u.binop.lhs);
      hc_encode_expr(hc, e->u.binop.rhs);
      break;
    case STIGI_EXPR_UNOP:
      cb_u8(hc->out, (uint8_t)e->u.unop.op);
      hc_encode_expr(hc, e->u.unop.operand);
      break;
    case STIGI_EXPR_IF:
      hc_encode_expr(hc, e->u.if_expr.cond);
      hc_encode_expr(hc, e->u.if_expr.then_br);
      hc_encode_expr(hc, e->u.if_expr.else_br);
      break;
    case STIGI_EXPR_WHILE:
      hc_encode_expr(hc, e->u.while_expr.cond);
      if (!hc->runtime_only) {
        cb_u8(hc->out, e->u.while_expr.decreases != NULL ? 1 : 0);
        if (e->u.while_expr.decreases != NULL) hc_encode_expr(hc, e->u.while_expr.decreases);
      }
      hc_encode_expr(hc, e->u.while_expr.body);
      break;
    case STIGI_EXPR_MATCH:
      hc_encode_expr(hc, e->u.match_expr.scrutinee);
      cb_uleb(hc->out, e->u.match_expr.arm_count);
      for (uint32_t i = 0; i < e->u.match_expr.arm_count; i++) {
        uint32_t saved = hc->binder_count;
        hc_encode_pat(hc, &e->u.match_expr.arms[i].pat);
        hc_encode_expr(hc, e->u.match_expr.arms[i].body);
        hc->binder_count = saved;
      }
      break;
    case STIGI_EXPR_VAL:
      hc_encode_type(hc, e->u.val_stmt.type);
      hc_encode_expr(hc, e->u.val_stmt.init);
      hc_encode_pat(hc, &e->u.val_stmt.pat);
      break;
    case STIGI_EXPR_VAR:
      hc_encode_type(hc, e->u.var_stmt.type);
      hc_encode_expr(hc, e->u.var_stmt.init);
      hc_push_binder(hc, e->u.var_stmt.name);
      if (!hc->runtime_only) {
        cb_u8(hc->out, e->u.var_stmt.has_inv ? 1 : 0);
        if (e->u.var_stmt.has_inv) hc_encode_fact_group(hc, &e->u.var_stmt.inv);
      }
      break;
    case STIGI_EXPR_ASSIGN:
      hc_encode_ref(hc, e->u.assign_stmt.name);
      hc_encode_expr(hc, e->u.assign_stmt.value);
      break;
    case STIGI_EXPR_INDEX_READ:
    case STIGI_EXPR_INDEX_WRITE:
      hc_encode_expr(hc, e->u.index_expr.target);
      hc_encode_expr(hc, e->u.index_expr.index);
      hc_encode_expr(hc, e->u.index_expr.value);
      break;
    case STIGI_EXPR_HANDLE:
      hc_encode_ref(hc, e->u.handle_stmt.handler_name);
      cb_uleb(hc->out, e->u.handle_stmt.arg_count);
      for (uint32_t i = 0; i < e->u.handle_stmt.arg_count; i++) {
        hc_encode_expr(hc, e->u.handle_stmt.args[i]);
      }
      if (e->u.handle_stmt.binder.len > 0) hc_push_binder(hc, e->u.handle_stmt.binder);
      hc_encode_expr(hc, e->u.handle_stmt.body);
      break;
    case STIGI_EXPR_ASSERT:
      hc_encode_expr(hc, e->u.assert_expr.expr);
      hc_encode_fact_group(hc, &e->u.assert_expr.facts);
      break;
    case STIGI_EXPR_DENY:
      hc_encode_fact_item(hc, &e->u.deny_expr.item);
      break;
    case STIGI_EXPR_BLOCK: {
      uint32_t saved = hc->binder_count;
      cb_uleb(hc->out, e->u.block.count);
      for (uint32_t i = 0; i < e->u.block.count; i++) {
        hc_encode_expr(hc, e->u.block.stmts[i]);
      }
      hc->binder_count = saved;
      break;
    }
    case STIGI_EXPR_LOCAL_DECL:
      hc_push_binder(hc, e->u.local_decl.decl->name);
      hc_encode_decl(hc, e->u.local_decl.decl);
      break;
  }
}

static void hc_encode_def_head(HashCtx *hc, const StigiDefHead *h) {
  cb_uleb(hc->out, h->type_param_count);
  for (uint32_t i = 0; i < h->type_param_count; i++) {
    hc_push_binder(hc, h->type_params[i].name);
  }
  cb_uleb(hc->out, h->param_count);
  for (uint32_t i = 0; i < h->param_count; i++) {
    hc_push_binder(hc, h->params[i].name);
  }
  if (hc->runtime_only) {
    /* S119: include ONLY runtime capabilities (non-marker); exclude proof facts */
    uint32_t rt_caps = 0;
    for (uint32_t i = 0; i < h->pre_group_count; i++) {
      if (!h->pre_groups[i].is_fact) {
        for (uint32_t c = 0; c < h->pre_groups[i].cap_count; c++) {
          if (!is_marker_cap(hc->img, hc->from_idx, h->pre_groups[i].caps[c].cap_type)) {
            rt_caps++;
          }
        }
      }
    }
    cb_uleb(hc->out, rt_caps);
    for (uint32_t i = 0; i < h->pre_group_count; i++) {
      if (!h->pre_groups[i].is_fact) {
        for (uint32_t c = 0; c < h->pre_groups[i].cap_count; c++) {
          if (!is_marker_cap(hc->img, hc->from_idx, h->pre_groups[i].caps[c].cap_type)) {
            if (h->pre_groups[i].caps[c].name.len > 0) {
              hc_push_binder(hc, h->pre_groups[i].caps[c].name);
            }
            hc_encode_type(hc, h->pre_groups[i].caps[c].cap_type);
          } else if (h->pre_groups[i].caps[c].name.len > 0 && hc->marker_binder_count < 64) {
            hc->marker_binders[hc->marker_binder_count++] = h->pre_groups[i].caps[c].name;
          }
        }
      }
    }
    for (uint32_t i = 0; i < h->param_count; i++) {
      hc_encode_type(hc, h->params[i].type);
    }
    hc_encode_type(hc, h->ret_type);
  } else {
    cb_uleb(hc->out, h->pre_group_count);
    for (uint32_t i = 0; i < h->pre_group_count; i++) {
      cb_u8(hc->out, h->pre_groups[i].is_fact ? 1 : 0);
      if (h->pre_groups[i].is_fact) {
        hc_encode_fact_group(hc, &h->pre_groups[i].facts);
      } else {
        cb_uleb(hc->out, h->pre_groups[i].cap_count);
        for (uint32_t c = 0; c < h->pre_groups[i].cap_count; c++) {
          if (h->pre_groups[i].caps[c].name.len > 0) {
            hc_push_binder(hc, h->pre_groups[i].caps[c].name);
          }
          hc_encode_type(hc, h->pre_groups[i].caps[c].cap_type);
        }
      }
    }
    for (uint32_t i = 0; i < h->param_count; i++) {
      hc_encode_type(hc, h->params[i].type);
    }
    cb_uleb(hc->out, h->post_val_fact_count);
    for (uint32_t i = 0; i < h->post_val_fact_count; i++) {
      hc_encode_fact_group(hc, &h->post_val_facts[i]);
    }
    hc_encode_type(hc, h->ret_type);
    cb_uleb(hc->out, h->exit_fact_count);
    for (uint32_t i = 0; i < h->exit_fact_count; i++) {
      hc_encode_fact_group(hc, &h->exit_facts[i]);
    }
    cb_u8(hc->out, h->decreases != NULL ? 1 : 0);
    if (h->decreases != NULL) hc_encode_expr(hc, h->decreases);
  }
}

static void hc_encode_decl(HashCtx *hc, const StigiDecl *d) {
  if (d == NULL) return;
  uint32_t saved = hc->binder_count;
  cb_u8(hc->out, (uint8_t)(0x80 + d->kind));
  switch (d->kind) {
    case STIGI_DECL_DEF:
    case STIGI_DECL_LEMMA_DEF:
      /* Name erased! Only head + body with de Bruijn levels */
      hc_encode_def_head(hc, &d->u.def_decl.head);
      hc_encode_expr(hc, d->u.def_decl.body);
      break;
    case STIGI_DECL_VAL:
    case STIGI_DECL_TEST:
    case STIGI_DECL_NEG_TEST:
      if (hc->runtime_only) {
        uint32_t rt_caps = 0;
        for (uint32_t i = 0; i < d->u.val_decl.cap_count; i++) {
          if (!is_marker_cap(hc->img, hc->from_idx, d->u.val_decl.caps[i].cap_type)) rt_caps++;
        }
        cb_uleb(hc->out, rt_caps);
        for (uint32_t i = 0; i < d->u.val_decl.cap_count; i++) {
          if (!is_marker_cap(hc->img, hc->from_idx, d->u.val_decl.caps[i].cap_type)) {
            hc_encode_type(hc, d->u.val_decl.caps[i].cap_type);
          }
        }
      } else {
        cb_uleb(hc->out, d->u.val_decl.cap_count);
        for (uint32_t i = 0; i < d->u.val_decl.cap_count; i++) {
          hc_encode_type(hc, d->u.val_decl.caps[i].cap_type);
        }
      }
      hc_encode_type(hc, d->u.val_decl.type);
      hc_encode_expr(hc, d->u.val_decl.body);
      break;
    case STIGI_DECL_ENUM:
      /* Enums are nominal: hash qualified name, case names, field names (identity.md) */
      cb_stigi_str(hc->out, d->name);
      cb_uleb(hc->out, d->u.enum_decl.type_param_count);
      for (uint32_t i = 0; i < d->u.enum_decl.type_param_count; i++) {
        hc_push_binder(hc, d->u.enum_decl.type_params[i].name);
      }
      cb_uleb(hc->out, d->u.enum_decl.case_count);
      for (uint32_t c = 0; c < d->u.enum_decl.case_count; c++) {
        uint32_t csaved = hc->binder_count;
        const StigiEnumCase *ec = &d->u.enum_decl.cases[c];
        cb_stigi_str(hc->out, ec->name);
        cb_uleb(hc->out, ec->field_count);
        for (uint32_t f = 0; f < ec->field_count; f++) {
          cb_stigi_str(hc->out, ec->fields[f].name);
          hc_push_binder(hc, ec->fields[f].name);
          hc_encode_type(hc, ec->fields[f].type);
        }
        if (!hc->runtime_only) {
          cb_u8(hc->out, ec->has_facts ? 1 : 0);
          if (ec->has_facts) hc_encode_fact_group(hc, &ec->facts);
        }
        hc->binder_count = csaved;
      }
      break;
    case STIGI_DECL_TYPE:
      cb_uleb(hc->out, d->u.type_decl.type_param_count);
      for (uint32_t i = 0; i < d->u.type_decl.type_param_count; i++) {
        hc_push_binder(hc, d->u.type_decl.type_params[i].name);
      }
      cb_uleb(hc->out, d->u.type_decl.val_param_count);
      for (uint32_t i = 0; i < d->u.type_decl.val_param_count; i++) {
        hc_push_binder(hc, d->u.type_decl.val_params[i].name);
        hc_encode_type(hc, d->u.type_decl.val_params[i].type);
      }
      for (uint32_t i = 0; i < d->u.type_decl.cap_param_count; i++) {
        if (!hc->runtime_only ||
            !is_marker_cap(hc->img, hc->from_idx, d->u.type_decl.cap_params[i].cap_type)) {
          if (d->u.type_decl.cap_params[i].name.len > 0) {
            hc_push_binder(hc, d->u.type_decl.cap_params[i].name);
          }
          hc_encode_type(hc, d->u.type_decl.cap_params[i].cap_type);
        }
      }
      hc_encode_type(hc, d->u.type_decl.rhs);
      if (!hc->runtime_only) {
        cb_u8(hc->out, d->u.type_decl.has_facts ? 1 : 0);
        if (d->u.type_decl.has_facts) hc_encode_fact_group(hc, &d->u.type_decl.facts);
      }
      break;
    case STIGI_DECL_CAPABILITY:
      /* Capabilities are nominal (identity.md) */
      cb_stigi_str(hc->out, d->name);
      cb_uleb(hc->out, d->u.cap_decl.type_param_count);
      for (uint32_t i = 0; i < d->u.cap_decl.type_param_count; i++) {
        hc_push_binder(hc, d->u.cap_decl.type_params[i].name);
      }
      cb_uleb(hc->out, d->u.cap_decl.state_count);
      for (uint32_t i = 0; i < d->u.cap_decl.state_count; i++) {
        cb_stigi_str(hc->out, d->u.cap_decl.states[i]);
      }
      if (hc->runtime_only) {
        uint32_t op_cnt = 0;
        for (uint32_t i = 0; i < d->u.cap_decl.member_count; i++) {
          if (d->u.cap_decl.members[i]->kind != STIGI_DECL_RULE) op_cnt++;
        }
        cb_uleb(hc->out, op_cnt);
        for (uint32_t i = 0; i < d->u.cap_decl.member_count; i++) {
          if (d->u.cap_decl.members[i]->kind != STIGI_DECL_RULE) {
            cb_stigi_str(hc->out, d->u.cap_decl.members[i]->name);
            hc_encode_decl(hc, d->u.cap_decl.members[i]);
          }
        }
      } else {
        cb_uleb(hc->out, d->u.cap_decl.member_count);
        for (uint32_t i = 0; i < d->u.cap_decl.member_count; i++) {
          cb_stigi_str(hc->out, d->u.cap_decl.members[i]->name);
          hc_encode_decl(hc, d->u.cap_decl.members[i]);
        }
      }
      break;
    case STIGI_DECL_HANDLER:
      cb_uleb(hc->out, d->u.handler_decl.type_param_count);
      for (uint32_t i = 0; i < d->u.handler_decl.type_param_count; i++) {
        hc_push_binder(hc, d->u.handler_decl.type_params[i].name);
      }
      hc_encode_ref(hc, d->u.handler_decl.for_cap);
      cb_uleb(hc->out, d->u.handler_decl.val_param_count);
      for (uint32_t i = 0; i < d->u.handler_decl.val_param_count; i++) {
        hc_push_binder(hc, d->u.handler_decl.val_params[i].name);
        hc_encode_type(hc, d->u.handler_decl.val_params[i].type);
      }
      cb_uleb(hc->out, d->u.handler_decl.var_count);
      for (uint32_t i = 0; i < d->u.handler_decl.var_count; i++) {
        hc_encode_expr(hc, d->u.handler_decl.vars[i]);
      }
      cb_uleb(hc->out, d->u.handler_decl.def_count);
      for (uint32_t i = 0; i < d->u.handler_decl.def_count; i++) {
        hc_encode_decl(hc, d->u.handler_decl.defs[i]);
      }
      break;
    case STIGI_DECL_EXTERN:
      /* Extern hashes its C symbol (identity.md) */
      cb_stigi_str(hc->out, d->u.extern_decl.abi);
      cb_stigi_str(hc->out, d->name);
      cb_u8(hc->out, d->u.extern_decl.is_val ? 1 : 0);
      if (d->u.extern_decl.is_val) {
        hc_encode_type(hc, d->u.extern_decl.val_type);
      } else {
        hc_encode_def_head(hc, &d->u.extern_decl.head);
        hc_encode_expr(hc, d->u.extern_decl.body);
      }
      break;
    case STIGI_DECL_RELATION:
      /* Relations are nominal (identity.md) */
      cb_stigi_str(hc->out, d->name);
      cb_uleb(hc->out, d->u.relation_decl.column_count);
      for (uint32_t i = 0; i < d->u.relation_decl.column_count; i++) {
        cb_stigi_str(hc->out, d->u.relation_decl.columns[i].name);
        hc_encode_type(hc, d->u.relation_decl.columns[i].type);
      }
      break;
    case STIGI_DECL_RULE:
      hc_encode_fact_item(hc, &d->u.rule_decl.head);
      cb_uleb(hc->out, d->u.rule_decl.body_count);
      for (uint32_t i = 0; i < d->u.rule_decl.body_count; i++) {
        hc_encode_fact_item(hc, &d->u.rule_decl.body[i]);
      }
      break;
  }
  hc->binder_count = saved;
}

static Status encode_unit_canon(const StigiImage *img, uint32_t uidx,
                                const int32_t *scc_pos, bool placeholder_pass,
                                bool runtime_only, CanonBuf *out, Diag *diag) {
  memset(out, 0, sizeof *out);
  if (runtime_only) {
    cb_str(out, "stigi/unit/v1", 13);
  } else {
    cb_str(out, "stigi/verdict/v1", 16);
    cb_bytes(out, img->units[uidx].unit_id, 32);
  }
  HashCtx hc;
  memset(&hc, 0, sizeof hc);
  hc.img = img;
  hc.from_idx = uidx;
  hc.scc_pos = scc_pos;
  hc.placeholder_pass = placeholder_pass;
  hc.runtime_only = runtime_only;
  hc.out = out;
  hc_encode_decl(&hc, img->units[uidx].ast->decl);
  if (out->oom) {
    free(out->buf);
    out->buf = NULL;
    return diag_set(diag, STIGI_ERR_OOM, -1, "canonical encode out of memory");
  }
  return STIGI_OK;
}

/* Tarjan's SCC to hash units in dependency-first order (identity.md) */
typedef struct {
  StigiImage *img;
  StigiNameDiags *diags;
  uint32_t *index;
  uint32_t *lowlink;
  uint32_t *stack;
  bool *on_stack;
  uint32_t next_index;
  uint32_t sp;
  int32_t *scc_pos;
  Status status;
  Diag *diag;
} TarjanCtx;

typedef struct {
  uint32_t uidx;
  CanonBuf ph;
} SccMember;

static int cmp_scc_member(const void *pa, const void *pb) {
  const SccMember *a = pa;
  const SccMember *b = pb;
  size_t m = a->ph.len < b->ph.len ? a->ph.len : b->ph.len;
  int c = m > 0 ? memcmp(a->ph.buf, b->ph.buf, m) : 0;
  if (c != 0) return c;
  if (a->ph.len < b->ph.len) return -1;
  if (a->ph.len > b->ph.len) return 1;
  return 0;
}

static void tarjan_hash_scc(TarjanCtx *tc, uint32_t *members, uint32_t count) {
  if (tc->status != STIGI_OK || count == 0) return;

  bool has_self_loop = false;
  if (count == 1) {
    uint32_t u = members[0];
    if (tc->img->adj_head != NULL && u < tc->img->adj_cap) {
      for (uint32_t e = tc->img->adj_head[u]; e != UINT32_MAX; e = tc->img->edge_next[e]) {
        if (tc->img->edges[e].to == u) {
          has_self_loop = true;
          break;
        }
      }
    }
  }

  if (count == 1 && !has_self_loop) {
    CanonBuf cb;
    tc->status = encode_unit_canon(tc->img, members[0], NULL, false, true, &cb, tc->diag);
    if (tc->status == STIGI_OK) {
      blake3_hash(cb.buf, cb.len, tc->img->units[members[0]].unit_id);
      free(cb.buf);
    }
    if (tc->status == STIGI_OK) {
      CanonBuf vb;
      tc->status = encode_unit_canon(tc->img, members[0], NULL, false, false, &vb, tc->diag);
      if (tc->status == STIGI_OK) {
        blake3_hash(vb.buf, vb.len, tc->img->units[members[0]].verdict_key);
        free(vb.buf);
      }
    }
    return;
  }

  /* Recursive group: encode with SelfGroup placeholder, sort by bytes, hash under stigi/group/v1 */
  for (uint32_t i = 0; i < count; i++) tc->scc_pos[members[i]] = -2;
  SccMember *sm = calloc(count, sizeof *sm);
  if (sm == NULL) {
    tc->status = diag_set(tc->diag, STIGI_ERR_OOM, -1, "tarjan_hash_scc: OOM");
    for (uint32_t i = 0; i < count; i++) tc->scc_pos[members[i]] = -1;
    return;
  }
  for (uint32_t i = 0; i < count; i++) {
    sm[i].uidx = members[i];
    tc->status = encode_unit_canon(tc->img, members[i], tc->scc_pos, true, true, &sm[i].ph, tc->diag);
    if (tc->status != STIGI_OK) break;
  }
  if (tc->status == STIGI_OK) {
    qsort(sm, count, sizeof *sm, cmp_scc_member);
    /* Fix #3 (identity.md:50-52): equal placeholder encodings in one SCC -> ambiguous-group */
    for (uint32_t i = 0; i + 1 < count; i++) {
      if (cmp_scc_member(&sm[i], &sm[i + 1]) == 0) {
        tc->status = add_ndiag(tc->diags, STIGI_NAME_AMBIGUOUS_GROUP,
                               tc->img->units[sm[i].uidx].ast->decl->span, tc->diag,
                               "ambiguous recursive group: '%s' and '%s' have identical placeholder encoding",
                               tc->img->units[sm[i].uidx].name,
                               tc->img->units[sm[i + 1].uidx].name);
        break;
      }
    }
    for (uint32_t i = 0; i < count; i++) tc->scc_pos[sm[i].uidx] = (int32_t)i;

    CanonBuf grp;
    memset(&grp, 0, sizeof grp);
    cb_str(&grp, "stigi/group/v1", 14);
    cb_uleb(&grp, count);
    for (uint32_t i = 0; i < count; i++) {
      CanonBuf mb;
      tc->status = encode_unit_canon(tc->img, sm[i].uidx, tc->scc_pos, false, true, &mb, tc->diag);
      if (tc->status != STIGI_OK) break;
      cb_uleb(&grp, mb.len);
      cb_bytes(&grp, mb.buf, mb.len);
      free(mb.buf);
    }
    if (tc->status == STIGI_OK && !grp.oom) {
      uint8_t grp_digest[32];
      blake3_hash(grp.buf, grp.len, grp_digest);
      for (uint32_t i = 0; i < count; i++) {
        uint8_t mid_in[36];
        memcpy(mid_in, grp_digest, 32);
        mid_in[32] = (uint8_t)(i & 0xFFU);
        mid_in[33] = (uint8_t)((i >> 8) & 0xFFU);
        mid_in[34] = (uint8_t)((i >> 16) & 0xFFU);
        mid_in[35] = (uint8_t)((i >> 24) & 0xFFU);
        blake3_hash(mid_in, 36, tc->img->units[sm[i].uidx].unit_id);
      }
    } else if (tc->status == STIGI_OK && grp.oom) {
      tc->status = diag_set(tc->diag, STIGI_ERR_OOM, -1, "tarjan_hash_scc: OOM");
    }
    free(grp.buf);

    /* Compute verdict_key for SCC in topological order (S119) */
    if (tc->status == STIGI_OK) {
      CanonBuf vgrp;
      memset(&vgrp, 0, sizeof vgrp);
      cb_str(&vgrp, "stigi/vgrp/v1", 13);
      cb_uleb(&vgrp, count);
      for (uint32_t i = 0; i < count; i++) {
        CanonBuf vmb;
        tc->status = encode_unit_canon(tc->img, sm[i].uidx, tc->scc_pos, false, false, &vmb, tc->diag);
        if (tc->status != STIGI_OK) break;
        cb_uleb(&vgrp, vmb.len);
        cb_bytes(&vgrp, vmb.buf, vmb.len);
        free(vmb.buf);
      }
      if (tc->status == STIGI_OK && !vgrp.oom) {
        uint8_t vgrp_digest[32];
        blake3_hash(vgrp.buf, vgrp.len, vgrp_digest);
        for (uint32_t i = 0; i < count; i++) {
          uint8_t mid_in[36];
          memcpy(mid_in, vgrp_digest, 32);
          mid_in[32] = (uint8_t)(i & 0xFFU);
          mid_in[33] = (uint8_t)((i >> 8) & 0xFFU);
          mid_in[34] = (uint8_t)((i >> 16) & 0xFFU);
          mid_in[35] = (uint8_t)((i >> 24) & 0xFFU);
          blake3_hash(mid_in, 36, tc->img->units[sm[i].uidx].verdict_key);
        }
      } else if (tc->status == STIGI_OK && vgrp.oom) {
        tc->status = diag_set(tc->diag, STIGI_ERR_OOM, -1, "tarjan_hash_scc: OOM");
      }
      free(vgrp.buf);
    }
  }
  for (uint32_t i = 0; i < count; i++) {
    free(sm[i].ph.buf);
    tc->scc_pos[members[i]] = -1;
  }
  free(sm);
}

static void tarjan_dfs(TarjanCtx *tc, uint32_t v) {
  if (tc->status != STIGI_OK) return;
  tc->index[v] = tc->lowlink[v] = tc->next_index++;
  tc->stack[tc->sp++] = v;
  tc->on_stack[v] = true;

  if (tc->img->adj_head != NULL && v < tc->img->adj_cap) {
    for (uint32_t e = tc->img->adj_head[v]; e != UINT32_MAX; e = tc->img->edge_next[e]) {
      uint32_t w = tc->img->edges[e].to;
      if (tc->index[w] == UINT32_MAX) {
        tarjan_dfs(tc, w);
        if (tc->lowlink[w] < tc->lowlink[v]) tc->lowlink[v] = tc->lowlink[w];
      } else if (tc->on_stack[w]) {
        if (tc->index[w] < tc->lowlink[v]) tc->lowlink[v] = tc->index[w];
      }
    }
  }

  if (tc->lowlink[v] == tc->index[v]) {
    uint32_t *members = malloc((size_t)tc->img->unit_count * sizeof *members);
    if (members == NULL) {
      tc->status = diag_set(tc->diag, STIGI_ERR_OOM, -1, "tarjan_dfs: OOM");
      return;
    }
    uint32_t mcnt = 0;
    for (;;) {
      uint32_t w = tc->stack[--tc->sp];
      tc->on_stack[w] = false;
      members[mcnt++] = w;
      if (w == v) break;
    }
    tarjan_hash_scc(tc, members, mcnt);
    free(members);
  }
}

static Status compute_all_unit_ids(StigiImage *img, StigiNameDiags *diags, Diag *diag) {
  uint32_t n = img->unit_count;
  if (n == 0) return STIGI_OK;
  TarjanCtx tc;
  memset(&tc, 0, sizeof tc);
  tc.img = img;
  tc.diags = diags;
  tc.status = STIGI_OK;
  tc.diag = diag;
  tc.index = malloc((size_t)n * sizeof *tc.index);
  tc.lowlink = malloc((size_t)n * sizeof *tc.lowlink);
  tc.stack = malloc((size_t)n * sizeof *tc.stack);
  tc.on_stack = calloc(n, sizeof *tc.on_stack);
  tc.scc_pos = malloc((size_t)n * sizeof *tc.scc_pos);
  if (!tc.index || !tc.lowlink || !tc.stack || !tc.on_stack || !tc.scc_pos) {
    free(tc.index); free(tc.lowlink); free(tc.stack); free(tc.on_stack); free(tc.scc_pos);
    return diag_set(diag, STIGI_ERR_OOM, -1, "compute_all_unit_ids: OOM");
  }
  for (uint32_t i = 0; i < n; i++) {
    tc.index[i] = UINT32_MAX;
    tc.scc_pos[i] = -1;
  }
  for (uint32_t i = 0; i < n && tc.status == STIGI_OK; i++) {
    if (tc.index[i] == UINT32_MAX) tarjan_dfs(&tc, i);
  }
  free(tc.index);
  free(tc.lowlink);
  free(tc.stack);
  free(tc.on_stack);
  free(tc.scc_pos);
  return tc.status;
}

static Status add_ndiag(StigiNameDiags *d, StigiNameCode code, StigiSpan span,
                        Diag *diag, const char *fmt, ...) {
  if (d == NULL) return STIGI_OK;
  if (d->count == d->cap) {
    uint32_t ncap = d->cap == 0 ? 8U : d->cap * 2U;
    if (ncap <= d->cap) return diag_set(diag, STIGI_ERR_OVERFLOW, -1, "name diags overflow");
    StigiNameDiag *ni = realloc(d->items, (size_t)ncap * sizeof *ni);
    if (ni == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "name diags out of memory");
    d->items = ni;
    d->cap = ncap;
  }
  StigiNameDiag *item = &d->items[d->count++];
  item->code = code;
  item->span = span;
  va_list ap;
  va_start(ap, fmt);
  (void)vsnprintf(item->msg, sizeof item->msg, fmt, ap);
  va_end(ap);
  return STIGI_OK;
}

static uint64_t hash_bytes(const char *s, size_t len) {
  uint64_t h = 14695981039346656037ULL;
  for (size_t i = 0; i < len; i++) {
    h ^= (uint8_t)s[i];
    h *= 1099511628211ULL;
  }
  h ^= h >> 30;
  h *= 0xbf58476d1ce4e5b9ULL;
  h ^= h >> 27;
  h *= 0x94d049bb133111ebULL;
  h ^= h >> 31;
  return h;
}

static inline unsigned char ascii_lower(unsigned char c) {
  return (c >= 'A' && c <= 'Z') ? (unsigned char)(c - 'A' + 'a') : c;
}

static bool str_eq_ci_len(const char *a, size_t alen, const char *b, size_t blen) {
  if (alen != blen) return false;
  for (size_t i = 0; i < alen; i++) {
    if (ascii_lower((unsigned char)a[i]) != ascii_lower((unsigned char)b[i])) return false;
  }
  return true;
}

static uint64_t hash_bytes_ci(const char *s, size_t len) {
  uint64_t h = 14695981039346656037ULL;
  for (size_t i = 0; i < len; i++) {
    h ^= (uint8_t)ascii_lower((unsigned char)s[i]);
    h *= 1099511628211ULL;
  }
  h ^= h >> 30;
  h *= 0xbf58476d1ce4e5b9ULL;
  h ^= h >> 27;
  h *= 0x94d049bb133111ebULL;
  h ^= h >> 31;
  return h;
}

void stigi_image_init(StigiImage *img) {
  if (img == NULL) return;
  memset(img, 0, sizeof *img);
  arena_init(&img->arena);
  intern_init(&img->intern);
}

void stigi_image_free(StigiImage *img) {
  if (img == NULL) return;
  free(img->units);
  free(img->slots);
  free(img->ci_slots);
  free(img->edges);
  free(img->edge_next);
  free(img->adj_head);
  intern_free(&img->intern);
  arena_free(&img->arena);
  memset(img, 0, sizeof *img);
}

static Status image_rehash(StigiImage *img, uint32_t new_cap, Diag *diag) {
  uint32_t *ns = calloc(new_cap, sizeof *ns);
  uint32_t *nci = calloc(new_cap, sizeof *nci);
  if (ns == NULL || nci == NULL) {
    free(ns); free(nci);
    return diag_set(diag, STIGI_ERR_OOM, -1, "image_rehash: out of memory");
  }
  uint32_t mask = new_cap - 1U;
  for (uint32_t i = 0; i < img->unit_count; i++) {
    uint64_t h = hash_bytes(img->units[i].name, img->units[i].name_len);
    uint32_t idx = (uint32_t)(h & mask);
    while (ns[idx] != 0) idx = (idx + 1U) & mask;
    ns[idx] = i + 1U;

    uint64_t hc = hash_bytes_ci(img->units[i].name, img->units[i].name_len);
    uint32_t cidx = (uint32_t)(hc & mask);
    while (nci[cidx] != 0) cidx = (cidx + 1U) & mask;
    nci[cidx] = i + 1U;
  }
  free(img->slots);
  free(img->ci_slots);
  img->slots = ns;
  img->slot_cap = new_cap;
  img->ci_slots = nci;
  img->ci_slot_cap = new_cap;
  return STIGI_OK;
}

static int32_t image_find_ci(const StigiImage *img, const char *name, size_t len) {
  if (img == NULL || name == NULL || img->ci_slot_cap == 0) return -1;
  uint64_t h = hash_bytes_ci(name, len);
  uint32_t mask = img->ci_slot_cap - 1U;
  uint32_t idx = (uint32_t)(h & mask);
  while (img->ci_slots[idx] != 0) {
    uint32_t uidx = img->ci_slots[idx] - 1U;
    const StigiImageUnit *u = &img->units[uidx];
    if (str_eq_ci_len(u->name, u->name_len, name, len)) {
      return (int32_t)uidx;
    }
    idx = (idx + 1U) & mask;
  }
  return -1;
}

int32_t stigi_image_find(const StigiImage *img, const char *name, size_t len) {
  if (img == NULL || name == NULL || img->slot_cap == 0) return -1;
  uint64_t h = hash_bytes(name, len);
  uint32_t mask = img->slot_cap - 1U;
  uint32_t idx = (uint32_t)(h & mask);
  while (img->slots[idx] != 0) {
    uint32_t uidx = img->slots[idx] - 1U;
    const StigiImageUnit *u = &img->units[uidx];
    if (u->name_len == len && memcmp(u->name, name, len) == 0) {
      return (int32_t)uidx;
    }
    idx = (idx + 1U) & mask;
  }
  return -1;
}

Status stigi_image_add_source(StigiImage *img, const char *src, size_t len,
                              StigiNameDiags *diags, Diag *diag) {
  if (img == NULL || src == NULL) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_image_add_source: invalid arguments");
  }
  /* Copy source into img->arena so AST slices stay alive */
  char *copy = arena_alloc(&img->arena, len + 1U);
  if (copy == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "out of memory");
  if (len > 0) memcpy(copy, src, len);
  copy[len] = '\0';

  StigiParseResult pr = stigi_parse_unit(copy, len, &img->arena);
  if (pr.status != STIGI_OK || pr.unit == NULL || pr.unit->decl == NULL) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "parse failed: %s", pr.msg);
  }

  StigiDecl *decl = pr.unit->decl;
  uint32_t name_id = 0;
  Status st = intern_str(&img->intern, decl->name.ptr, decl->name.len, &name_id, diag);
  if (st != STIGI_OK) return st;
  size_t nlen = 0;
  const char *iname = intern_get(&img->intern, name_id, &nlen);

  /* Check duplicate & O(1) case-pair */
  if (stigi_image_find(img, iname, nlen) >= 0) {
    return add_ndiag(diags, STIGI_NAME_DUPLICATE, decl->span, diag,
                     "duplicate unit '%s'", iname);
  }
  int32_t ci_hit = image_find_ci(img, iname, nlen);
  if (ci_hit >= 0) {
    st = add_ndiag(diags, STIGI_NAME_CASE_PAIR, decl->span, diag,
                   "case-pair collision between '%s' and '%s'", img->units[ci_hit].name, iname);
    if (st != STIGI_OK) return st;
  }

  if (img->unit_count == img->unit_cap) {
    uint32_t ncap = img->unit_cap == 0 ? 16U : img->unit_cap * 2U;
    StigiImageUnit *nu = realloc(img->units, (size_t)ncap * sizeof *nu);
    if (nu == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "out of memory");
    img->units = nu;
    img->unit_cap = ncap;
  }

  if (img->slot_cap == 0) {
    st = image_rehash(img, 32U, diag);
    if (st != STIGI_OK) return st;
  } else if ((uint64_t)(img->unit_count + 1U) * 4ULL >= (uint64_t)img->slot_cap * 3ULL) {
    st = image_rehash(img, img->slot_cap * 2U, diag);
    if (st != STIGI_OK) return st;
  }

  uint32_t uidx = img->unit_count++;
  StigiImageUnit *u = &img->units[uidx];
  memset(u, 0, sizeof *u);
  u->idx = uidx;
  u->name_id = name_id;
  u->name = iname;
  u->name_len = nlen;
  u->ast = pr.unit;
  u->is_pub = decl->is_pub;
  u->is_root = (decl->kind == STIGI_DECL_TEST || decl->kind == STIGI_DECL_NEG_TEST ||
                (decl->is_pub && (decl->kind == STIGI_DECL_DEF || decl->kind == STIGI_DECL_VAL)) ||
                strcmp(iname, "main") == 0);

  /* Compute initial canonical BLAKE3-256 unit_id & verdict_key (recomputed in SCC order on resolve) */
  CanonBuf cb;
  st = encode_unit_canon(img, uidx, NULL, false, true, &cb, diag);
  if (st != STIGI_OK) return st;
  blake3_hash(cb.buf, cb.len, u->unit_id);
  free(cb.buf);
  st = encode_unit_canon(img, uidx, NULL, false, false, &cb, diag);
  if (st != STIGI_OK) return st;
  blake3_hash(cb.buf, cb.len, u->verdict_key);
  free(cb.buf);

  /* Insert into O(1) exact and case-folded hash tables */
  uint64_t h = hash_bytes(iname, nlen);
  uint32_t mask = img->slot_cap - 1U;
  uint32_t slot = (uint32_t)(h & mask);
  while (img->slots[slot] != 0) slot = (slot + 1U) & mask;
  img->slots[slot] = uidx + 1U;

  uint64_t hc = hash_bytes_ci(iname, nlen);
  uint32_t cslot = (uint32_t)(hc & mask);
  while (img->ci_slots[cslot] != 0) cslot = (cslot + 1U) & mask;
  img->ci_slots[cslot] = uidx + 1U;

  return STIGI_OK;
}

static size_t namespace_len(const char *name, size_t len) {
  size_t last_dot = 0;
  bool found = false;
  for (size_t i = 0; i < len; i++) {
    if (name[i] == '.') {
      last_dot = i;
      found = true;
    }
  }
  return found ? last_dot : 0;
}

bool stigi_image_visible_from(const StigiImage *img, uint32_t target_idx, uint32_t from_idx) {
  if (img == NULL || target_idx >= img->unit_count || from_idx >= img->unit_count) return false;
  const StigiImageUnit *tgt = &img->units[target_idx];
  if (tgt->is_pub || target_idx == from_idx) return true;

  size_t ns_len = namespace_len(tgt->name, tgt->name_len);
  if (ns_len == 0) return false;

  const StigiImageUnit *frm = &img->units[from_idx];
  return frm->name_len > ns_len && memcmp(frm->name, tgt->name, ns_len) == 0 &&
         frm->name[ns_len] == '.';
}

static bool same_namespace_or_sub(const StigiImageUnit *tgt, const StigiImageUnit *frm) {
  size_t ns_len = namespace_len(tgt->name, tgt->name_len);
  if (ns_len == 0) return tgt->idx == frm->idx;
  return frm->name_len > ns_len && memcmp(frm->name, tgt->name, ns_len) == 0 &&
         frm->name[ns_len] == '.';
}

int32_t stigi_image_resolve_name(const StigiImage *img, uint32_t from_idx,
                                 const char *ref, size_t ref_len) {
  if (img == NULL || from_idx >= img->unit_count || ref == NULL || ref_len == 0) return -1;
  const StigiImageUnit *u = &img->units[from_idx];

  /* 1. Check unit's aliases */
  for (uint32_t i = 0; i < u->ast->alias_count; i++) {
    StigiStr an = u->ast->aliases[i].name;
    if (an.len == ref_len && memcmp(an.ptr, ref, ref_len) == 0) {
      ref = u->ast->aliases[i].target.ptr;
      ref_len = u->ast->aliases[i].target.len;
      break;
    }
  }

  /* 2. Namepath search: A.B.c -> try A.B.ref, then A.ref, then ref */
  char cand[512];
  size_t ns_len = namespace_len(u->name, u->name_len);
  while (ns_len > 0) {
    if (ns_len + 1 + ref_len < sizeof(cand)) {
      memcpy(cand, u->name, ns_len);
      cand[ns_len] = '.';
      memcpy(cand + ns_len + 1, ref, ref_len);
      int32_t found = stigi_image_find(img, cand, ns_len + 1 + ref_len);
      if (found >= 0) return found;
    }
    ns_len = namespace_len(u->name, ns_len);
  }

  return stigi_image_find(img, ref, ref_len);
}

static int32_t find_enum_by_case(const StigiImage *img, uint32_t from_idx,
                                 const char *case_name, size_t len) {
  /* Prefer enums in same namespace, then any enum in image */
  int32_t fallback = -1;
  for (uint32_t i = 0; i < img->unit_count; i++) {
    const StigiDecl *d = img->units[i].ast->decl;
    if (d->kind != STIGI_DECL_ENUM) continue;
    for (uint32_t c = 0; c < d->u.enum_decl.case_count; c++) {
      StigiStr cn = d->u.enum_decl.cases[c].name;
      if (cn.len == len && memcmp(cn.ptr, case_name, len) == 0) {
        if (same_namespace_or_sub(&img->units[i], &img->units[from_idx])) {
          return (int32_t)i;
        }
        if (fallback < 0) fallback = (int32_t)i;
      }
    }
  }
  return fallback;
}

static bool is_builtin_name(const char *s, size_t len) {
  static const char *const k_builtins[] = {
    "U8", "U32", "U64", "I32", "I64", "Bool", "Unit", "Never", "Type",
    "Array", "Cell", "Ptr", "self", "result", "_",
    "Stigi.Test", "Stigi.Test.Result", "Stigi.Test.expect",
    "Stigi.NegTest", "Stigi.NegTest.expect",
    "Stigi.Mut", "Stigi.Region", "Stigi.Integer", "Stigi.Arena",
    "Array.tryGrow", "U64.ofU8", "U8.ofU64", "U64.bitwiseAnd", "U64.shiftLeft",
    "below", "lessOrEqual", "at", "valid", "overwrites",
    "Stigi.Facts.below", "Stigi.Facts.lessOrEqual", "Stigi.Facts.overwrites"
  };
  for (size_t i = 0; i < sizeof(k_builtins) / sizeof(k_builtins[0]); i++) {
    if (strlen(k_builtins[i]) == len && memcmp(k_builtins[i], s, len) == 0) return true;
  }
  if (len > 11 && memcmp(s, "Stigi.Code.", 11) == 0) return true;
  if (len > 9 && memcmp(s, "Stigi.Id.", 9) == 0) return true;
  return false;
}

static Status add_edge(StigiImage *img, uint32_t from, uint32_t to, Diag *diag) {
  if (img->adj_head != NULL && from < img->adj_cap) {
    for (uint32_t e = img->adj_head[from]; e != UINT32_MAX; e = img->edge_next[e]) {
      if (img->edges[e].to == to) return STIGI_OK;
    }
  }
  if (img->edge_count == img->edge_cap) {
    uint32_t ncap = img->edge_cap == 0 ? 16U : img->edge_cap * 2U;
    StigiDepEdge *ne = realloc(img->edges, (size_t)ncap * sizeof *ne);
    if (ne == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "out of memory");
    img->edges = ne;
    uint32_t *nn = realloc(img->edge_next, (size_t)ncap * sizeof *nn);
    if (nn == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "out of memory");
    img->edge_next = nn;
    img->edge_cap = ncap;
  }
  uint32_t eidx = img->edge_count++;
  img->edges[eidx].from = from;
  img->edges[eidx].to = to;
  if (img->adj_head != NULL && from < img->adj_cap) {
    img->edge_next[eidx] = img->adj_head[from];
    img->adj_head[from] = eidx;
  } else {
    img->edge_next[eidx] = UINT32_MAX;
  }
  return STIGI_OK;
}

typedef struct {
  StigiImage *img;
  uint32_t from_idx;
  StigiNameDiags *diags;
  Diag *diag;
  StigiStr locals[256];
  uint32_t local_count;
} ResCtx;

static bool is_local(const ResCtx *ctx, StigiStr s) {
  for (uint32_t i = 0; i < ctx->local_count; i++) {
    if (ctx->locals[i].len == s.len && memcmp(ctx->locals[i].ptr, s.ptr, s.len) == 0) {
      return true;
    }
  }
  return false;
}

static Status push_local(ResCtx *ctx, StigiStr s, StigiSpan sp, bool check_dup) {
  if (s.len == 0 || (s.len == 1 && s.ptr[0] == '_')) return STIGI_OK;
  if (check_dup && is_local(ctx, s)) {
    Status st = add_ndiag(ctx->diags, STIGI_NAME_DUPLICATE, sp, ctx->diag,
                          "duplicate binder '%.*s'", (int)s.len, s.ptr);
    if (st != STIGI_OK) return st;
  }
  if (ctx->local_count < 256) {
    ctx->locals[ctx->local_count++] = s;
  }
  return STIGI_OK;
}

static Status resolve_ref(ResCtx *ctx, StigiStr ref, StigiSpan sp, bool allow_case) {
  if (ref.len == 0 || is_local(ctx, ref)) return STIGI_OK;
  int32_t tgt = stigi_image_resolve_name(ctx->img, ctx->from_idx, ref.ptr, ref.len);
  if (tgt < 0 && allow_case) {
    tgt = find_enum_by_case(ctx->img, ctx->from_idx, ref.ptr, ref.len);
  }
  if (tgt < 0) {
    if (is_builtin_name(ref.ptr, ref.len)) return STIGI_OK;
    return add_ndiag(ctx->diags, STIGI_NAME_UNRESOLVED, sp, ctx->diag,
                     "unresolved name '%.*s' in '%s'", (int)ref.len, ref.ptr,
                     ctx->img->units[ctx->from_idx].name);
  }
  uint32_t uidx = (uint32_t)tgt;
  if (!stigi_image_visible_from(ctx->img, uidx, ctx->from_idx)) {
    Status st = add_ndiag(ctx->diags, STIGI_NAME_PRIVATE_REF, sp, ctx->diag,
                          "'%s' cannot access private unit '%s'",
                          ctx->img->units[ctx->from_idx].name, ctx->img->units[uidx].name);
    if (st != STIGI_OK) return st;
  }
  if (!same_namespace_or_sub(&ctx->img->units[uidx], &ctx->img->units[ctx->from_idx])) {
    ctx->img->units[uidx].used_outside_ns = true;
  }
  return add_edge(ctx->img, ctx->from_idx, uidx, ctx->diag);
}

static Status walk_expr(ResCtx *ctx, const StigiExpr *e);
static Status walk_type(ResCtx *ctx, const StigiType *ty);
static Status walk_decl(ResCtx *ctx, const StigiDecl *d);

static Status walk_fact_item(ResCtx *ctx, const StigiFactItem *it, bool is_rule) {
  if (it->kind == STIGI_FACT_ATOM) {
    Status st = resolve_ref(ctx, it->atom_name, it->span, false);
    if (st != STIGI_OK) return st;
    if (!is_rule) {
      for (uint32_t i = 0; i < it->arg_count; i++) {
        st = walk_expr(ctx, it->args[i]);
        if (st != STIGI_OK) return st;
      }
    }
  } else if (!is_rule) {
    if (it->lhs != NULL) {
      Status st = walk_expr(ctx, it->lhs);
      if (st != STIGI_OK) return st;
    }
    if (it->rhs != NULL) {
      Status st = walk_expr(ctx, it->rhs);
      if (st != STIGI_OK) return st;
    }
  }
  return STIGI_OK;
}

static Status walk_fact_group(ResCtx *ctx, const StigiFactGroup *fg) {
  for (uint32_t i = 0; i < fg->count; i++) {
    Status st = walk_fact_item(ctx, &fg->items[i], false);
    if (st != STIGI_OK) return st;
  }
  return STIGI_OK;
}

static Status walk_type(ResCtx *ctx, const StigiType *ty) {
  if (ty == NULL) return STIGI_OK;
  Status st = resolve_ref(ctx, ty->name, ty->span, false);
  if (st != STIGI_OK) return st;
  for (uint32_t i = 0; i < ty->type_arg_count; i++) {
    st = walk_type(ctx, ty->type_args[i]);
    if (st != STIGI_OK) return st;
  }
  for (uint32_t i = 0; i < ty->len_arg_count; i++) {
    st = walk_expr(ctx, ty->len_args[i]);
    if (st != STIGI_OK) return st;
  }
  return STIGI_OK;
}

static Status walk_expr(ResCtx *ctx, const StigiExpr *e) {
  if (e == NULL) return STIGI_OK;
  Status st = STIGI_OK;
  switch (e->kind) {
    case STIGI_EXPR_LOCAL:
    case STIGI_EXPR_GLOBAL:
      return resolve_ref(ctx, e->u.ident.name, e->span, true);
    case STIGI_EXPR_LIT_INT:
    case STIGI_EXPR_LIT_CHAR:
    case STIGI_EXPR_LIT_STR:
    case STIGI_EXPR_LIT_BOOL:
    case STIGI_EXPR_LIT_UNIT:
      return STIGI_OK;
    case STIGI_EXPR_CALL:
      st = walk_expr(ctx, e->u.call.callee);
      if (st != STIGI_OK) return st;
      for (uint32_t i = 0; i < e->u.call.type_arg_count; i++) {
        st = walk_type(ctx, e->u.call.type_args[i]);
        if (st != STIGI_OK) return st;
      }
      for (uint32_t i = 0; i < e->u.call.arg_count; i++) {
        st = walk_expr(ctx, e->u.call.args[i]);
        if (st != STIGI_OK) return st;
      }
      return STIGI_OK;
    case STIGI_EXPR_BINOP:
      st = walk_expr(ctx, e->u.binop.lhs);
      if (st != STIGI_OK) return st;
      return walk_expr(ctx, e->u.binop.rhs);
    case STIGI_EXPR_UNOP:
      return walk_expr(ctx, e->u.unop.operand);
    case STIGI_EXPR_IF:
      st = walk_expr(ctx, e->u.if_expr.cond);
      if (st != STIGI_OK) return st;
      st = walk_expr(ctx, e->u.if_expr.then_br);
      if (st != STIGI_OK) return st;
      return walk_expr(ctx, e->u.if_expr.else_br);
    case STIGI_EXPR_WHILE:
      st = walk_expr(ctx, e->u.while_expr.cond);
      if (st != STIGI_OK) return st;
      if (e->u.while_expr.decreases != NULL) {
        st = walk_expr(ctx, e->u.while_expr.decreases);
        if (st != STIGI_OK) return st;
      }
      return walk_expr(ctx, e->u.while_expr.body);
    case STIGI_EXPR_MATCH:
      st = walk_expr(ctx, e->u.match_expr.scrutinee);
      if (st != STIGI_OK) return st;
      for (uint32_t i = 0; i < e->u.match_expr.arm_count; i++) {
        uint32_t saved = ctx->local_count;
        const StigiPat *pat = &e->u.match_expr.arms[i].pat;
        if (pat->kind == STIGI_PAT_CTOR) {
          st = resolve_ref(ctx, pat->name, pat->span, true);
          if (st != STIGI_OK) return st;
          for (uint32_t f = 0; f < pat->field_count; f++) {
            st = push_local(ctx, pat->fields[f], pat->span, false);
            if (st != STIGI_OK) return st;
          }
        } else if (pat->kind == STIGI_PAT_VAR) {
          st = push_local(ctx, pat->name, pat->span, false);
          if (st != STIGI_OK) return st;
        }
        st = walk_expr(ctx, e->u.match_expr.arms[i].body);
        ctx->local_count = saved;
        if (st != STIGI_OK) return st;
      }
      return STIGI_OK;
    case STIGI_EXPR_VAL:
      if (e->u.val_stmt.type != NULL) {
        st = walk_type(ctx, e->u.val_stmt.type);
        if (st != STIGI_OK) return st;
      }
      st = walk_expr(ctx, e->u.val_stmt.init);
      if (st != STIGI_OK) return st;
      if (e->u.val_stmt.pat.kind == STIGI_PAT_VAR) {
        return push_local(ctx, e->u.val_stmt.pat.name, e->u.val_stmt.pat.span, true);
      } else if (e->u.val_stmt.pat.kind == STIGI_PAT_CTOR) {
        st = resolve_ref(ctx, e->u.val_stmt.pat.name, e->u.val_stmt.pat.span, true);
        if (st != STIGI_OK) return st;
        for (uint32_t f = 0; f < e->u.val_stmt.pat.field_count; f++) {
          st = push_local(ctx, e->u.val_stmt.pat.fields[f], e->u.val_stmt.pat.span, true);
          if (st != STIGI_OK) return st;
        }
      }
      return STIGI_OK;
    case STIGI_EXPR_VAR:
      if (e->u.var_stmt.type != NULL) {
        st = walk_type(ctx, e->u.var_stmt.type);
        if (st != STIGI_OK) return st;
      }
      st = walk_expr(ctx, e->u.var_stmt.init);
      if (st != STIGI_OK) return st;
      st = push_local(ctx, e->u.var_stmt.name, e->span, true);
      if (st != STIGI_OK) return st;
      if (e->u.var_stmt.has_inv) {
        return walk_fact_group(ctx, &e->u.var_stmt.inv);
      }
      return STIGI_OK;
    case STIGI_EXPR_ASSIGN:
      st = resolve_ref(ctx, e->u.assign_stmt.name, e->span, false);
      if (st != STIGI_OK) return st;
      return walk_expr(ctx, e->u.assign_stmt.value);
    case STIGI_EXPR_INDEX_READ:
    case STIGI_EXPR_INDEX_WRITE:
      st = walk_expr(ctx, e->u.index_expr.target);
      if (st != STIGI_OK) return st;
      st = walk_expr(ctx, e->u.index_expr.index);
      if (st != STIGI_OK) return st;
      return walk_expr(ctx, e->u.index_expr.value);
    case STIGI_EXPR_HANDLE:
      st = resolve_ref(ctx, e->u.handle_stmt.handler_name, e->span, false);
      if (st != STIGI_OK) return st;
      for (uint32_t i = 0; i < e->u.handle_stmt.arg_count; i++) {
        st = walk_expr(ctx, e->u.handle_stmt.args[i]);
        if (st != STIGI_OK) return st;
      }
      if (e->u.handle_stmt.binder.len > 0) {
        st = push_local(ctx, e->u.handle_stmt.binder, e->span, true);
        if (st != STIGI_OK) return st;
      }
      return walk_expr(ctx, e->u.handle_stmt.body);
    case STIGI_EXPR_ASSERT:
      if (e->u.assert_expr.expr != NULL) {
        st = walk_expr(ctx, e->u.assert_expr.expr);
        if (st != STIGI_OK) return st;
      }
      return walk_fact_group(ctx, &e->u.assert_expr.facts);
    case STIGI_EXPR_DENY:
      return walk_fact_item(ctx, &e->u.deny_expr.item, false);
    case STIGI_EXPR_BLOCK: {
      uint32_t saved = ctx->local_count;
      for (uint32_t i = 0; i < e->u.block.count; i++) {
        st = walk_expr(ctx, e->u.block.stmts[i]);
        if (st != STIGI_OK) { ctx->local_count = saved; return st; }
      }
      ctx->local_count = saved;
      return STIGI_OK;
    }
    case STIGI_EXPR_LOCAL_DECL:
      st = push_local(ctx, e->u.local_decl.decl->name, e->span, false);
      if (st != STIGI_OK) return st;
      return walk_decl(ctx, e->u.local_decl.decl);
  }
  return STIGI_OK;
}

static Status walk_def_head(ResCtx *ctx, const StigiDefHead *h) {
  Status st = STIGI_OK;
  for (uint32_t i = 0; i < h->type_param_count; i++) {
    st = push_local(ctx, h->type_params[i].name, h->type_params[i].span, true);
    if (st != STIGI_OK) return st;
  }
  for (uint32_t i = 0; i < h->param_count; i++) {
    st = push_local(ctx, h->params[i].name, h->params[i].span, true);
    if (st != STIGI_OK) return st;
  }
  for (uint32_t i = 0; i < h->pre_group_count; i++) {
    if (h->pre_groups[i].is_fact) {
      st = walk_fact_group(ctx, &h->pre_groups[i].facts);
    } else {
      for (uint32_t c = 0; c < h->pre_groups[i].cap_count; c++) {
        st = push_local(ctx, h->pre_groups[i].caps[c].name, h->pre_groups[i].caps[c].span, true);
        if (st != STIGI_OK) return st;
        st = walk_type(ctx, h->pre_groups[i].caps[c].cap_type);
        if (st != STIGI_OK) return st;
      }
    }
    if (st != STIGI_OK) return st;
  }
  for (uint32_t i = 0; i < h->param_count; i++) {
    st = walk_type(ctx, h->params[i].type);
    if (st != STIGI_OK) return st;
  }
  for (uint32_t i = 0; i < h->post_val_fact_count; i++) {
    st = walk_fact_group(ctx, &h->post_val_facts[i]);
    if (st != STIGI_OK) return st;
  }
  if (h->ret_type != NULL) {
    st = walk_type(ctx, h->ret_type);
    if (st != STIGI_OK) return st;
  }
  for (uint32_t i = 0; i < h->exit_fact_count; i++) {
    st = walk_fact_group(ctx, &h->exit_facts[i]);
    if (st != STIGI_OK) return st;
  }
  if (h->decreases != NULL) {
    st = walk_expr(ctx, h->decreases);
  }
  return st;
}

static Status walk_decl(ResCtx *ctx, const StigiDecl *d) {
  if (d == NULL) return STIGI_OK;
  uint32_t saved = ctx->local_count;
  Status st = STIGI_OK;
  switch (d->kind) {
    case STIGI_DECL_DEF:
    case STIGI_DECL_LEMMA_DEF:
      st = walk_def_head(ctx, &d->u.def_decl.head);
      if (st == STIGI_OK) st = walk_expr(ctx, d->u.def_decl.body);
      break;
    case STIGI_DECL_VAL:
    case STIGI_DECL_TEST:
    case STIGI_DECL_NEG_TEST:
      for (uint32_t i = 0; i < d->u.val_decl.cap_count; i++) {
        st = walk_type(ctx, d->u.val_decl.caps[i].cap_type);
        if (st != STIGI_OK) break;
      }
      if (st == STIGI_OK) st = walk_type(ctx, d->u.val_decl.type);
      if (st == STIGI_OK) st = walk_expr(ctx, d->u.val_decl.body);
      break;
    case STIGI_DECL_ENUM:
      for (uint32_t i = 0; i < d->u.enum_decl.type_param_count; i++) {
        st = push_local(ctx, d->u.enum_decl.type_params[i].name,
                        d->u.enum_decl.type_params[i].span, true);
        if (st != STIGI_OK) break;
      }
      for (uint32_t c = 0; st == STIGI_OK && c < d->u.enum_decl.case_count; c++) {
        uint32_t csaved = ctx->local_count;
        const StigiEnumCase *ec = &d->u.enum_decl.cases[c];
        for (uint32_t f = 0; f < ec->field_count; f++) {
          st = push_local(ctx, ec->fields[f].name, ec->fields[f].span, true);
          if (st != STIGI_OK) break;
          st = walk_type(ctx, ec->fields[f].type);
          if (st != STIGI_OK) break;
        }
        if (st == STIGI_OK && ec->has_facts) {
          st = walk_fact_group(ctx, &ec->facts);
        }
        ctx->local_count = csaved;
      }
      break;
    case STIGI_DECL_TYPE:
      for (uint32_t i = 0; i < d->u.type_decl.type_param_count; i++) {
        st = push_local(ctx, d->u.type_decl.type_params[i].name,
                        d->u.type_decl.type_params[i].span, true);
        if (st != STIGI_OK) break;
      }
      for (uint32_t i = 0; st == STIGI_OK && i < d->u.type_decl.val_param_count; i++) {
        st = push_local(ctx, d->u.type_decl.val_params[i].name,
                        d->u.type_decl.val_params[i].span, true);
        if (st == STIGI_OK) st = walk_type(ctx, d->u.type_decl.val_params[i].type);
      }
      if (st == STIGI_OK) st = walk_type(ctx, d->u.type_decl.rhs);
      if (st == STIGI_OK && d->u.type_decl.has_facts) {
        st = walk_fact_group(ctx, &d->u.type_decl.facts);
      }
      break;
    case STIGI_DECL_CAPABILITY:
      for (uint32_t i = 0; i < d->u.cap_decl.type_param_count; i++) {
        st = push_local(ctx, d->u.cap_decl.type_params[i].name,
                        d->u.cap_decl.type_params[i].span, true);
        if (st != STIGI_OK) break;
      }
      for (uint32_t i = 0; st == STIGI_OK && i < d->u.cap_decl.state_count; i++) {
        st = push_local(ctx, d->u.cap_decl.states[i], d->span, false);
      }
      for (uint32_t i = 0; st == STIGI_OK && i < d->u.cap_decl.member_count; i++) {
        st = walk_decl(ctx, d->u.cap_decl.members[i]);
      }
      break;
    case STIGI_DECL_HANDLER:
      for (uint32_t i = 0; i < d->u.handler_decl.type_param_count; i++) {
        st = push_local(ctx, d->u.handler_decl.type_params[i].name,
                        d->u.handler_decl.type_params[i].span, true);
        if (st != STIGI_OK) break;
      }
      for (uint32_t i = 0; st == STIGI_OK && i < d->u.handler_decl.val_param_count; i++) {
        st = push_local(ctx, d->u.handler_decl.val_params[i].name,
                        d->u.handler_decl.val_params[i].span, true);
        if (st == STIGI_OK) st = walk_type(ctx, d->u.handler_decl.val_params[i].type);
      }
      if (st == STIGI_OK) st = resolve_ref(ctx, d->u.handler_decl.for_cap, d->span, false);
      for (uint32_t i = 0; st == STIGI_OK && i < d->u.handler_decl.var_count; i++) {
        st = walk_expr(ctx, d->u.handler_decl.vars[i]);
      }
      for (uint32_t i = 0; st == STIGI_OK && i < d->u.handler_decl.def_count; i++) {
        st = walk_decl(ctx, d->u.handler_decl.defs[i]);
      }
      break;
    case STIGI_DECL_EXTERN:
      if (d->u.extern_decl.is_val) {
        st = walk_type(ctx, d->u.extern_decl.val_type);
      } else {
        st = walk_def_head(ctx, &d->u.extern_decl.head);
        if (st == STIGI_OK && d->u.extern_decl.body != NULL) {
          st = walk_expr(ctx, d->u.extern_decl.body);
        }
      }
      break;
    case STIGI_DECL_RELATION:
      for (uint32_t i = 0; i < d->u.relation_decl.column_count; i++) {
        st = walk_type(ctx, d->u.relation_decl.columns[i].type);
        if (st != STIGI_OK) break;
      }
      break;
    case STIGI_DECL_RULE:
      st = walk_fact_item(ctx, &d->u.rule_decl.head, true);
      for (uint32_t i = 0; st == STIGI_OK && i < d->u.rule_decl.body_count; i++) {
        st = walk_fact_item(ctx, &d->u.rule_decl.body[i], true);
      }
      break;
  }
  ctx->local_count = saved;
  return st;
}

Status stigi_image_resolve(StigiImage *img, StigiNameDiags *diags, Diag *diag) {
  if (img == NULL) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_image_resolve: invalid arguments");
  }

  /* Reset edges, adjacency heads, and flags */
  img->edge_count = 0;
  if (img->unit_count > img->adj_cap) {
    uint32_t *nh = realloc(img->adj_head, (size_t)img->unit_count * sizeof *nh);
    if (nh == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "out of memory");
    img->adj_head = nh;
    img->adj_cap = img->unit_count;
  }
  for (uint32_t i = 0; i < img->unit_count; i++) {
    img->adj_head[i] = UINT32_MAX;
    img->units[i].reachable = false;
    img->units[i].used_outside_ns = false;
  }

  for (uint32_t i = 0; i < img->unit_count; i++) {
    StigiImageUnit *u = &img->units[i];
    const StigiDecl *d = u->ast->decl;

    /* pub-on-test check */
    if (d->is_pub && (d->kind == STIGI_DECL_TEST || d->kind == STIGI_DECL_NEG_TEST)) {
      Status st = add_ndiag(diags, STIGI_NAME_PUB_ON_TEST, d->span, diag,
                            "Test/NegTest '%s' cannot be 'pub'", u->name);
      if (st != STIGI_OK) return st;
    }

    /* Duplicate alias check */
    for (uint32_t a = 0; a < u->ast->alias_count; a++) {
      for (uint32_t b = a + 1; b < u->ast->alias_count; b++) {
        if (u->ast->aliases[a].name.len == u->ast->aliases[b].name.len &&
            memcmp(u->ast->aliases[a].name.ptr, u->ast->aliases[b].name.ptr,
                   u->ast->aliases[a].name.len) == 0) {
          Status st = add_ndiag(diags, STIGI_NAME_DUPLICATE, u->ast->aliases[b].span, diag,
                                "duplicate alias '%.*s'",
                                (int)u->ast->aliases[b].name.len, u->ast->aliases[b].name.ptr);
          if (st != STIGI_OK) return st;
        }
      }
    }

    ResCtx ctx;
    memset(&ctx, 0, sizeof ctx);
    ctx.img = img;
    ctx.from_idx = i;
    ctx.diags = diags;
    ctx.diag = diag;
    Status st = walk_decl(&ctx, d);
    if (st != STIGI_OK) return st;

    /* Relation -> Rule edge (names.md 5) */
    if (d->kind == STIGI_DECL_RULE && d->u.rule_decl.head.kind == STIGI_FACT_ATOM) {
      int32_t rel = stigi_image_resolve_name(img, i, d->u.rule_decl.head.atom_name.ptr,
                                             d->u.rule_decl.head.atom_name.len);
      if (rel >= 0) {
        st = add_edge(img, (uint32_t)rel, i, diag);
        if (st != STIGI_OK) return st;
      }
    }
  }

  /* Reachability BFS from roots */
  uint32_t *queue = NULL;
  if (img->unit_count > 0) {
    queue = malloc((size_t)img->unit_count * sizeof *queue);
    if (queue == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "out of memory");
  }
  uint32_t qh = 0, qt = 0;
  for (uint32_t i = 0; i < img->unit_count; i++) {
    if (img->units[i].is_root) {
      img->units[i].reachable = true;
      queue[qt++] = i;
    }
  }
  while (qh < qt) {
    uint32_t cur = queue[qh++];
    for (uint32_t e = img->adj_head[cur]; e != UINT32_MAX; e = img->edge_next[e]) {
      uint32_t to = img->edges[e].to;
      if (!img->units[to].reachable) {
        img->units[to].reachable = true;
        queue[qt++] = to;
      }
    }
  }
  free(queue);

  /* no-reader and pub-unused checks */
  for (uint32_t i = 0; i < img->unit_count; i++) {
    const StigiImageUnit *u = &img->units[i];
    const StigiDecl *d = u->ast->decl;
    if (!u->reachable) {
      Status st = add_ndiag(diags, STIGI_NAME_NO_READER, d->span, diag,
                            "unit '%s' is not reachable from any root", u->name);
      if (st != STIGI_OK) return st;
    }
    if (u->is_pub && d->kind != STIGI_DECL_TEST && d->kind != STIGI_DECL_NEG_TEST &&
        strcmp(u->name, "main") != 0 && !u->used_outside_ns) {
      Status st = add_ndiag(diags, STIGI_NAME_PUB_UNUSED, d->span, diag,
                            "'pub' unit '%s' is not used outside its namespace", u->name);
      if (st != STIGI_OK) return st;
    }
  }

  /* Compute canonical de Bruijn + dependency-hash BLAKE3-256 unit IDs in SCC order */
  return compute_all_unit_ids(img, diags, diag);
}
