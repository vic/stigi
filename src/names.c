#include "names.h"
#include "blake3.h"
#include "parse.h"

#include <limits.h>
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

const char *stigi_decode_reason_name(StigiDecodeReason reason) {
  switch (reason) {
    case STIGI_DECODE_OK: return "Ok";
    case STIGI_DECODE_UNKNOWN_TAG: return "UnknownTag";
    case STIGI_DECODE_TRUNCATED: return "Truncated";
    case STIGI_DECODE_TRAILING_BYTES: return "TrailingBytes";
    case STIGI_DECODE_NON_MINIMAL_NUMBER: return "NonMinimalNumber";
    case STIGI_DECODE_NUMBER_TOO_LARGE: return "NumberTooLarge";
    case STIGI_DECODE_UNBOUND_LOCAL: return "UnboundLocal";
    case STIGI_DECODE_MEMBER_OUT_OF_RANGE: return "MemberOutOfRange";
    case STIGI_DECODE_NON_CANONICAL_GROUP_ORDER: return "NonCanonicalGroupOrder";
    case STIGI_DECODE_WRONG_SCHEMA: return "WrongSchema";
  }
  return "Unknown";
}

static Status add_ndiag(StigiNameDiags *d, StigiNameCode code, StigiSpan span,
                        Diag *diag, const char *fmt, ...);

/* ---- Exact closed Stigi.Code.* validation (obligations.md) ---- */

bool stigi_is_known_code_case(const char *s, size_t len) {
  static const char *const k_codes[] = {
    /* Step 1 / tenet gate */
    "Parse", "PromptMissing", "NotInLanguage", "NameCase",
    "ForbiddenWord", "CrypticName", "CasePair", "BarePointer", "ExpectForm",
    /* Z */
    "UnmodeledOperation",
    /* Names */
    "UnresolvedName", "PrivateReference", "Duplicate", "AmbiguousGroup",
    "ReservedName", "ExternDenied", "RuleOutsideKernel", "TopLevelName",
    "ExternOutsideHandler", "PubUnused", "PubOnTest", "NoReader",
    /* Types */
    "Mismatch", "CannotInfer", "UnknownType", "Arity", "TypeArity", "PatternArity",
    "LiteralOutOfRange", "LengthNotLinear", "RecursiveType",
    "TypeFuel", "EvaluationFuel", "NotCompileTime",
    "NotExhaustive", "WildcardArm", "NestedPattern",
    "AssignVal", "HandlerSignature", "UnusedParameter", "ExternForgesFacts",
    /* Facts & Lemma & Progress */
    "RelationArity", "PreconditionUnproved", "RefinementUnproved",
    "ExitUnproved", "AssertionUnproved", "InvariantUnproved", "DenialViolated",
    "LemmaNoBody", "LemmaEffect", "LemmaCallsPartial", "LemmaUnfounded",
    "ProgressUnproved", "FactsMissing", "FactIdle", "RuleIdle",
    "RuleUnsafe", "NegationCycle", "SafetyReadsUser",
    /* Bounds & Arithmetic & Pointers */
    "IndexUnproved", "OverflowUnproved", "DivisorUnproved", "ShiftUnproved",
    "WriteToStatic", "ExternSummaryMissing",
    /* Memory */
    "ValueUnused", "DeadInRegion", "SharedWrite", "DeadRetained",
    "RegionEscape", "AliasAcrossWrite", "MovedTwice", "OwnerDiffersByPath",
    "UseAfterFree", "DoubleFree", "Leak", "WriteUnread", "CapacityUnused",
    "FillSharesHandle", "HandlerStateEscape",
    /* Stack */
    "StackUnbounded", "StackOverBudget",
    /* Capabilities */
    "UnhandledEffect", "OpNotAllowed", "CapabilityNotEnded",
    "BranchStates", "LoopState", "UnknownState", "CapabilityValue",
    "HandlePlacement", "HandleOnMarker",
    /* Flow */
    "ExpressionUnused", "BinderUnused", "BranchDecided", "NeverReached",
    /* Tests */
    "TestFailed", "TestEvaluatorDiffers", "TestLeaksCapability",
    "FailurePathUntested", "NegtestNotRejected", "NegtestWrongCode",
    "NegtestForm", "NegtestWithoutRule",
    /* Engine */
    "DatalogArith", "DatalogBudget", "EngineOutOfMemory"
  };
  for (size_t i = 0; i < sizeof(k_codes) / sizeof(k_codes[0]); i++) {
    if (strlen(k_codes[i]) == len && memcmp(k_codes[i], s, len) == 0) return true;
  }
  return false;
}

static bool is_known_id_type(const char *s, size_t len) {
  static const char *const k_ids[] = {
    "Unit", "Def", "Type", "Enum", "Capability", "Handler", "Relation",
    "Rule", "Extern", "Value", "Site", "Point", "Scope", "Region",
    "Instance", "Field", "Case", "Operation", "Binder", "State", "Object",
    "Code", "Label", "Lemma"
  };
  for (size_t i = 0; i < sizeof(k_ids) / sizeof(k_ids[0]); i++) {
    if (strlen(k_ids[i]) == len && memcmp(k_ids[i], s, len) == 0) return true;
  }
  return false;
}

/* ---- Canonical de Bruijn + dependency-hash BLAKE3 encoder (identity.md, S119) ---- */

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

static uint64_t zigzag_encode(int64_t v) {
  uint64_t u = (uint64_t)v;
  return (u << 1) ^ (uint64_t)(v < 0 ? ~0ULL : 0ULL);
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
  const int32_t *scc_pos;
  bool placeholder_pass;
  bool runtime_only;
  StigiStr *binders;
  uint32_t binder_count;
  uint32_t binder_cap;
  StigiStr *marker_binders;
  uint32_t marker_binder_count;
  uint32_t marker_binder_cap;
  CanonBuf *out;
  int type_expand_depth;
} HashCtx;

static void hc_free(HashCtx *hc) {
  free(hc->binders);
  free(hc->marker_binders);
  hc->binders = NULL;
  hc->marker_binders = NULL;
  hc->binder_count = hc->binder_cap = 0;
  hc->marker_binder_count = hc->marker_binder_cap = 0;
}

static void hc_push_binder(HashCtx *hc, StigiStr s) {
  if (hc->out->oom) return;
  if (hc->binder_count == hc->binder_cap) {
    uint32_t ncap = hc->binder_cap == 0 ? 32U : hc->binder_cap * 2U;
    if (ncap <= hc->binder_cap) { hc->out->oom = true; return; }
    StigiStr *nb = realloc(hc->binders, (size_t)ncap * sizeof *nb);
    if (nb == NULL) { hc->out->oom = true; return; }
    hc->binders = nb;
    hc->binder_cap = ncap;
  }
  hc->binders[hc->binder_count++] = s;
}

static void hc_push_marker_binder(HashCtx *hc, StigiStr s) {
  if (hc->out->oom || s.len == 0) return;
  if (hc->marker_binder_count == hc->marker_binder_cap) {
    uint32_t ncap = hc->marker_binder_cap == 0 ? 16U : hc->marker_binder_cap * 2U;
    if (ncap <= hc->marker_binder_cap) { hc->out->oom = true; return; }
    StigiStr *nb = realloc(hc->marker_binders, (size_t)ncap * sizeof *nb);
    if (nb == NULL) { hc->out->oom = true; return; }
    hc->marker_binders = nb;
    hc->marker_binder_cap = ncap;
  }
  hc->marker_binders[hc->marker_binder_count++] = s;
}

static int32_t hc_find_binder(const HashCtx *hc, StigiStr s) {
  if (s.len == 0) return -1;
  for (uint32_t i = hc->binder_count; i > 0; i--) {
    if (hc->binders[i - 1].len == s.len &&
        memcmp(hc->binders[i - 1].ptr, s.ptr, s.len) == 0) {
      return (int32_t)(i - 1);
    }
  }
  return -1;
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

static bool is_marker_cap(const StigiImage *img, uint32_t from_idx, const StigiType *cap_ty) {
  if (cap_ty == NULL) return true;
  const char *ref = cap_ty->name.ptr;
  size_t ref_len = cap_ty->name.len;
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
  /* Local binder shadows any global lemma */
  if (hc_find_binder(hc, cn) >= 0) return false;
  int32_t tgt = stigi_image_resolve_name(hc->img, hc->from_idx, cn.ptr, cn.len);
  return tgt >= 0 && hc->img->units[tgt].ast->decl->kind == STIGI_DECL_LEMMA_DEF;
}

static bool is_erased_stmt(const HashCtx *hc, const StigiExpr *e) {
  if (e == NULL) return false;
  if (e->kind == STIGI_EXPR_ASSERT && e->u.assert_expr.expr == NULL) return true;
  if (e->kind == STIGI_EXPR_DENY) return true;
  if (is_lemma_call(hc, e)) return true;
  return false;
}

/* S119: check if expr is pure (side-effect free) for comptime-only local val erasure */
static bool expr_is_pure_for_erasure(const HashCtx *hc, const StigiExpr *e) {
  if (e == NULL) return true;
  switch (e->kind) {
    case STIGI_EXPR_LIT_INT:
    case STIGI_EXPR_LIT_CHAR:
    case STIGI_EXPR_LIT_STR:
    case STIGI_EXPR_LIT_BOOL:
    case STIGI_EXPR_LIT_UNIT:
    case STIGI_EXPR_LOCAL:
      return true;
    case STIGI_EXPR_GLOBAL: {
      int32_t tgt = stigi_image_resolve_name(hc->img, hc->from_idx,
                                             e->u.ident.name.ptr, e->u.ident.name.len);
      if (tgt >= 0) {
        return hc->img->units[tgt].ast->decl->kind == STIGI_DECL_VAL;
      }
      uint32_t cidx = 0;
      bool amb = false;
      return stigi_image_resolve_enum_case(hc->img, hc->from_idx, e->u.ident.name.ptr,
                                           e->u.ident.name.len, &cidx, &amb) >= 0;
    }
    case STIGI_EXPR_UNOP:
      return expr_is_pure_for_erasure(hc, e->u.unop.operand);
    case STIGI_EXPR_BINOP:
      return expr_is_pure_for_erasure(hc, e->u.binop.lhs) &&
             expr_is_pure_for_erasure(hc, e->u.binop.rhs);
    case STIGI_EXPR_IF:
      return expr_is_pure_for_erasure(hc, e->u.if_expr.cond) &&
             expr_is_pure_for_erasure(hc, e->u.if_expr.then_br) &&
             expr_is_pure_for_erasure(hc, e->u.if_expr.else_br);
    case STIGI_EXPR_ASSERT:
      return expr_is_pure_for_erasure(hc, e->u.assert_expr.expr);
    case STIGI_EXPR_DENY:
      return true;
    default:
      return false;
  }
}

/* Check if `name` is referenced at runtime (ignoring erased @<..> and lemma calls) */
static bool str_eq_ss(StigiStr a, StigiStr b) {
  return a.len == b.len && (a.len == 0 || memcmp(a.ptr, b.ptr, a.len) == 0);
}

static bool name_matches_ref_or_member(StigiStr target, StigiStr ref) {
  if (str_eq_ss(target, ref)) return true;
  if (ref.len > target.len && memcmp(ref.ptr, target.ptr, target.len) == 0 &&
      ref.ptr[target.len] == '.') {
    return true;
  }
  return false;
}

static bool type_uses_name_rt(const HashCtx *hc, const StigiType *ty, StigiStr name);
static bool expr_uses_name_rt(const HashCtx *hc, const StigiExpr *e, StigiStr name);

static bool type_uses_name_rt(const HashCtx *hc, const StigiType *ty, StigiStr name) {
  if (ty == NULL) return false;
  if (name_matches_ref_or_member(name, ty->name)) return true;
  if (ty->cap_arg.len > 0 && str_eq_ss(name, ty->cap_arg)) return true;
  for (uint32_t i = 0; i < ty->type_arg_count; i++) {
    if (type_uses_name_rt(hc, ty->type_args[i], name)) return true;
  }
  for (uint32_t i = 0; i < ty->len_arg_count; i++) {
    if (expr_uses_name_rt(hc, ty->len_args[i], name)) return true;
  }
  return false;
}

static bool pat_binds_name(const StigiPat *pat, StigiStr name) {
  if (pat->kind == STIGI_PAT_VAR) return str_eq_ss(pat->name, name);
  if (pat->kind == STIGI_PAT_CTOR) {
    for (uint32_t i = 0; i < pat->field_count; i++) {
      if (str_eq_ss(pat->fields[i], name)) return true;
    }
  }
  return false;
}

static bool expr_uses_name_rt(const HashCtx *hc, const StigiExpr *e, StigiStr name) {
  if (e == NULL) return false;
  if (is_erased_stmt(hc, e)) return false;
  switch (e->kind) {
    case STIGI_EXPR_LOCAL:
    case STIGI_EXPR_GLOBAL:
      return name_matches_ref_or_member(name, e->u.ident.name);
    case STIGI_EXPR_ASSERT:
      return expr_uses_name_rt(hc, e->u.assert_expr.expr, name);
    case STIGI_EXPR_CALL:
      if (expr_uses_name_rt(hc, e->u.call.callee, name)) return true;
      for (uint32_t i = 0; i < e->u.call.type_arg_count; i++) {
        if (type_uses_name_rt(hc, e->u.call.type_args[i], name)) return true;
      }
      for (uint32_t i = 0; i < e->u.call.cap_arg_count; i++) {
        if (!hc_is_marker_binder(hc, e->u.call.cap_args[i]) &&
            str_eq_ss(name, e->u.call.cap_args[i])) {
          return true;
        }
      }
      for (uint32_t i = 0; i < e->u.call.arg_count; i++) {
        if (expr_uses_name_rt(hc, e->u.call.args[i], name)) return true;
      }
      return false;
    case STIGI_EXPR_BINOP:
      return expr_uses_name_rt(hc, e->u.binop.lhs, name) ||
             expr_uses_name_rt(hc, e->u.binop.rhs, name);
    case STIGI_EXPR_UNOP:
      return expr_uses_name_rt(hc, e->u.unop.operand, name);
    case STIGI_EXPR_IF:
      return expr_uses_name_rt(hc, e->u.if_expr.cond, name) ||
             expr_uses_name_rt(hc, e->u.if_expr.then_br, name) ||
             expr_uses_name_rt(hc, e->u.if_expr.else_br, name);
    case STIGI_EXPR_WHILE:
      return expr_uses_name_rt(hc, e->u.while_expr.cond, name) ||
             expr_uses_name_rt(hc, e->u.while_expr.body, name);
    case STIGI_EXPR_MATCH:
      if (expr_uses_name_rt(hc, e->u.match_expr.scrutinee, name)) return true;
      for (uint32_t i = 0; i < e->u.match_expr.arm_count; i++) {
        if (!pat_binds_name(&e->u.match_expr.arms[i].pat, name) &&
            expr_uses_name_rt(hc, e->u.match_expr.arms[i].body, name)) {
          return true;
        }
      }
      return false;
    case STIGI_EXPR_VAL:
      return type_uses_name_rt(hc, e->u.val_stmt.type, name) ||
             expr_uses_name_rt(hc, e->u.val_stmt.init, name);
    case STIGI_EXPR_VAR:
      return type_uses_name_rt(hc, e->u.var_stmt.type, name) ||
             expr_uses_name_rt(hc, e->u.var_stmt.init, name);
    case STIGI_EXPR_ASSIGN:
      return str_eq_ss(name, e->u.assign_stmt.name) ||
             expr_uses_name_rt(hc, e->u.assign_stmt.value, name);
    case STIGI_EXPR_INDEX_READ:
    case STIGI_EXPR_INDEX_WRITE:
      return expr_uses_name_rt(hc, e->u.index_expr.target, name) ||
             expr_uses_name_rt(hc, e->u.index_expr.index, name) ||
             expr_uses_name_rt(hc, e->u.index_expr.value, name);
    case STIGI_EXPR_HANDLE:
      for (uint32_t i = 0; i < e->u.handle_stmt.type_arg_count; i++) {
        if (type_uses_name_rt(hc, e->u.handle_stmt.type_args[i], name)) return true;
      }
      for (uint32_t i = 0; i < e->u.handle_stmt.cap_arg_count; i++) {
        if (str_eq_ss(name, e->u.handle_stmt.cap_args[i])) return true;
      }
      for (uint32_t i = 0; i < e->u.handle_stmt.arg_count; i++) {
        if (expr_uses_name_rt(hc, e->u.handle_stmt.args[i], name)) return true;
      }
      if (e->u.handle_stmt.body != NULL && !str_eq_ss(e->u.handle_stmt.binder, name)) {
        return expr_uses_name_rt(hc, e->u.handle_stmt.body, name);
      }
      return false;
    case STIGI_EXPR_BLOCK:
      for (uint32_t i = 0; i < e->u.block.count; i++) {
        const StigiExpr *s = e->u.block.stmts[i];
        if (expr_uses_name_rt(hc, s, name)) return true;
        if (s->kind == STIGI_EXPR_VAL && pat_binds_name(&s->u.val_stmt.pat, name)) break;
        if (s->kind == STIGI_EXPR_VAR && str_eq_ss(s->u.var_stmt.name, name)) break;
        if (s->kind == STIGI_EXPR_HANDLE && s->u.handle_stmt.body == NULL &&
            str_eq_ss(s->u.handle_stmt.binder, name)) {
          break;
        }
      }
      return false;
    default:
      return false;
  }
}

/* Canonical linear form over de Bruijn levels for length expressions (n + 1 == 1 + n) */
typedef struct {
  int64_t c;
  struct {
    uint32_t lvl;
    int64_t coeff;
  } terms[16];
  int nterms;
} CanonLinear;

static bool ci64_add(int64_t a, int64_t b, int64_t *out) {
  if ((b > 0 && a > INT64_MAX - b) || (b < 0 && a < INT64_MIN - b)) return false;
  *out = a + b;
  return true;
}

static bool ci64_mul(int64_t a, int64_t b, int64_t *out) {
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

static bool cl_combine(const CanonLinear *a, const CanonLinear *b, int64_t sgn, CanonLinear *out) {
  CanonLinear r;
  memset(&r, 0, sizeof r);
  int64_t bc = 0;
  if (!ci64_mul(sgn, b->c, &bc) || !ci64_add(a->c, bc, &r.c)) return false;
  int i = 0, j = 0;
  while (i < a->nterms || j < b->nterms) {
    uint32_t lvl = 0;
    int64_t coef = 0;
    if (j >= b->nterms || (i < a->nterms && a->terms[i].lvl < b->terms[j].lvl)) {
      lvl = a->terms[i].lvl;
      coef = a->terms[i].coeff;
      i++;
    } else if (i >= a->nterms || b->terms[j].lvl < a->terms[i].lvl) {
      lvl = b->terms[j].lvl;
      if (!ci64_mul(sgn, b->terms[j].coeff, &coef)) return false;
      j++;
    } else {
      lvl = a->terms[i].lvl;
      int64_t sb = 0;
      if (!ci64_mul(sgn, b->terms[j].coeff, &sb) || !ci64_add(a->terms[i].coeff, sb, &coef)) {
        return false;
      }
      i++; j++;
    }
    if (coef != 0) {
      if (r.nterms >= 16) return false;
      r.terms[r.nterms].lvl = lvl;
      r.terms[r.nterms].coeff = coef;
      r.nterms++;
    }
  }
  *out = r;
  return true;
}

static bool cl_from_expr(const HashCtx *hc, const StigiExpr *e, CanonLinear *out) {
  if (e == NULL) return false;
  memset(out, 0, sizeof *out);
  switch (e->kind) {
    case STIGI_EXPR_LIT_INT:
      if (e->u.lit_int.overflow || e->u.lit_int.value > (uint64_t)INT64_MAX) return false;
      out->c = (int64_t)e->u.lit_int.value;
      return true;
    case STIGI_EXPR_LOCAL:
    case STIGI_EXPR_GLOBAL: {
      int32_t lvl = hc_find_binder(hc, e->u.ident.name);
      if (lvl >= 0) {
        out->terms[0].lvl = (uint32_t)lvl;
        out->terms[0].coeff = 1;
        out->nterms = 1;
        return true;
      }
      return false;
    }
    case STIGI_EXPR_UNOP:
      if (e->u.unop.op == STIGI_OP_NEG) {
        CanonLinear in;
        if (!cl_from_expr(hc, e->u.unop.operand, &in)) return false;
        CanonLinear z;
        memset(&z, 0, sizeof z);
        return cl_combine(&z, &in, -1, out);
      }
      return false;
    case STIGI_EXPR_BINOP: {
      CanonLinear l, r;
      if (!cl_from_expr(hc, e->u.binop.lhs, &l) || !cl_from_expr(hc, e->u.binop.rhs, &r)) {
        return false;
      }
      if (e->u.binop.op == STIGI_OP_ADD) return cl_combine(&l, &r, 1, out);
      if (e->u.binop.op == STIGI_OP_SUB) return cl_combine(&l, &r, -1, out);
      if (e->u.binop.op == STIGI_OP_MUL) {
        if (l.nterms > 0 && r.nterms > 0) return false;
        const CanonLinear *lin = l.nterms > 0 ? &l : &r;
        int64_t k = l.nterms > 0 ? r.c : l.c;
        if (!ci64_mul(lin->c, k, &out->c)) return false;
        if (k != 0) {
          for (int i = 0; i < lin->nterms; i++) {
            int64_t nc = 0;
            if (!ci64_mul(lin->terms[i].coeff, k, &nc)) return false;
            out->terms[out->nterms].lvl = lin->terms[i].lvl;
            out->terms[out->nterms].coeff = nc;
            out->nterms++;
          }
        }
        return true;
      }
      return false;
    }
    default:
      return false;
  }
}

static bool hc_encode_enum_case(HashCtx *hc, StigiStr case_name) {
  uint32_t cidx = 0;
  bool amb = false;
  int32_t eidx = stigi_image_resolve_enum_case(hc->img, hc->from_idx, case_name.ptr,
                                               case_name.len, &cidx, &amb);
  if (eidx < 0) return false;
  cb_u8(hc->out, 0x08); /* EnumCase(enum_ref, case_idx) */
  if (hc->scc_pos != NULL && hc->scc_pos[eidx] != -1) {
    if (hc->placeholder_pass) cb_u8(hc->out, 0x04);
    else { cb_u8(hc->out, 0x05); cb_uleb(hc->out, (uint64_t)hc->scc_pos[eidx]); }
  } else {
    cb_u8(hc->out, 0x06);
    cb_bytes(hc->out,
             hc->runtime_only ? hc->img->units[eidx].unit_id : hc->img->units[eidx].verdict_key,
             32);
  }
  cb_uleb(hc->out, cidx);
  return true;
}

static void hc_encode_ref(HashCtx *hc, StigiStr name) {
  int32_t lvl = hc_find_binder(hc, name);
  if (lvl >= 0) {
    cb_u8(hc->out, 0x01); /* Local(level) */
    cb_uleb(hc->out, (uint64_t)lvl);
    return;
  }
  /* Capability operation call `binder.op`: encode as LocalMember(lvl, op) so binder rename keeps ID */
  if (name.ptr != NULL && name.len > 0) {
    const char *dot = memchr(name.ptr, '.', name.len);
    if (dot != NULL) {
      StigiStr prefix;
      prefix.ptr = name.ptr;
      prefix.len = (size_t)(dot - name.ptr);
      int32_t plvl = hc_find_binder(hc, prefix);
      if (plvl >= 0) {
        StigiStr member;
        member.ptr = dot + 1;
        member.len = name.len - prefix.len - 1U;
        cb_u8(hc->out, 0x09); /* LocalMember(level, member_name) */
        cb_uleb(hc->out, (uint64_t)plvl);
        cb_stigi_str(hc->out, member);
        return;
      }
    }
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
  cb_u8(hc->out, 0x07); /* Builtin / state / unresolved literal tag */
  cb_stigi_str(hc->out, name);
}

static void hc_encode_expr(HashCtx *hc, const StigiExpr *e);
static void hc_encode_type(HashCtx *hc, const StigiType *ty);
static void hc_encode_decl(HashCtx *hc, const StigiDecl *d);

static void hc_encode_fact_item(HashCtx *hc, const StigiFactItem *it) {
  cb_u8(hc->out, (uint8_t)it->kind);
  cb_u8(hc->out, it->is_negated ? 1 : 0);
  cb_u8(hc->out, it->evidence_name.len > 0 ? 1 : 0);
  if (it->evidence_name.len > 0) {
    hc_push_binder(hc, it->evidence_name);
  }
  if (it->kind == STIGI_FACT_ATOM) {
    hc_encode_ref(hc, it->atom_name);
    cb_uleb(hc->out, it->arg_count);
    for (uint32_t i = 0; i < it->arg_count; i++) {
      hc_encode_expr(hc, it->args[i]);
    }
  } else if (it->kind == STIGI_FACT_DENY) {
    cb_uleb(hc->out, it->deny_atom_count);
    for (uint32_t i = 0; i < it->deny_atom_count; i++) {
      hc_encode_fact_item(hc, &it->deny_atoms[i]);
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
  /* S119: expand transparent (fact-free, zero-param) type definitions in runtime_only mode */
  if (hc->runtime_only && ty->kind == STIGI_TYPE_NAMED &&
      ty->type_arg_count == 0 && ty->len_arg_count == 0 && ty->cap_arg.len == 0 &&
      hc->type_expand_depth < 16 && hc_find_binder(hc, ty->name) < 0) {
    int32_t tgt = stigi_image_resolve_name(hc->img, hc->from_idx, ty->name.ptr, ty->name.len);
    if (tgt >= 0) {
      const StigiDecl *td = hc->img->units[tgt].ast->decl;
      if (td->kind == STIGI_DECL_TYPE && !td->u.type_decl.has_facts &&
          td->u.type_decl.type_param_count == 0 && td->u.type_decl.val_param_count == 0 &&
          td->u.type_decl.cap_param_count == 0 && td->u.type_decl.rhs != NULL) {
        uint32_t saved_from = hc->from_idx;
        hc->from_idx = (uint32_t)tgt;
        hc->type_expand_depth++;
        hc_encode_type(hc, td->u.type_decl.rhs);
        hc->type_expand_depth--;
        hc->from_idx = saved_from;
        return;
      }
    }
  }
  cb_u8(hc->out, (uint8_t)(0x10 + ty->kind));
  hc_encode_ref(hc, ty->name);
  cb_uleb(hc->out, ty->type_arg_count);
  for (uint32_t i = 0; i < ty->type_arg_count; i++) {
    hc_encode_type(hc, ty->type_args[i]);
  }
  cb_uleb(hc->out, ty->len_arg_count);
  for (uint32_t i = 0; i < ty->len_arg_count; i++) {
    CanonLinear cl;
    if (cl_from_expr(hc, ty->len_args[i], &cl)) {
      cb_u8(hc->out, 0x01); /* Canonical linear form */
      cb_uleb(hc->out, zigzag_encode(cl.c));
      cb_uleb(hc->out, (uint64_t)cl.nterms);
      for (int k = 0; k < cl.nterms; k++) {
        cb_uleb(hc->out, cl.terms[k].lvl);
        cb_uleb(hc->out, zigzag_encode(cl.terms[k].coeff));
      }
    } else {
      cb_u8(hc->out, 0x00); /* General expression fallback */
      hc_encode_expr(hc, ty->len_args[i]);
    }
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
  } else if (pat->kind == STIGI_PAT_BOOL) {
    cb_u8(hc->out, pat->bool_val ? 1 : 0);
  } else if (pat->kind == STIGI_PAT_VAR) {
    hc_push_binder(hc, pat->name);
  } else if (pat->kind == STIGI_PAT_CTOR) {
    if (!hc_encode_enum_case(hc, pat->name)) {
      cb_u8(hc->out, 0x07);
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
  /* S119: unwrap expr@<..> and erase bare @<..>, deny, and lemma calls in runtime_only mode */
  if (hc->runtime_only && e->kind == STIGI_EXPR_ASSERT) {
    if (e->u.assert_expr.expr != NULL) {
      hc_encode_expr(hc, e->u.assert_expr.expr);
    } else {
      cb_u8(hc->out, (uint8_t)(0x30 + STIGI_EXPR_LIT_UNIT));
    }
    return;
  }
  if (hc->runtime_only && (e->kind == STIGI_EXPR_DENY || is_lemma_call(hc, e))) {
    cb_u8(hc->out, (uint8_t)(0x30 + STIGI_EXPR_LIT_UNIT));
    return;
  }
  /* S119: in runtime_only mode, erase dead comptime-only pure local vals & collapse wrapper block */
  if (hc->runtime_only && e->kind == STIGI_EXPR_BLOCK) {
    uint32_t n = e->u.block.count;
    bool *keep = n > 0 ? calloc(n, sizeof *keep) : NULL;
    if (n > 0 && keep == NULL) { hc->out->oom = true; return; }
    for (uint32_t i = 0; i < n; i++) {
      keep[i] = !is_erased_stmt(hc, e->u.block.stmts[i]);
    }
    /* Iteratively erase pure local `val x = expr` statements unused by any later surviving stmt */
    bool changed = true;
    while (changed) {
      changed = false;
      for (uint32_t i = n; i > 0; i--) {
        uint32_t idx = i - 1U;
        if (!keep[idx]) continue;
        const StigiExpr *s = e->u.block.stmts[idx];
        if (s->kind != STIGI_EXPR_VAL || s->u.val_stmt.pat.kind != STIGI_PAT_VAR) continue;
        if (!expr_is_pure_for_erasure(hc, s->u.val_stmt.init)) continue;
        /* Never erase the final statement if it's the only surviving statement returning Unit?
         * Actually a val statement has type Unit, so if idx + 1 == n and nothing follows it,
         * erasing it leaves Unit as block value — still safe if another statement precedes it or eff==0 -> Unit! */
        StigiStr vname = s->u.val_stmt.pat.name;
        bool used = false;
        for (uint32_t j = idx + 1U; j < n; j++) {
          const StigiExpr *sj = e->u.block.stmts[j];
          if (keep[j] && expr_uses_name_rt(hc, sj, vname)) {
            used = true;
            break;
          }
          if (keep[j]) {
            if (sj->kind == STIGI_EXPR_VAL && pat_binds_name(&sj->u.val_stmt.pat, vname)) break;
            if (sj->kind == STIGI_EXPR_VAR && str_eq_ss(sj->u.var_stmt.name, vname)) break;
            if (sj->kind == STIGI_EXPR_HANDLE && sj->u.handle_stmt.body == NULL &&
                str_eq_ss(sj->u.handle_stmt.binder, vname)) {
              break;
            }
          }
        }
        if (!used) {
          keep[idx] = false;
          changed = true;
        }
      }
    }
    uint32_t eff = 0;
    const StigiExpr *only = NULL;
    for (uint32_t i = 0; i < n; i++) {
      if (keep[i]) { eff++; only = e->u.block.stmts[i]; }
    }
    if (eff == 0) {
      free(keep);
      cb_u8(hc->out, (uint8_t)(0x30 + STIGI_EXPR_LIT_UNIT));
      return;
    }
    if (eff == 1 && only->kind != STIGI_EXPR_VAL && only->kind != STIGI_EXPR_VAR &&
        only->kind != STIGI_EXPR_HANDLE && only->kind != STIGI_EXPR_LOCAL_DECL) {
      free(keep);
      hc_encode_expr(hc, only);
      return;
    }
    uint32_t saved_b = hc->binder_count;
    uint32_t saved_m = hc->marker_binder_count;
    cb_u8(hc->out, (uint8_t)(0x30 + STIGI_EXPR_BLOCK));
    cb_uleb(hc->out, eff);
    for (uint32_t i = 0; i < n; i++) {
      if (keep[i]) hc_encode_expr(hc, e->u.block.stmts[i]);
    }
    hc->binder_count = saved_b;
    hc->marker_binder_count = saved_m;
    free(keep);
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
        uint32_t saved_b = hc->binder_count;
        uint32_t saved_m = hc->marker_binder_count;
        hc_encode_pat(hc, &e->u.match_expr.arms[i].pat);
        hc_encode_expr(hc, e->u.match_expr.arms[i].body);
        hc->binder_count = saved_b;
        hc->marker_binder_count = saved_m;
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
    case STIGI_EXPR_HANDLE: {
      hc_encode_ref(hc, e->u.handle_stmt.handler_name);
      cb_uleb(hc->out, e->u.handle_stmt.type_arg_count);
      for (uint32_t i = 0; i < e->u.handle_stmt.type_arg_count; i++) {
        hc_encode_type(hc, e->u.handle_stmt.type_args[i]);
      }
      if (hc->runtime_only) {
        uint32_t rt_cargs = 0;
        for (uint32_t i = 0; i < e->u.handle_stmt.cap_arg_count; i++) {
          if (!hc_is_marker_binder(hc, e->u.handle_stmt.cap_args[i])) rt_cargs++;
        }
        cb_uleb(hc->out, rt_cargs);
        for (uint32_t i = 0; i < e->u.handle_stmt.cap_arg_count; i++) {
          if (!hc_is_marker_binder(hc, e->u.handle_stmt.cap_args[i])) {
            hc_encode_ref(hc, e->u.handle_stmt.cap_args[i]);
          }
        }
      } else {
        cb_uleb(hc->out, e->u.handle_stmt.cap_arg_count);
        for (uint32_t i = 0; i < e->u.handle_stmt.cap_arg_count; i++) {
          hc_encode_ref(hc, e->u.handle_stmt.cap_args[i]);
        }
      }
      cb_uleb(hc->out, e->u.handle_stmt.arg_count);
      for (uint32_t i = 0; i < e->u.handle_stmt.arg_count; i++) {
        hc_encode_expr(hc, e->u.handle_stmt.args[i]);
      }
      bool has_b = e->u.handle_stmt.binder.len > 0;
      bool has_body = e->u.handle_stmt.body != NULL;
      cb_u8(hc->out, has_b ? 1 : 0);
      cb_u8(hc->out, has_body ? 1 : 0);
      if (has_body) {
        uint32_t saved_b = hc->binder_count;
        uint32_t saved_m = hc->marker_binder_count;
        if (has_b) hc_push_binder(hc, e->u.handle_stmt.binder);
        hc_encode_expr(hc, e->u.handle_stmt.body);
        hc->binder_count = saved_b;
        hc->marker_binder_count = saved_m;
      } else {
        if (has_b) hc_push_binder(hc, e->u.handle_stmt.binder);
      }
      break;
    }
    case STIGI_EXPR_ASSERT:
      hc_encode_expr(hc, e->u.assert_expr.expr);
      hc_encode_fact_group(hc, &e->u.assert_expr.facts);
      break;
    case STIGI_EXPR_DENY:
      hc_encode_fact_item(hc, &e->u.deny_expr.item);
      break;
    case STIGI_EXPR_BLOCK: {
      uint32_t saved_b = hc->binder_count;
      uint32_t saved_m = hc->marker_binder_count;
      cb_uleb(hc->out, e->u.block.count);
      for (uint32_t i = 0; i < e->u.block.count; i++) {
        hc_encode_expr(hc, e->u.block.stmts[i]);
      }
      hc->binder_count = saved_b;
      hc->marker_binder_count = saved_m;
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
            bool has_b = h->pre_groups[i].caps[c].name.len > 0;
            cb_u8(hc->out, has_b ? 1 : 0);
            if (has_b) hc_push_binder(hc, h->pre_groups[i].caps[c].name);
            hc_encode_type(hc, h->pre_groups[i].caps[c].cap_type);
          } else if (h->pre_groups[i].caps[c].name.len > 0) {
            hc_push_marker_binder(hc, h->pre_groups[i].caps[c].name);
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
          bool has_b = h->pre_groups[i].caps[c].name.len > 0;
          cb_u8(hc->out, has_b ? 1 : 0);
          if (has_b) hc_push_binder(hc, h->pre_groups[i].caps[c].name);
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

/* Collect free rule variables in left-to-right occurrence order for alpha-invariant rule hashing */
static void collect_rule_var_expr(HashCtx *hc, const StigiExpr *e, uint32_t *pushed) {
  if (e == NULL) return;
  if (e->kind == STIGI_EXPR_LOCAL) {
    StigiStr nm = e->u.ident.name;
    if (nm.len > 0 && !(nm.len == 1 && nm.ptr[0] == '_') &&
        !(nm.len == 6 && memcmp(nm.ptr, "result", 6) == 0) &&
        !(nm.len == 4 && memcmp(nm.ptr, "self", 4) == 0) &&
        hc_find_binder(hc, nm) < 0 &&
        stigi_image_resolve_name(hc->img, hc->from_idx, nm.ptr, nm.len) < 0) {
      hc_push_binder(hc, nm);
      (*pushed)++;
    }
  } else if (e->kind == STIGI_EXPR_BINOP) {
    collect_rule_var_expr(hc, e->u.binop.lhs, pushed);
    collect_rule_var_expr(hc, e->u.binop.rhs, pushed);
  } else if (e->kind == STIGI_EXPR_UNOP) {
    collect_rule_var_expr(hc, e->u.unop.operand, pushed);
  }
}

static void collect_rule_var_item(HashCtx *hc, const StigiFactItem *it, uint32_t *pushed) {
  if (it->kind == STIGI_FACT_ATOM) {
    for (uint32_t i = 0; i < it->arg_count; i++) {
      collect_rule_var_expr(hc, it->args[i], pushed);
    }
  } else if (it->kind == STIGI_FACT_DENY) {
    for (uint32_t i = 0; i < it->deny_atom_count; i++) {
      collect_rule_var_item(hc, &it->deny_atoms[i], pushed);
    }
  } else {
    if (it->kind == STIGI_FACT_ARITH && it->lhs_var.len > 0 &&
        hc_find_binder(hc, it->lhs_var) < 0) {
      hc_push_binder(hc, it->lhs_var);
      (*pushed)++;
    }
    collect_rule_var_expr(hc, it->lhs, pushed);
    collect_rule_var_expr(hc, it->rhs, pushed);
  }
}

static void hc_encode_decl(HashCtx *hc, const StigiDecl *d) {
  if (d == NULL) return;
  uint32_t saved_b = hc->binder_count;
  uint32_t saved_m = hc->marker_binder_count;
  cb_u8(hc->out, (uint8_t)(0x80 + d->kind));
  switch (d->kind) {
    case STIGI_DECL_DEF:
    case STIGI_DECL_LEMMA_DEF:
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
            bool has_b = d->u.val_decl.caps[i].name.len > 0;
            cb_u8(hc->out, has_b ? 1 : 0);
            if (has_b) hc_push_binder(hc, d->u.val_decl.caps[i].name);
            hc_encode_type(hc, d->u.val_decl.caps[i].cap_type);
          } else if (d->u.val_decl.caps[i].name.len > 0) {
            hc_push_marker_binder(hc, d->u.val_decl.caps[i].name);
          }
        }
      } else {
        cb_uleb(hc->out, d->u.val_decl.cap_count);
        for (uint32_t i = 0; i < d->u.val_decl.cap_count; i++) {
          bool has_b = d->u.val_decl.caps[i].name.len > 0;
          cb_u8(hc->out, has_b ? 1 : 0);
          if (has_b) hc_push_binder(hc, d->u.val_decl.caps[i].name);
          hc_encode_type(hc, d->u.val_decl.caps[i].cap_type);
        }
      }
      hc_encode_type(hc, d->u.val_decl.type);
      hc_encode_expr(hc, d->u.val_decl.body);
      break;
    case STIGI_DECL_ENUM:
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
    case STIGI_DECL_TYPE: {
      cb_uleb(hc->out, d->u.type_decl.type_param_count);
      for (uint32_t i = 0; i < d->u.type_decl.type_param_count; i++) {
        hc_push_binder(hc, d->u.type_decl.type_params[i].name);
      }
      cb_uleb(hc->out, d->u.type_decl.val_param_count);
      for (uint32_t i = 0; i < d->u.type_decl.val_param_count; i++) {
        hc_push_binder(hc, d->u.type_decl.val_params[i].name);
        hc_encode_type(hc, d->u.type_decl.val_params[i].type);
      }
      uint32_t ccnt = 0;
      for (uint32_t i = 0; i < d->u.type_decl.cap_param_count; i++) {
        if (!hc->runtime_only ||
            !is_marker_cap(hc->img, hc->from_idx, d->u.type_decl.cap_params[i].cap_type)) {
          ccnt++;
        }
      }
      cb_uleb(hc->out, ccnt);
      for (uint32_t i = 0; i < d->u.type_decl.cap_param_count; i++) {
        if (!hc->runtime_only ||
            !is_marker_cap(hc->img, hc->from_idx, d->u.type_decl.cap_params[i].cap_type)) {
          bool has_b = d->u.type_decl.cap_params[i].name.len > 0;
          cb_u8(hc->out, has_b ? 1 : 0);
          if (has_b) hc_push_binder(hc, d->u.type_decl.cap_params[i].name);
          hc_encode_type(hc, d->u.type_decl.cap_params[i].cap_type);
        } else if (d->u.type_decl.cap_params[i].name.len > 0) {
          hc_push_marker_binder(hc, d->u.type_decl.cap_params[i].name);
        }
      }
      hc_encode_type(hc, d->u.type_decl.rhs);
      if (!hc->runtime_only) {
        cb_u8(hc->out, d->u.type_decl.has_facts ? 1 : 0);
        if (d->u.type_decl.has_facts) hc_encode_fact_group(hc, &d->u.type_decl.facts);
      }
      break;
    }
    case STIGI_DECL_CAPABILITY:
      cb_stigi_str(hc->out, d->name);
      cb_uleb(hc->out, d->u.cap_decl.type_param_count);
      for (uint32_t i = 0; i < d->u.cap_decl.type_param_count; i++) {
        hc_push_binder(hc, d->u.cap_decl.type_params[i].name);
      }
      cb_uleb(hc->out, d->u.cap_decl.state_count);
      for (uint32_t i = 0; i < d->u.cap_decl.state_count; i++) {
        cb_stigi_str(hc->out, d->u.cap_decl.states[i]);
      }
      cb_u8(hc->out, d->u.cap_decl.start_state.len > 0 ? 1 : 0);
      if (d->u.cap_decl.start_state.len > 0) {
        cb_stigi_str(hc->out, d->u.cap_decl.start_state);
      }
      cb_uleb(hc->out, d->u.cap_decl.stop_state_count);
      for (uint32_t i = 0; i < d->u.cap_decl.stop_state_count; i++) {
        cb_stigi_str(hc->out, d->u.cap_decl.stop_states[i]);
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
    case STIGI_DECL_HANDLER: {
      cb_uleb(hc->out, d->u.handler_decl.type_param_count);
      for (uint32_t i = 0; i < d->u.handler_decl.type_param_count; i++) {
        hc_push_binder(hc, d->u.handler_decl.type_params[i].name);
      }
      uint32_t ccnt = 0;
      for (uint32_t i = 0; i < d->u.handler_decl.cap_param_count; i++) {
        if (!hc->runtime_only ||
            !is_marker_cap(hc->img, hc->from_idx, d->u.handler_decl.cap_params[i].cap_type)) {
          ccnt++;
        }
      }
      cb_uleb(hc->out, ccnt);
      for (uint32_t i = 0; i < d->u.handler_decl.cap_param_count; i++) {
        if (!hc->runtime_only ||
            !is_marker_cap(hc->img, hc->from_idx, d->u.handler_decl.cap_params[i].cap_type)) {
          bool has_b = d->u.handler_decl.cap_params[i].name.len > 0;
          cb_u8(hc->out, has_b ? 1 : 0);
          if (has_b) hc_push_binder(hc, d->u.handler_decl.cap_params[i].name);
          hc_encode_type(hc, d->u.handler_decl.cap_params[i].cap_type);
        } else if (d->u.handler_decl.cap_params[i].name.len > 0) {
          hc_push_marker_binder(hc, d->u.handler_decl.cap_params[i].name);
        }
      }
      if (d->u.handler_decl.for_cap_type != NULL) {
        hc_encode_type(hc, d->u.handler_decl.for_cap_type);
      } else {
        StigiType tmp_ty;
        memset(&tmp_ty, 0, sizeof tmp_ty);
        tmp_ty.kind = STIGI_TYPE_NAMED;
        tmp_ty.name = d->u.handler_decl.for_cap;
        hc_encode_type(hc, &tmp_ty);
      }
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
        cb_stigi_str(hc->out, d->u.handler_decl.defs[i]->name);
        hc_encode_decl(hc, d->u.handler_decl.defs[i]);
      }
      break;
    }
    case STIGI_DECL_EXTERN:
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
      cb_stigi_str(hc->out, d->name);
      cb_uleb(hc->out, d->u.relation_decl.column_count);
      for (uint32_t i = 0; i < d->u.relation_decl.column_count; i++) {
        cb_stigi_str(hc->out, d->u.relation_decl.columns[i].name);
        hc_encode_type(hc, d->u.relation_decl.columns[i].type);
      }
      break;
    case STIGI_DECL_RULE: {
      uint32_t pushed = 0;
      collect_rule_var_item(hc, &d->u.rule_decl.head, &pushed);
      for (uint32_t i = 0; i < d->u.rule_decl.body_count; i++) {
        collect_rule_var_item(hc, &d->u.rule_decl.body[i], &pushed);
      }
      cb_uleb(hc->out, pushed);
      hc_encode_fact_item(hc, &d->u.rule_decl.head);
      cb_uleb(hc->out, d->u.rule_decl.body_count);
      for (uint32_t i = 0; i < d->u.rule_decl.body_count; i++) {
        hc_encode_fact_item(hc, &d->u.rule_decl.body[i]);
      }
      break;
    }
  }
  hc->binder_count = saved_b;
  hc->marker_binder_count = saved_m;
}

/* ---- Strict canonical decoder (identity.md:42-55) ---- */

typedef struct {
  const uint8_t *buf;
  size_t len;
  size_t pos;
  uint64_t binder_count;
  uint64_t group_size;
  bool runtime_only;
  CanonBuf *ph_out; /* optional: records canonical placeholder bytes with 0x05+member -> 0x04 */
  StigiDecodeResult res;
  int depth;
} DecCtx;

static bool dec_fail(DecCtx *dc, StigiDecodeReason r, size_t off, const char *msg) {
  if (dc->res.reason == STIGI_DECODE_OK) {
    dc->res.reason = r;
    dc->res.offset = off;
    (void)snprintf(dc->res.msg, sizeof dc->res.msg, "%s", msg);
  }
  return false;
}

static bool dec_u8_raw(DecCtx *dc, uint8_t *out, bool copy_ph) {
  if (dc->pos >= dc->len) return dec_fail(dc, STIGI_DECODE_TRUNCATED, dc->pos, "unexpected EOF");
  uint8_t b = dc->buf[dc->pos++];
  if (copy_ph && dc->ph_out != NULL) cb_u8(dc->ph_out, b);
  *out = b;
  return true;
}

static bool dec_bytes_raw(DecCtx *dc, size_t n, bool copy_ph) {
  if (n > dc->len - dc->pos) return dec_fail(dc, STIGI_DECODE_TRUNCATED, dc->pos, "truncated bytes");
  if (copy_ph && dc->ph_out != NULL) cb_bytes(dc->ph_out, dc->buf + dc->pos, n);
  dc->pos += n;
  return true;
}

static bool dec_uleb_raw(DecCtx *dc, uint64_t *out, bool copy_ph) {
  size_t start = dc->pos;
  uint64_t val = 0;
  unsigned shift = 0;
  for (unsigned i = 0; i < 10; i++) {
    if (dc->pos >= dc->len) {
      return dec_fail(dc, STIGI_DECODE_TRUNCATED, dc->pos, "truncated ULEB128");
    }
    uint8_t b = dc->buf[dc->pos++];
    if (copy_ph && dc->ph_out != NULL) cb_u8(dc->ph_out, b);
    if (i == 9 && b > 1U) {
      return dec_fail(dc, STIGI_DECODE_NUMBER_TOO_LARGE, start, "ULEB128 exceeds 64 bits");
    }
    if ((b & 0x80U) == 0) {
      if (i > 0 && b == 0) {
        return dec_fail(dc, STIGI_DECODE_NON_MINIMAL_NUMBER, start, "non-minimal ULEB128");
      }
      val |= ((uint64_t)b) << shift;
      *out = val;
      return true;
    }
    val |= ((uint64_t)(b & 0x7FU)) << shift;
    shift += 7;
  }
  return dec_fail(dc, STIGI_DECODE_NUMBER_TOO_LARGE, start, "ULEB128 too long");
}

static bool dec_bool_u8(DecCtx *dc, bool *out) {
  size_t off = dc->pos;
  uint8_t b = 0;
  if (!dec_u8_raw(dc, &b, true)) return false;
  if (b != 0 && b != 1) return dec_fail(dc, STIGI_DECODE_UNKNOWN_TAG, off, "invalid boolean byte");
  *out = (b == 1);
  return true;
}

static bool dec_str(DecCtx *dc, const uint8_t **out_ptr, size_t *out_len) {
  uint64_t n = 0;
  if (!dec_uleb_raw(dc, &n, true)) return false;
  if (n > dc->len - dc->pos) return dec_fail(dc, STIGI_DECODE_TRUNCATED, dc->pos, "truncated string");
  if (out_ptr != NULL) *out_ptr = dc->buf + dc->pos;
  if (out_len != NULL) *out_len = (size_t)n;
  return dec_bytes_raw(dc, (size_t)n, true);
}

static bool dec_ref(DecCtx *dc) {
  size_t off = dc->pos;
  uint8_t tag = 0;
  if (!dec_u8_raw(dc, &tag, false)) return false;
  switch (tag) {
    case 0x01: { /* Local(lvl) */
      if (dc->ph_out != NULL) cb_u8(dc->ph_out, tag);
      uint64_t lvl = 0;
      if (!dec_uleb_raw(dc, &lvl, true)) return false;
      if (lvl >= dc->binder_count) {
        return dec_fail(dc, STIGI_DECODE_UNBOUND_LOCAL, off, "local de Bruijn level out of scope");
      }
      return true;
    }
    case 0x02:
    case 0x03:
      if (dc->ph_out != NULL) cb_u8(dc->ph_out, tag);
      return true;
    case 0x04:
      /* SelfGroup placeholder is NEVER valid in canonical bytes (identity.md:54) */
      return dec_fail(dc, STIGI_DECODE_UNKNOWN_TAG, off, "placeholder reference tag not allowed");
    case 0x05: { /* SelfGroup(member) */
      uint64_t mem = 0;
      if (!dec_uleb_raw(dc, &mem, false)) return false;
      if (mem >= dc->group_size) {
        return dec_fail(dc, STIGI_DECODE_MEMBER_OUT_OF_RANGE, off, "SCC member index out of range");
      }
      if (dc->ph_out != NULL) cb_u8(dc->ph_out, 0x04);
      return true;
    }
    case 0x06: /* External(32 bytes) */
      if (dc->ph_out != NULL) cb_u8(dc->ph_out, tag);
      return dec_bytes_raw(dc, 32, true);
    case 0x07: /* Builtin/literal */
      if (dc->ph_out != NULL) cb_u8(dc->ph_out, tag);
      return dec_str(dc, NULL, NULL);
    case 0x08: { /* EnumCase(enum_ref, case_idx) */
      if (dc->ph_out != NULL) cb_u8(dc->ph_out, tag);
      size_t ioff = dc->pos;
      uint8_t itag = 0;
      if (!dec_u8_raw(dc, &itag, false)) return false;
      if (itag == 0x05) {
        uint64_t mem = 0;
        if (!dec_uleb_raw(dc, &mem, false)) return false;
        if (mem >= dc->group_size) {
          return dec_fail(dc, STIGI_DECODE_MEMBER_OUT_OF_RANGE, ioff, "enum SCC member out of range");
        }
        if (dc->ph_out != NULL) cb_u8(dc->ph_out, 0x04);
      } else if (itag == 0x06) {
        if (dc->ph_out != NULL) cb_u8(dc->ph_out, itag);
        if (!dec_bytes_raw(dc, 32, true)) return false;
      } else {
        return dec_fail(dc, STIGI_DECODE_UNKNOWN_TAG, ioff, "invalid EnumCase inner ref tag");
      }
      uint64_t cidx = 0;
      return dec_uleb_raw(dc, &cidx, true);
    }
    case 0x09: { /* LocalMember(lvl, member) */
      if (dc->ph_out != NULL) cb_u8(dc->ph_out, tag);
      uint64_t lvl = 0;
      if (!dec_uleb_raw(dc, &lvl, true)) return false;
      if (lvl >= dc->binder_count) {
        return dec_fail(dc, STIGI_DECODE_UNBOUND_LOCAL, off, "local member binder out of scope");
      }
      return dec_str(dc, NULL, NULL);
    }
    default:
      return dec_fail(dc, STIGI_DECODE_UNKNOWN_TAG, off, "unknown reference tag");
  }
}

static bool dec_expr(DecCtx *dc);
static bool dec_type(DecCtx *dc);
static bool dec_decl(DecCtx *dc);

static bool dec_fact_item(DecCtx *dc) {
  if (++dc->depth > 512) return dec_fail(dc, STIGI_DECODE_UNKNOWN_TAG, dc->pos, "nesting too deep");
  size_t off = dc->pos;
  uint8_t kind = 0;
  bool is_neg = false, has_ev = false;
  if (!dec_u8_raw(dc, &kind, true) || !dec_bool_u8(dc, &is_neg) || !dec_bool_u8(dc, &has_ev)) {
    dc->depth--;
    return false;
  }
  if (has_ev) dc->binder_count++;
  bool ok = false;
  if (kind == STIGI_FACT_ATOM) {
    uint64_t argc = 0;
    ok = dec_ref(dc) && dec_uleb_raw(dc, &argc, true);
    for (uint64_t i = 0; ok && i < argc; i++) ok = dec_expr(dc);
  } else if (kind == STIGI_FACT_DENY) {
    uint64_t dcnt = 0;
    ok = dec_uleb_raw(dc, &dcnt, true);
    for (uint64_t i = 0; ok && i < dcnt; i++) ok = dec_fact_item(dc);
  } else if (kind == STIGI_FACT_CMP || kind == STIGI_FACT_ARITH) {
    size_t op_off = dc->pos;
    uint8_t op = 0;
    ok = dec_u8_raw(dc, &op, true);
    if (ok && op > (uint8_t)STIGI_OP_NEG) {
      ok = dec_fail(dc, STIGI_DECODE_UNKNOWN_TAG, op_off, "unknown fact operator");
    }
    if (ok && kind == STIGI_FACT_ARITH) ok = dec_ref(dc);
    if (ok) ok = dec_expr(dc) && dec_expr(dc);
  } else {
    ok = dec_fail(dc, STIGI_DECODE_UNKNOWN_TAG, off, "unknown fact item kind");
  }
  dc->depth--;
  return ok;
}

static bool dec_fact_group(DecCtx *dc) {
  uint64_t cnt = 0;
  if (!dec_uleb_raw(dc, &cnt, true)) return false;
  for (uint64_t i = 0; i < cnt; i++) {
    if (!dec_fact_item(dc)) return false;
  }
  return true;
}

static bool dec_type(DecCtx *dc) {
  if (++dc->depth > 512) return dec_fail(dc, STIGI_DECODE_UNKNOWN_TAG, dc->pos, "nesting too deep");
  size_t off = dc->pos;
  uint8_t tag = 0;
  if (!dec_u8_raw(dc, &tag, true)) { dc->depth--; return false; }
  if (tag == 0x00) { dc->depth--; return true; }
  if (tag < 0x10 || tag > (uint8_t)(0x10 + STIGI_TYPE_PTR)) {
    dc->depth--;
    return dec_fail(dc, STIGI_DECODE_UNKNOWN_TAG, off, "unknown type tag");
  }
  if (!dec_ref(dc)) { dc->depth--; return false; }
  uint64_t tac = 0, lac = 0;
  if (!dec_uleb_raw(dc, &tac, true)) { dc->depth--; return false; }
  for (uint64_t i = 0; i < tac; i++) {
    if (!dec_type(dc)) { dc->depth--; return false; }
  }
  if (!dec_uleb_raw(dc, &lac, true)) { dc->depth--; return false; }
  for (uint64_t i = 0; i < lac; i++) {
    size_t loff = dc->pos;
    uint8_t lkind = 0;
    if (!dec_u8_raw(dc, &lkind, true)) { dc->depth--; return false; }
    if (lkind == 0x01) {
      uint64_t c = 0, nt = 0, prev_lvl = 0;
      if (!dec_uleb_raw(dc, &c, true) || !dec_uleb_raw(dc, &nt, true)) { dc->depth--; return false; }
      for (uint64_t k = 0; k < nt; k++) {
        size_t lv_off = dc->pos;
        uint64_t lvl = 0, coef = 0;
        if (!dec_uleb_raw(dc, &lvl, true) || !dec_uleb_raw(dc, &coef, true)) {
          dc->depth--;
          return false;
        }
        if (lvl >= dc->binder_count) {
          dc->depth--;
          return dec_fail(dc, STIGI_DECODE_UNBOUND_LOCAL, lv_off, "linear term binder out of scope");
        }
        if (k > 0 && lvl <= prev_lvl) {
          dc->depth--;
          return dec_fail(dc, STIGI_DECODE_UNKNOWN_TAG, lv_off, "linear terms not sorted");
        }
        prev_lvl = lvl;
      }
    } else if (lkind == 0x00) {
      if (!dec_expr(dc)) { dc->depth--; return false; }
    } else {
      dc->depth--;
      return dec_fail(dc, STIGI_DECODE_UNKNOWN_TAG, loff, "unknown length term tag");
    }
  }
  bool has_cap = false;
  if (!dec_bool_u8(dc, &has_cap)) { dc->depth--; return false; }
  if (has_cap && !dec_ref(dc)) { dc->depth--; return false; }
  dc->depth--;
  return true;
}

static bool dec_pat(DecCtx *dc) {
  size_t off = dc->pos;
  uint8_t tag = 0;
  if (!dec_u8_raw(dc, &tag, true)) return false;
  if (tag < 0x20 || tag > (uint8_t)(0x20 + STIGI_PAT_CTOR)) {
    return dec_fail(dc, STIGI_DECODE_UNKNOWN_TAG, off, "unknown pattern tag");
  }
  StigiPatKind pk = (StigiPatKind)(tag - 0x20);
  if (pk == STIGI_PAT_WILD) return true;
  if (pk == STIGI_PAT_VAR) { dc->binder_count++; return true; }
  if (pk == STIGI_PAT_INT) {
    uint64_t v = 0;
    return dec_uleb_raw(dc, &v, true);
  }
  if (pk == STIGI_PAT_BOOL) {
    bool bv = false;
    return dec_bool_u8(dc, &bv);
  }
  if (pk == STIGI_PAT_CTOR) {
    if (!dec_ref(dc)) return false;
    uint64_t fc = 0;
    if (!dec_uleb_raw(dc, &fc, true)) return false;
    for (uint64_t i = 0; i < fc; i++) {
      bool b = false;
      if (!dec_bool_u8(dc, &b)) return false;
      if (b) dc->binder_count++;
    }
    return true;
  }
  return dec_fail(dc, STIGI_DECODE_UNKNOWN_TAG, off, "unknown pattern kind");
}

static bool dec_expr(DecCtx *dc) {
  if (++dc->depth > 512) return dec_fail(dc, STIGI_DECODE_UNKNOWN_TAG, dc->pos, "nesting too deep");
  size_t off = dc->pos;
  uint8_t tag = 0;
  if (!dec_u8_raw(dc, &tag, true)) { dc->depth--; return false; }
  if (tag == 0x00) { dc->depth--; return true; }
  if (tag < 0x30 || tag > (uint8_t)(0x30 + STIGI_EXPR_LOCAL_DECL)) {
    dc->depth--;
    return dec_fail(dc, STIGI_DECODE_UNKNOWN_TAG, off, "unknown expression tag");
  }
  StigiExprKind ek = (StigiExprKind)(tag - 0x30);
  bool ok = true;
  switch (ek) {
    case STIGI_EXPR_LOCAL:
    case STIGI_EXPR_GLOBAL:
      ok = dec_ref(dc);
      break;
    case STIGI_EXPR_LIT_INT: {
      uint64_t v = 0;
      ok = dec_uleb_raw(dc, &v, true);
      break;
    }
    case STIGI_EXPR_LIT_CHAR: {
      uint8_t c = 0;
      ok = dec_u8_raw(dc, &c, true);
      break;
    }
    case STIGI_EXPR_LIT_STR:
      ok = dec_str(dc, NULL, NULL);
      break;
    case STIGI_EXPR_LIT_BOOL: {
      bool bv = false;
      ok = dec_bool_u8(dc, &bv);
      break;
    }
    case STIGI_EXPR_LIT_UNIT:
      break;
    case STIGI_EXPR_CALL: {
      uint64_t tac = 0, cac = 0, ac = 0;
      ok = dec_expr(dc) && dec_uleb_raw(dc, &tac, true);
      for (uint64_t i = 0; ok && i < tac; i++) ok = dec_type(dc);
      if (ok) ok = dec_uleb_raw(dc, &cac, true);
      for (uint64_t i = 0; ok && i < cac; i++) ok = dec_ref(dc);
      if (ok) ok = dec_uleb_raw(dc, &ac, true);
      for (uint64_t i = 0; ok && i < ac; i++) ok = dec_expr(dc);
      break;
    }
    case STIGI_EXPR_BINOP: {
      size_t op_off = dc->pos;
      uint8_t op = 0;
      ok = dec_u8_raw(dc, &op, true);
      if (ok && op > (uint8_t)STIGI_OP_NEG) {
        ok = dec_fail(dc, STIGI_DECODE_UNKNOWN_TAG, op_off, "unknown binop");
      }
      if (ok) ok = dec_expr(dc) && dec_expr(dc);
      break;
    }
    case STIGI_EXPR_UNOP: {
      size_t op_off = dc->pos;
      uint8_t op = 0;
      ok = dec_u8_raw(dc, &op, true);
      if (ok && op > (uint8_t)STIGI_OP_NEG) {
        ok = dec_fail(dc, STIGI_DECODE_UNKNOWN_TAG, op_off, "unknown unop");
      }
      if (ok) ok = dec_expr(dc);
      break;
    }
    case STIGI_EXPR_IF:
      ok = dec_expr(dc) && dec_expr(dc) && dec_expr(dc);
      break;
    case STIGI_EXPR_WHILE:
      ok = dec_expr(dc);
      if (ok && !dc->runtime_only) {
        bool hd = false;
        ok = dec_bool_u8(dc, &hd);
        if (ok && hd) ok = dec_expr(dc);
      }
      if (ok) ok = dec_expr(dc);
      break;
    case STIGI_EXPR_MATCH: {
      uint64_t ac = 0;
      ok = dec_expr(dc) && dec_uleb_raw(dc, &ac, true);
      for (uint64_t i = 0; ok && i < ac; i++) {
        uint64_t saved = dc->binder_count;
        ok = dec_pat(dc) && dec_expr(dc);
        dc->binder_count = saved;
      }
      break;
    }
    case STIGI_EXPR_VAL:
      ok = dec_type(dc) && dec_expr(dc) && dec_pat(dc);
      break;
    case STIGI_EXPR_VAR:
      ok = dec_type(dc) && dec_expr(dc);
      if (ok) {
        dc->binder_count++;
        if (!dc->runtime_only) {
          bool hi = false;
          ok = dec_bool_u8(dc, &hi);
          if (ok && hi) ok = dec_fact_group(dc);
        }
      }
      break;
    case STIGI_EXPR_ASSIGN:
      ok = dec_ref(dc) && dec_expr(dc);
      break;
    case STIGI_EXPR_INDEX_READ:
    case STIGI_EXPR_INDEX_WRITE:
      ok = dec_expr(dc) && dec_expr(dc) && dec_expr(dc);
      break;
    case STIGI_EXPR_HANDLE: {
      uint64_t tac = 0, cac = 0, ac = 0;
      bool hb = false, hbody = false;
      ok = dec_ref(dc) && dec_uleb_raw(dc, &tac, true);
      for (uint64_t i = 0; ok && i < tac; i++) ok = dec_type(dc);
      if (ok) ok = dec_uleb_raw(dc, &cac, true);
      for (uint64_t i = 0; ok && i < cac; i++) ok = dec_ref(dc);
      if (ok) ok = dec_uleb_raw(dc, &ac, true);
      for (uint64_t i = 0; ok && i < ac; i++) ok = dec_expr(dc);
      if (ok) ok = dec_bool_u8(dc, &hb) && dec_bool_u8(dc, &hbody);
      if (ok) {
        if (hbody) {
          uint64_t saved = dc->binder_count;
          if (hb) dc->binder_count++;
          ok = dec_expr(dc);
          dc->binder_count = saved;
        } else {
          if (hb) dc->binder_count++;
        }
      }
      break;
    }
    case STIGI_EXPR_ASSERT:
      if (dc->runtime_only) {
        ok = dec_fail(dc, STIGI_DECODE_UNKNOWN_TAG, off, "assert not allowed in runtime unit");
      } else {
        ok = dec_expr(dc) && dec_fact_group(dc);
      }
      break;
    case STIGI_EXPR_DENY:
      if (dc->runtime_only) {
        ok = dec_fail(dc, STIGI_DECODE_UNKNOWN_TAG, off, "deny not allowed in runtime unit");
      } else {
        ok = dec_fact_item(dc);
      }
      break;
    case STIGI_EXPR_BLOCK: {
      uint64_t saved = dc->binder_count;
      uint64_t cnt = 0;
      ok = dec_uleb_raw(dc, &cnt, true);
      for (uint64_t i = 0; ok && i < cnt; i++) ok = dec_expr(dc);
      dc->binder_count = saved;
      break;
    }
    case STIGI_EXPR_LOCAL_DECL:
      dc->binder_count++;
      ok = dec_decl(dc);
      break;
  }
  dc->depth--;
  return ok;
}

static bool dec_def_head(DecCtx *dc) {
  uint64_t tpc = 0, pc = 0;
  if (!dec_uleb_raw(dc, &tpc, true)) return false;
  dc->binder_count += tpc;
  if (!dec_uleb_raw(dc, &pc, true)) return false;
  dc->binder_count += pc;
  if (dc->runtime_only) {
    uint64_t rc = 0;
    if (!dec_uleb_raw(dc, &rc, true)) return false;
    for (uint64_t i = 0; i < rc; i++) {
      bool hb = false;
      if (!dec_bool_u8(dc, &hb)) return false;
      if (hb) dc->binder_count++;
      if (!dec_type(dc)) return false;
    }
    for (uint64_t i = 0; i < pc; i++) {
      if (!dec_type(dc)) return false;
    }
    return dec_type(dc);
  }
  uint64_t pgc = 0;
  if (!dec_uleb_raw(dc, &pgc, true)) return false;
  for (uint64_t i = 0; i < pgc; i++) {
    bool is_f = false;
    if (!dec_bool_u8(dc, &is_f)) return false;
    if (is_f) {
      if (!dec_fact_group(dc)) return false;
    } else {
      uint64_t cc = 0;
      if (!dec_uleb_raw(dc, &cc, true)) return false;
      for (uint64_t c = 0; c < cc; c++) {
        bool hb = false;
        if (!dec_bool_u8(dc, &hb)) return false;
        if (hb) dc->binder_count++;
        if (!dec_type(dc)) return false;
      }
    }
  }
  for (uint64_t i = 0; i < pc; i++) {
    if (!dec_type(dc)) return false;
  }
  uint64_t pvf = 0, exf = 0;
  if (!dec_uleb_raw(dc, &pvf, true)) return false;
  for (uint64_t i = 0; i < pvf; i++) {
    if (!dec_fact_group(dc)) return false;
  }
  if (!dec_type(dc)) return false;
  if (!dec_uleb_raw(dc, &exf, true)) return false;
  for (uint64_t i = 0; i < exf; i++) {
    if (!dec_fact_group(dc)) return false;
  }
  bool hd = false;
  if (!dec_bool_u8(dc, &hd)) return false;
  if (hd && !dec_expr(dc)) return false;
  return true;
}

static bool dec_decl(DecCtx *dc) {
  if (++dc->depth > 512) return dec_fail(dc, STIGI_DECODE_UNKNOWN_TAG, dc->pos, "nesting too deep");
  uint64_t saved = dc->binder_count;
  size_t off = dc->pos;
  uint8_t tag = 0;
  if (!dec_u8_raw(dc, &tag, true)) { dc->depth--; return false; }
  if (tag < 0x80 || tag > (uint8_t)(0x80 + STIGI_DECL_NEG_TEST)) {
    dc->depth--;
    return dec_fail(dc, STIGI_DECODE_UNKNOWN_TAG, off, "unknown declaration tag");
  }
  StigiDeclKind dk = (StigiDeclKind)(tag - 0x80);
  bool ok = true;
  switch (dk) {
    case STIGI_DECL_DEF:
    case STIGI_DECL_LEMMA_DEF:
      ok = dec_def_head(dc) && dec_expr(dc);
      break;
    case STIGI_DECL_VAL:
    case STIGI_DECL_TEST:
    case STIGI_DECL_NEG_TEST: {
      uint64_t cc = 0;
      ok = dec_uleb_raw(dc, &cc, true);
      for (uint64_t i = 0; ok && i < cc; i++) {
        bool hb = false;
        ok = dec_bool_u8(dc, &hb);
        if (ok && hb) dc->binder_count++;
        if (ok) ok = dec_type(dc);
      }
      if (ok) ok = dec_type(dc) && dec_expr(dc);
      break;
    }
    case STIGI_DECL_ENUM: {
      uint64_t tpc = 0, cc = 0;
      ok = dec_str(dc, NULL, NULL) && dec_uleb_raw(dc, &tpc, true);
      if (ok) {
        dc->binder_count += tpc;
        ok = dec_uleb_raw(dc, &cc, true);
      }
      for (uint64_t c = 0; ok && c < cc; c++) {
        uint64_t csaved = dc->binder_count;
        uint64_t fc = 0;
        ok = dec_str(dc, NULL, NULL) && dec_uleb_raw(dc, &fc, true);
        for (uint64_t f = 0; ok && f < fc; f++) {
          ok = dec_str(dc, NULL, NULL);
          if (ok) { dc->binder_count++; ok = dec_type(dc); }
        }
        if (ok && !dc->runtime_only) {
          bool hf = false;
          ok = dec_bool_u8(dc, &hf);
          if (ok && hf) ok = dec_fact_group(dc);
        }
        dc->binder_count = csaved;
      }
      break;
    }
    case STIGI_DECL_TYPE: {
      uint64_t tpc = 0, vpc = 0, cpc = 0;
      ok = dec_uleb_raw(dc, &tpc, true);
      if (ok) { dc->binder_count += tpc; ok = dec_uleb_raw(dc, &vpc, true); }
      for (uint64_t i = 0; ok && i < vpc; i++) {
        dc->binder_count++;
        ok = dec_type(dc);
      }
      if (ok) ok = dec_uleb_raw(dc, &cpc, true);
      for (uint64_t i = 0; ok && i < cpc; i++) {
        bool hb = false;
        ok = dec_bool_u8(dc, &hb);
        if (ok && hb) dc->binder_count++;
        if (ok) ok = dec_type(dc);
      }
      if (ok) ok = dec_type(dc);
      if (ok && !dc->runtime_only) {
        bool hf = false;
        ok = dec_bool_u8(dc, &hf);
        if (ok && hf) ok = dec_fact_group(dc);
      }
      break;
    }
    case STIGI_DECL_CAPABILITY: {
      uint64_t tpc = 0, sc = 0, stc = 0, mc = 0;
      bool hstart = false;
      ok = dec_str(dc, NULL, NULL) && dec_uleb_raw(dc, &tpc, true);
      if (ok) { dc->binder_count += tpc; ok = dec_uleb_raw(dc, &sc, true); }
      for (uint64_t i = 0; ok && i < sc; i++) ok = dec_str(dc, NULL, NULL);
      if (ok) ok = dec_bool_u8(dc, &hstart);
      if (ok && hstart) ok = dec_str(dc, NULL, NULL);
      if (ok) ok = dec_uleb_raw(dc, &stc, true);
      for (uint64_t i = 0; ok && i < stc; i++) ok = dec_str(dc, NULL, NULL);
      if (ok) ok = dec_uleb_raw(dc, &mc, true);
      for (uint64_t i = 0; ok && i < mc; i++) {
        ok = dec_str(dc, NULL, NULL) && dec_decl(dc);
      }
      break;
    }
    case STIGI_DECL_HANDLER: {
      uint64_t tpc = 0, cpc = 0, vpc = 0, vc = 0, dcnt = 0;
      ok = dec_uleb_raw(dc, &tpc, true);
      if (ok) { dc->binder_count += tpc; ok = dec_uleb_raw(dc, &cpc, true); }
      for (uint64_t i = 0; ok && i < cpc; i++) {
        bool hb = false;
        ok = dec_bool_u8(dc, &hb);
        if (ok && hb) dc->binder_count++;
        if (ok) ok = dec_type(dc);
      }
      if (ok) ok = dec_type(dc) && dec_uleb_raw(dc, &vpc, true);
      for (uint64_t i = 0; ok && i < vpc; i++) {
        dc->binder_count++;
        ok = dec_type(dc);
      }
      if (ok) ok = dec_uleb_raw(dc, &vc, true);
      for (uint64_t i = 0; ok && i < vc; i++) ok = dec_expr(dc);
      if (ok) ok = dec_uleb_raw(dc, &dcnt, true);
      for (uint64_t i = 0; ok && i < dcnt; i++) {
        ok = dec_str(dc, NULL, NULL) && dec_decl(dc);
      }
      break;
    }
    case STIGI_DECL_EXTERN: {
      bool is_val = false;
      ok = dec_str(dc, NULL, NULL) && dec_str(dc, NULL, NULL) && dec_bool_u8(dc, &is_val);
      if (ok) {
        if (is_val) ok = dec_type(dc);
        else ok = dec_def_head(dc) && dec_expr(dc);
      }
      break;
    }
    case STIGI_DECL_RELATION: {
      uint64_t cc = 0;
      ok = dec_str(dc, NULL, NULL) && dec_uleb_raw(dc, &cc, true);
      for (uint64_t i = 0; ok && i < cc; i++) {
        ok = dec_str(dc, NULL, NULL) && dec_type(dc);
      }
      break;
    }
    case STIGI_DECL_RULE: {
      uint64_t rvc = 0, bc = 0;
      ok = dec_uleb_raw(dc, &rvc, true);
      if (ok) {
        dc->binder_count += rvc;
        ok = dec_fact_item(dc) && dec_uleb_raw(dc, &bc, true);
      }
      for (uint64_t i = 0; ok && i < bc; i++) ok = dec_fact_item(dc);
      break;
    }
  }
  dc->binder_count = saved;
  dc->depth--;
  return ok;
}

static bool dec_unit_payload(const uint8_t *buf, size_t len, uint64_t group_size,
                             bool expect_runtime, CanonBuf *ph_out,
                             size_t base_off, StigiDecodeResult *out_res) {
  DecCtx dc;
  memset(&dc, 0, sizeof dc);
  dc.buf = buf;
  dc.len = len;
  dc.group_size = group_size;
  dc.runtime_only = expect_runtime;
  dc.ph_out = ph_out;

  const uint8_t *sp = NULL;
  size_t slen = 0;
  if (!dec_str(&dc, &sp, &slen)) {
    dc.res.offset += base_off;
    *out_res = dc.res;
    return false;
  }
  if (expect_runtime) {
    if (slen != 13 || memcmp(sp, "stigi/unit/v1", 13) != 0) {
      dec_fail(&dc, STIGI_DECODE_WRONG_SCHEMA, 0, "expected schema 'stigi/unit/v1'");
      dc.res.offset += base_off;
      *out_res = dc.res;
      return false;
    }
  } else {
    if (slen != 16 || memcmp(sp, "stigi/verdict/v1", 16) != 0) {
      dec_fail(&dc, STIGI_DECODE_WRONG_SCHEMA, 0, "expected schema 'stigi/verdict/v1'");
      dc.res.offset += base_off;
      *out_res = dc.res;
      return false;
    }
    if (!dec_bytes_raw(&dc, 32, true)) {
      dc.res.offset += base_off;
      *out_res = dc.res;
      return false;
    }
  }
  if (!dec_decl(&dc)) {
    dc.res.offset += base_off;
    *out_res = dc.res;
    return false;
  }
  if (dc.pos != dc.len) {
    dec_fail(&dc, STIGI_DECODE_TRAILING_BYTES, dc.pos, "trailing bytes after canonical unit");
    dc.res.offset += base_off;
    *out_res = dc.res;
    return false;
  }
  return true;
}

StigiDecodeResult stigi_canon_decode(const uint8_t *bytes, size_t len) {
  StigiDecodeResult res;
  memset(&res, 0, sizeof res);
  if (bytes == NULL || len == 0) {
    res.reason = STIGI_DECODE_TRUNCATED;
    (void)snprintf(res.msg, sizeof res.msg, "empty canonical buffer");
    return res;
  }

  DecCtx top;
  memset(&top, 0, sizeof top);
  top.buf = bytes;
  top.len = len;

  const uint8_t *sp = NULL;
  size_t slen = 0;
  if (!dec_str(&top, &sp, &slen)) return top.res;

  if (slen == 13 && memcmp(sp, "stigi/unit/v1", 13) == 0) {
    top.runtime_only = true;
    top.group_size = 0;
    if (!dec_decl(&top)) return top.res;
    if (top.pos != top.len) {
      dec_fail(&top, STIGI_DECODE_TRAILING_BYTES, top.pos, "trailing bytes after unit");
      return top.res;
    }
    return res;
  }

  if (slen == 16 && memcmp(sp, "stigi/verdict/v1", 16) == 0) {
    top.runtime_only = false;
    top.group_size = 0;
    if (!dec_bytes_raw(&top, 32, false)) return top.res;
    if (!dec_decl(&top)) return top.res;
    if (top.pos != top.len) {
      dec_fail(&top, STIGI_DECODE_TRAILING_BYTES, top.pos, "trailing bytes after verdict");
      return top.res;
    }
    return res;
  }

  bool is_group = (slen == 14 && memcmp(sp, "stigi/group/v1", 14) == 0);
  bool is_vgrp = (slen == 13 && memcmp(sp, "stigi/vgrp/v1", 13) == 0);
  if (is_group || is_vgrp) {
    uint64_t count = 0;
    if (!dec_uleb_raw(&top, &count, false)) return top.res;
    if (count == 0) {
      dec_fail(&top, STIGI_DECODE_NON_CANONICAL_GROUP_ORDER, top.pos, "empty group");
      return top.res;
    }
    CanonBuf prev_ph;
    memset(&prev_ph, 0, sizeof prev_ph);
    for (uint64_t i = 0; i < count; i++) {
      uint64_t mlen = 0;
      if (!dec_uleb_raw(&top, &mlen, false)) { free(prev_ph.buf); return top.res; }
      if (mlen > top.len - top.pos) {
        free(prev_ph.buf);
        dec_fail(&top, STIGI_DECODE_TRUNCATED, top.pos, "truncated group member");
        return top.res;
      }
      size_t moff = top.pos;
      CanonBuf cur_ph;
      memset(&cur_ph, 0, sizeof cur_ph);
      if (!dec_unit_payload(bytes + moff, (size_t)mlen, count, is_group,
                            is_group ? &cur_ph : NULL, moff, &res)) {
        free(cur_ph.buf);
        free(prev_ph.buf);
        return res;
      }
      if (is_group) {
        if (cur_ph.oom) {
          free(cur_ph.buf);
          free(prev_ph.buf);
          res.reason = STIGI_DECODE_TRUNCATED;
          (void)snprintf(res.msg, sizeof res.msg, "decoder OOM");
          return res;
        }
        if (i > 0) {
          size_t m = prev_ph.len < cur_ph.len ? prev_ph.len : cur_ph.len;
          int cmp = m > 0 ? memcmp(prev_ph.buf, cur_ph.buf, m) : 0;
          if (cmp == 0) {
            cmp = (prev_ph.len < cur_ph.len) ? -1 : (prev_ph.len > cur_ph.len ? 1 : 0);
          }
          if (cmp >= 0) {
            free(cur_ph.buf);
            free(prev_ph.buf);
            res.reason = STIGI_DECODE_NON_CANONICAL_GROUP_ORDER;
            res.offset = moff;
            (void)snprintf(res.msg, sizeof res.msg, "SCC members not in strict canonical order");
            return res;
          }
        }
        free(prev_ph.buf);
        prev_ph = cur_ph;
      }
      top.pos += (size_t)mlen;
    }
    free(prev_ph.buf);
    if (top.pos != top.len) {
      dec_fail(&top, STIGI_DECODE_TRAILING_BYTES, top.pos, "trailing bytes after group");
      return top.res;
    }
    return res;
  }

  dec_fail(&top, STIGI_DECODE_WRONG_SCHEMA, 0, "unrecognized canonical schema tag");
  return top.res;
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
  hc_free(&hc);
  if (out->oom) {
    free(out->buf);
    out->buf = NULL;
    return diag_set(diag, STIGI_ERR_OOM, -1, "canonical encode out of memory");
  }
  /* Verify standalone encoded bytes against strict decoder (identity.md:55) */
  if (scc_pos == NULL && !placeholder_pass) {
    StigiDecodeResult dr = stigi_canon_decode(out->buf, out->len);
    if (dr.reason != STIGI_DECODE_OK) {
      free(out->buf);
      out->buf = NULL;
      return diag_set(diag, STIGI_ERR_INVALID, -1,
                      "canonical encode failed strict self-decode (%s at %zu: %s)",
                      stigi_decode_reason_name(dr.reason), dr.offset, dr.msg);
    }
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
  uint32_t *members;
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
    bool ambiguous = false;
    for (uint32_t i = 0; i + 1 < count; i++) {
      if (cmp_scc_member(&sm[i], &sm[i + 1]) == 0) {
        ambiguous = true;
        tc->status = add_ndiag(tc->diags, STIGI_NAME_AMBIGUOUS_GROUP,
                               tc->img->units[sm[i].uidx].ast->decl->span, tc->diag,
                               "ambiguous recursive group: '%s' and '%s' have identical placeholder encoding",
                               tc->img->units[sm[i].uidx].name,
                               tc->img->units[sm[i + 1].uidx].name);
        break;
      }
    }
    if (!ambiguous && tc->status == STIGI_OK) {
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
      if (tc->status == STIGI_OK && grp.oom) {
        tc->status = diag_set(tc->diag, STIGI_ERR_OOM, -1, "tarjan_hash_scc: OOM");
      }
      if (tc->status == STIGI_OK) {
        /* identity.md:55: decode every group's bytes again before its digest is used */
        StigiDecodeResult dr = stigi_canon_decode(grp.buf, grp.len);
        if (dr.reason != STIGI_DECODE_OK) {
          tc->status = diag_set(tc->diag, STIGI_ERR_INVALID, -1,
                                "group encode failed strict self-decode (%s at %zu: %s)",
                                stigi_decode_reason_name(dr.reason), dr.offset, dr.msg);
        }
      }
      if (tc->status == STIGI_OK) {
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
      }
      free(grp.buf);

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
        if (tc->status == STIGI_OK && vgrp.oom) {
          tc->status = diag_set(tc->diag, STIGI_ERR_OOM, -1, "tarjan_hash_scc: OOM");
        }
        if (tc->status == STIGI_OK) {
          StigiDecodeResult vdr = stigi_canon_decode(vgrp.buf, vgrp.len);
          if (vdr.reason != STIGI_DECODE_OK) {
            tc->status = diag_set(tc->diag, STIGI_ERR_INVALID, -1,
                                  "vgrp encode failed strict self-decode (%s at %zu: %s)",
                                  stigi_decode_reason_name(vdr.reason), vdr.offset, vdr.msg);
          }
        }
        if (tc->status == STIGI_OK) {
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
        }
        free(vgrp.buf);
      }
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
    uint32_t mcnt = 0;
    for (;;) {
      uint32_t w = tc->stack[--tc->sp];
      tc->on_stack[w] = false;
      tc->members[mcnt++] = w;
      if (w == v) break;
    }
    tarjan_hash_scc(tc, tc->members, mcnt);
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
  tc.members = malloc((size_t)n * sizeof *tc.members);
  tc.on_stack = calloc(n, sizeof *tc.on_stack);
  tc.scc_pos = malloc((size_t)n * sizeof *tc.scc_pos);
  if (!tc.index || !tc.lowlink || !tc.stack || !tc.members || !tc.on_stack || !tc.scc_pos) {
    free(tc.index); free(tc.lowlink); free(tc.stack); free(tc.members);
    free(tc.on_stack); free(tc.scc_pos);
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
  free(tc.members);
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

  if (stigi_image_find(img, iname, nlen) >= 0) {
    return add_ndiag(diags, STIGI_NAME_DUPLICATE, decl->span, diag,
                     "duplicate unit '%s'", iname);
  }
  int32_t ci_hit = image_find_ci(img, iname, nlen);
  if (ci_hit >= 0) {
    return add_ndiag(diags, STIGI_NAME_CASE_PAIR, decl->span, diag,
                     "case-pair collision between '%s' and '%s'", img->units[ci_hit].name, iname);
  }

  if (img->unit_count == img->unit_cap) {
    uint32_t ncap = img->unit_cap == 0 ? 16U : img->unit_cap * 2U;
    if (ncap <= img->unit_cap) return diag_set(diag, STIGI_ERR_OVERFLOW, -1, "unit table overflow");
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
  /* Roots: pub def, pub val, Test, NegTest (names.md 6) */
  u->is_root = (decl->kind == STIGI_DECL_TEST || decl->kind == STIGI_DECL_NEG_TEST ||
                (decl->is_pub && (decl->kind == STIGI_DECL_DEF || decl->kind == STIGI_DECL_VAL)));

  /* Initial canonical BLAKE3-256 unit_id & verdict_key (recomputed in SCC order on resolve) */
  CanonBuf cb;
  st = encode_unit_canon(img, uidx, NULL, false, true, &cb, diag);
  if (st != STIGI_OK) { img->unit_count--; return st; }
  blake3_hash(cb.buf, cb.len, u->unit_id);
  free(cb.buf);
  st = encode_unit_canon(img, uidx, NULL, false, false, &cb, diag);
  if (st != STIGI_OK) { img->unit_count--; return st; }
  blake3_hash(cb.buf, cb.len, u->verdict_key);
  free(cb.buf);

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

  /* 1. Check unit's aliases (exact or leading prefix before '.') */
  char aliased[512];
  for (uint32_t i = 0; i < u->ast->alias_count; i++) {
    StigiStr an = u->ast->aliases[i].name;
    StigiStr at = u->ast->aliases[i].target;
    if (an.len == ref_len && memcmp(an.ptr, ref, ref_len) == 0) {
      ref = at.ptr;
      ref_len = at.len;
      break;
    }
    if (ref_len > an.len && memcmp(ref, an.ptr, an.len) == 0 && ref[an.len] == '.') {
      size_t tail = ref_len - an.len;
      if (at.len + tail < sizeof(aliased)) {
        memcpy(aliased, at.ptr, at.len);
        memcpy(aliased + at.len, ref + an.len, tail);
        ref = aliased;
        ref_len = at.len + tail;
        break;
      }
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

int32_t stigi_image_resolve_enum_case(const StigiImage *img, uint32_t from_idx,
                                      const char *ref, size_t ref_len,
                                      uint32_t *out_case_idx, bool *out_ambiguous) {
  if (out_ambiguous != NULL) *out_ambiguous = false;
  if (img == NULL || from_idx >= img->unit_count || ref == NULL || ref_len == 0) return -1;

  /* 1. Qualified EnumName.CaseName (e.g. AppendResult.Appended, Stigi.Table.AppendResult.Appended) */
  size_t last_dot = 0;
  bool has_dot = false;
  for (size_t i = 0; i < ref_len; i++) {
    if (ref[i] == '.') { last_dot = i; has_dot = true; }
  }
  if (has_dot && last_dot > 0 && last_dot + 1 < ref_len) {
    const char *epart = ref;
    size_t elen = last_dot;
    const char *cpart = ref + last_dot + 1;
    size_t clen = ref_len - (last_dot + 1);
    int32_t eidx = stigi_image_resolve_name(img, from_idx, epart, elen);
    if (eidx >= 0) {
      const StigiDecl *ed = img->units[eidx].ast->decl;
      if (ed->kind == STIGI_DECL_ENUM) {
        for (uint32_t c = 0; c < ed->u.enum_decl.case_count; c++) {
          StigiStr cn = ed->u.enum_decl.cases[c].name;
          if (cn.len == clen && memcmp(cn.ptr, cpart, clen) == 0) {
            if (out_case_idx != NULL) *out_case_idx = c;
            return eidx;
          }
        }
      }
    }
    return -1;
  }

  /* 2. Unqualified CaseName: order-independent search preferring same namespace, detecting ambiguity */
  int32_t ns_hit = -1;
  uint32_t ns_cidx = 0;
  uint32_t ns_count = 0;
  int32_t vis_hit = -1;
  uint32_t vis_cidx = 0;
  uint32_t vis_count = 0;

  for (uint32_t i = 0; i < img->unit_count; i++) {
    const StigiDecl *d = img->units[i].ast->decl;
    if (d->kind != STIGI_DECL_ENUM) continue;
    for (uint32_t c = 0; c < d->u.enum_decl.case_count; c++) {
      StigiStr cn = d->u.enum_decl.cases[c].name;
      if (cn.len == ref_len && memcmp(cn.ptr, ref, ref_len) == 0) {
        if (same_namespace_or_sub(&img->units[i], &img->units[from_idx])) {
          ns_hit = (int32_t)i;
          ns_cidx = c;
          ns_count++;
        } else if (stigi_image_visible_from(img, i, from_idx)) {
          vis_hit = (int32_t)i;
          vis_cidx = c;
          vis_count++;
        }
        break;
      }
    }
  }
  if (ns_count == 1) {
    if (out_case_idx != NULL) *out_case_idx = ns_cidx;
    return ns_hit;
  }
  if (ns_count > 1) {
    if (out_ambiguous != NULL) *out_ambiguous = true;
    return -1;
  }
  if (vis_count == 1) {
    if (out_case_idx != NULL) *out_case_idx = vis_cidx;
    return vis_hit;
  }
  if (vis_count > 1) {
    if (out_ambiguous != NULL) *out_ambiguous = true;
    return -1;
  }
  return -1;
}

static bool is_builtin_name(const char *s, size_t len) {
  static const char *const k_builtins[] = {
    "U8", "U32", "U64", "I32", "I64", "Bool", "Unit", "Never", "Type",
    "Array", "Cell", "Ptr", "self", "result", "_", "size",
    "Stigi.Test", "Stigi.Test.Result", "Stigi.Test.expect",
    "Stigi.NegTest", "Stigi.NegTest.expect",
    "Stigi.Mut", "Stigi.Region", "Stigi.Integer", "Stigi.Arena", "Stigi.Heap",
    "Stigi.File", "Stigi.Output", "Stigi.Socket", "Stigi.Clock", "Stigi.Usage",
    "Array.tryFill", "Array.tryGrow", "Cell.tryMake",
    "Filled", "FillFailed", "Grown", "GrowFailed",
    "U64.ofU8", "U64.ofI64", "U64.ofI32", "U64.ofU32",
    "U8.ofU64", "U32.ofU64", "I64.ofU64", "I32.ofU64",
    "U64.mix", "U64.wrappingAdd", "U64.wrappingSubtract", "U64.wrappingMultiply",
    "U64.bitwiseAnd", "U64.bitwiseOr", "U64.bitwiseXor", "U64.shiftLeft", "U64.shiftRight",
    "U32.wrappingAdd", "U32.wrappingSubtract", "U32.wrappingMultiply", "U32.rotateRight",
    "U32.bitwiseAnd", "U32.bitwiseOr", "U32.bitwiseXor", "U32.shiftLeft", "U32.shiftRight",
    "below", "lessOrEqual", "at", "valid", "overwrites", "decreases", "outlives",
    "Stigi.Facts.below", "Stigi.Facts.lessOrEqual", "Stigi.Facts.overwrites",
    "Stigi.Facts.at", "Stigi.Facts.outlives", "Stigi.Facts.decreases",
    "Stigi.Gate.cleared", "Stigi.Gate.owes", "Stigi.Gate.reject",
    "Stigi.Raw.isMapFailed", "Stigi.Raw.nullPtr"
  };
  for (size_t i = 0; i < sizeof(k_builtins) / sizeof(k_builtins[0]); i++) {
    if (strlen(k_builtins[i]) == len && memcmp(k_builtins[i], s, len) == 0) return true;
  }
  if (len > 11 && memcmp(s, "Stigi.Code.", 11) == 0) {
    return stigi_is_known_code_case(s + 11, len - 11);
  }
  if (len > 9 && memcmp(s, "Stigi.Id.", 9) == 0) {
    return is_known_id_type(s + 9, len - 9);
  }
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
    if (ncap <= img->edge_cap) return diag_set(diag, STIGI_ERR_OVERFLOW, -1, "edge table overflow");
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
  StigiStr name;
  int32_t cap_unit; /* >= 0 if capability binder resolved to unit, -2 if builtin cap, -1 otherwise */
} LocalEntry;

typedef struct {
  StigiImage *img;
  uint32_t from_idx;
  StigiNameDiags *diags;
  Diag *diag;
  LocalEntry *locals;
  uint32_t local_count;
  uint32_t local_cap;
} ResCtx;

static void res_ctx_free(ResCtx *ctx) {
  free(ctx->locals);
  ctx->locals = NULL;
  ctx->local_count = ctx->local_cap = 0;
}

static const LocalEntry *find_local_entry(const ResCtx *ctx, StigiStr s) {
  for (uint32_t i = ctx->local_count; i > 0; i--) {
    if (ctx->locals[i - 1].name.len == s.len &&
        memcmp(ctx->locals[i - 1].name.ptr, s.ptr, s.len) == 0) {
      return &ctx->locals[i - 1];
    }
  }
  return NULL;
}

static bool is_local(const ResCtx *ctx, StigiStr s) {
  return find_local_entry(ctx, s) != NULL;
}

static bool is_unit_alias(const StigiImage *img, uint32_t uidx, StigiStr s) {
  if (uidx >= img->unit_count || s.len == 0) return false;
  const StigiUnit *u = img->units[uidx].ast;
  for (uint32_t i = 0; i < u->alias_count; i++) {
    if (u->aliases[i].name.len == s.len &&
        memcmp(u->aliases[i].name.ptr, s.ptr, s.len) == 0) {
      return true;
    }
  }
  return false;
}

static Status push_local_ex(ResCtx *ctx, StigiStr s, StigiSpan sp, bool check_dup, int32_t cap_unit) {
  if (s.len == 0 || (s.len == 1 && s.ptr[0] == '_')) return STIGI_OK;
  if (check_dup && (is_local(ctx, s) || is_unit_alias(ctx->img, ctx->from_idx, s))) {
    Status st = add_ndiag(ctx->diags, STIGI_NAME_DUPLICATE, sp, ctx->diag,
                          "duplicate binder or alias '%.*s'", (int)s.len, s.ptr);
    if (st != STIGI_OK) return st;
  }
  if (ctx->local_count == ctx->local_cap) {
    uint32_t ncap = ctx->local_cap == 0 ? 32U : ctx->local_cap * 2U;
    if (ncap <= ctx->local_cap) return diag_set(ctx->diag, STIGI_ERR_OVERFLOW, -1, "locals overflow");
    LocalEntry *nl = realloc(ctx->locals, (size_t)ncap * sizeof *nl);
    if (nl == NULL) return diag_set(ctx->diag, STIGI_ERR_OOM, -1, "locals out of memory");
    ctx->locals = nl;
    ctx->local_cap = ncap;
  }
  ctx->locals[ctx->local_count].name = s;
  ctx->locals[ctx->local_count].cap_unit = cap_unit;
  ctx->local_count++;
  return STIGI_OK;
}

static Status push_local(ResCtx *ctx, StigiStr s, StigiSpan sp, bool check_dup) {
  return push_local_ex(ctx, s, sp, check_dup, -1);
}

static int32_t resolve_cap_type_unit(const ResCtx *ctx, const StigiType *cap_ty) {
  if (cap_ty == NULL) return -1;
  int32_t tgt = stigi_image_resolve_name(ctx->img, ctx->from_idx,
                                         cap_ty->name.ptr, cap_ty->name.len);
  if (tgt >= 0 && ctx->img->units[tgt].ast->decl->kind == STIGI_DECL_CAPABILITY) {
    return tgt;
  }
  if (is_builtin_name(cap_ty->name.ptr, cap_ty->name.len)) return -2;
  return -1;
}

static bool cap_has_op(const StigiImage *img, uint32_t cidx, const char *op, size_t op_len) {
  if (cidx >= img->unit_count) return false;
  const StigiDecl *d = img->units[cidx].ast->decl;
  if (d->kind != STIGI_DECL_CAPABILITY) return false;
  for (uint32_t i = 0; i < d->u.cap_decl.member_count; i++) {
    const StigiDecl *m = d->u.cap_decl.members[i];
    if (m->kind == STIGI_DECL_DEF && m->name.len == op_len &&
        memcmp(m->name.ptr, op, op_len) == 0) {
      return true;
    }
  }
  return false;
}

static Status resolve_ref(ResCtx *ctx, StigiStr ref, StigiSpan sp, bool allow_case) {
  if (ref.len == 0 || is_local(ctx, ref)) return STIGI_OK;

  /* Capability operation call `binder.op` (capabilities.md 1) */
  const char *dot = memchr(ref.ptr, '.', ref.len);
  if (dot != NULL) {
    StigiStr prefix;
    prefix.ptr = ref.ptr;
    prefix.len = (size_t)(dot - ref.ptr);
    const LocalEntry *le = find_local_entry(ctx, prefix);
    if (le != NULL && le->cap_unit != -1) {
      const char *op = dot + 1;
      size_t op_len = ref.len - prefix.len - 1U;
      if (le->cap_unit == -2) return STIGI_OK;
      uint32_t cidx = (uint32_t)le->cap_unit;
      if (cap_has_op(ctx->img, cidx, op, op_len)) {
        if (!same_namespace_or_sub(&ctx->img->units[cidx], &ctx->img->units[ctx->from_idx])) {
          ctx->img->units[cidx].used_outside_ns = true;
        }
        return add_edge(ctx->img, ctx->from_idx, cidx, ctx->diag);
      }
      return add_ndiag(ctx->diags, STIGI_NAME_UNRESOLVED, sp, ctx->diag,
                       "capability '%s' has no operation '%.*s'",
                       ctx->img->units[cidx].name, (int)op_len, op);
    }
  }

  int32_t tgt = stigi_image_resolve_name(ctx->img, ctx->from_idx, ref.ptr, ref.len);
  bool amb = false;
  if (tgt < 0 && allow_case) {
    tgt = stigi_image_resolve_enum_case(ctx->img, ctx->from_idx, ref.ptr, ref.len, NULL, &amb);
  }
  if (tgt < 0) {
    if (!amb && is_builtin_name(ref.ptr, ref.len)) return STIGI_OK;
    return add_ndiag(ctx->diags, STIGI_NAME_UNRESOLVED, sp, ctx->diag,
                     amb ? "ambiguous unqualified enum case '%.*s' in '%s'"
                         : "unresolved name '%.*s' in '%s'",
                     (int)ref.len, ref.ptr, ctx->img->units[ctx->from_idx].name);
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
  Status st = STIGI_OK;
  if (!is_rule && it->evidence_name.len > 0) {
    st = push_local(ctx, it->evidence_name, it->span, true);
    if (st != STIGI_OK) return st;
  }
  if (it->kind == STIGI_FACT_ATOM) {
    st = resolve_ref(ctx, it->atom_name, it->span, false);
    if (st != STIGI_OK) return st;
    if (!is_rule) {
      for (uint32_t i = 0; i < it->arg_count; i++) {
        st = walk_expr(ctx, it->args[i]);
        if (st != STIGI_OK) return st;
      }
    }
  } else if (it->kind == STIGI_FACT_DENY) {
    for (uint32_t i = 0; i < it->deny_atom_count; i++) {
      st = walk_fact_item(ctx, &it->deny_atoms[i], is_rule);
      if (st != STIGI_OK) return st;
    }
  } else if (!is_rule) {
    if (it->kind == STIGI_FACT_ARITH && it->lhs_var.len > 0) {
      st = resolve_ref(ctx, it->lhs_var, it->span, false);
      if (st != STIGI_OK) return st;
    }
    if (it->lhs != NULL) {
      st = walk_expr(ctx, it->lhs);
      if (st != STIGI_OK) return st;
    }
    if (it->rhs != NULL) {
      st = walk_expr(ctx, it->rhs);
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
  if (ty->cap_arg.len > 0) {
    st = resolve_ref(ctx, ty->cap_arg, ty->span, false);
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
      for (uint32_t i = 0; i < e->u.call.cap_arg_count; i++) {
        st = resolve_ref(ctx, e->u.call.cap_args[i], e->span, false);
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
          if (st != STIGI_OK) { ctx->local_count = saved; return st; }
          for (uint32_t f = 0; f < pat->field_count; f++) {
            st = push_local(ctx, pat->fields[f], pat->span, false);
            if (st != STIGI_OK) { ctx->local_count = saved; return st; }
          }
        } else if (pat->kind == STIGI_PAT_VAR) {
          st = push_local(ctx, pat->name, pat->span, false);
          if (st != STIGI_OK) { ctx->local_count = saved; return st; }
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
    case STIGI_EXPR_HANDLE: {
      st = resolve_ref(ctx, e->u.handle_stmt.handler_name, e->span, false);
      if (st != STIGI_OK) return st;
      for (uint32_t i = 0; i < e->u.handle_stmt.type_arg_count; i++) {
        st = walk_type(ctx, e->u.handle_stmt.type_args[i]);
        if (st != STIGI_OK) return st;
      }
      for (uint32_t i = 0; i < e->u.handle_stmt.cap_arg_count; i++) {
        st = resolve_ref(ctx, e->u.handle_stmt.cap_args[i], e->span, false);
        if (st != STIGI_OK) return st;
      }
      for (uint32_t i = 0; i < e->u.handle_stmt.arg_count; i++) {
        st = walk_expr(ctx, e->u.handle_stmt.args[i]);
        if (st != STIGI_OK) return st;
      }
      int32_t cap_u = -1;
      int32_t hidx = stigi_image_resolve_name(ctx->img, ctx->from_idx,
                                              e->u.handle_stmt.handler_name.ptr,
                                              e->u.handle_stmt.handler_name.len);
      if (hidx >= 0 && ctx->img->units[hidx].ast->decl->kind == STIGI_DECL_HANDLER) {
        const StigiDecl *hd = ctx->img->units[hidx].ast->decl;
        int32_t cidx = stigi_image_resolve_name(ctx->img, (uint32_t)hidx,
                                                hd->u.handler_decl.for_cap.ptr,
                                                hd->u.handler_decl.for_cap.len);
        if (cidx >= 0) cap_u = cidx;
        else if (is_builtin_name(hd->u.handler_decl.for_cap.ptr, hd->u.handler_decl.for_cap.len)) {
          cap_u = -2;
        }
      } else if (is_builtin_name(e->u.handle_stmt.handler_name.ptr,
                                 e->u.handle_stmt.handler_name.len)) {
        cap_u = -2;
      }
      if (e->u.handle_stmt.body != NULL) {
        uint32_t saved = ctx->local_count;
        if (e->u.handle_stmt.binder.len > 0) {
          st = push_local_ex(ctx, e->u.handle_stmt.binder, e->span, true, cap_u);
          if (st != STIGI_OK) { ctx->local_count = saved; return st; }
        }
        st = walk_expr(ctx, e->u.handle_stmt.body);
        ctx->local_count = saved;
        return st;
      }
      if (e->u.handle_stmt.binder.len > 0) {
        return push_local_ex(ctx, e->u.handle_stmt.binder, e->span, true, cap_u);
      }
      return STIGI_OK;
    }
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
        int32_t cap_u = resolve_cap_type_unit(ctx, h->pre_groups[i].caps[c].cap_type);
        st = push_local_ex(ctx, h->pre_groups[i].caps[c].name, h->pre_groups[i].caps[c].span,
                           true, cap_u);
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
        int32_t cap_u = resolve_cap_type_unit(ctx, d->u.val_decl.caps[i].cap_type);
        st = push_local_ex(ctx, d->u.val_decl.caps[i].name, d->u.val_decl.caps[i].span,
                           true, cap_u);
        if (st != STIGI_OK) break;
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
      for (uint32_t i = 0; st == STIGI_OK && i < d->u.type_decl.cap_param_count; i++) {
        int32_t cap_u = resolve_cap_type_unit(ctx, d->u.type_decl.cap_params[i].cap_type);
        st = push_local_ex(ctx, d->u.type_decl.cap_params[i].name,
                           d->u.type_decl.cap_params[i].span, true, cap_u);
        if (st == STIGI_OK) st = walk_type(ctx, d->u.type_decl.cap_params[i].cap_type);
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
      for (uint32_t i = 0; st == STIGI_OK && i < d->u.handler_decl.cap_param_count; i++) {
        int32_t cap_u = resolve_cap_type_unit(ctx, d->u.handler_decl.cap_params[i].cap_type);
        st = push_local_ex(ctx, d->u.handler_decl.cap_params[i].name,
                           d->u.handler_decl.cap_params[i].span, true, cap_u);
        if (st == STIGI_OK) st = walk_type(ctx, d->u.handler_decl.cap_params[i].cap_type);
      }
      for (uint32_t i = 0; st == STIGI_OK && i < d->u.handler_decl.val_param_count; i++) {
        st = push_local(ctx, d->u.handler_decl.val_params[i].name,
                        d->u.handler_decl.val_params[i].span, true);
        if (st == STIGI_OK) st = walk_type(ctx, d->u.handler_decl.val_params[i].type);
      }
      if (st == STIGI_OK) {
        if (d->u.handler_decl.for_cap_type != NULL) {
          st = walk_type(ctx, d->u.handler_decl.for_cap_type);
        } else {
          st = resolve_ref(ctx, d->u.handler_decl.for_cap, d->span, false);
        }
      }
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

    if (d->is_pub && (d->kind == STIGI_DECL_TEST || d->kind == STIGI_DECL_NEG_TEST)) {
      Status st = add_ndiag(diags, STIGI_NAME_PUB_ON_TEST, d->span, diag,
                            "Test/NegTest '%s' cannot be 'pub'", u->name);
      if (st != STIGI_OK) return st;
    }

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
    res_ctx_free(&ctx);
    if (st != STIGI_OK) return st;

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

  return compute_all_unit_ids(img, diags, diag);
}
