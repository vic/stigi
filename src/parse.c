#include "parse.h"
#include "tenet_gate.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STIGI_MAX_PARSE_DEPTH 256U

typedef struct {
  const StigiTok *toks;
  uint32_t count;
  uint32_t pos;
  uint32_t depth;
  Arena *arena;
  StigiParseResult res;
  bool failed;
} Parser;

static bool p_enter_depth(Parser *p, StigiSpan sp) {
  if (p->depth >= STIGI_MAX_PARSE_DEPTH) {
    if (!p->failed) {
      p->failed = true;
      p->res.status = STIGI_ERR_INVALID;
      p->res.diag_kind = STIGI_PARSE_ERR_PARSE;
      p->res.diag_code = "parse";
      p->res.help = NULL;
      p->res.err_span = sp;
      (void)snprintf(p->res.msg, sizeof p->res.msg, "maximum nesting depth exceeded");
    }
    return false;
  }
  p->depth++;
  return true;
}

static void p_leave_depth(Parser *p) {
  if (p->depth > 0) p->depth--;
}

static bool p_grow(Parser *p, void **buf, uint32_t *cap, size_t elem_sz) {
  uint32_t ncap = (*cap == 0) ? 4U : (*cap <= UINT32_MAX / 2U ? (*cap) * 2U : 0U);
  if (ncap == 0 || (size_t)ncap > SIZE_MAX / elem_sz) {
    if (!p->failed) {
      p->failed = true;
      p->res.status = STIGI_ERR_OVERFLOW;
      p->res.diag_kind = STIGI_PARSE_ERR_PARSE;
      p->res.diag_code = "parse";
      p->res.help = NULL;
      (void)snprintf(p->res.msg, sizeof p->res.msg, "array capacity overflow");
    }
    return false;
  }
  void *nb = realloc(*buf, (size_t)ncap * elem_sz);
  if (nb == NULL) {
    if (!p->failed) {
      p->failed = true;
      p->res.status = STIGI_ERR_OOM;
      p->res.diag_kind = STIGI_PARSE_ERR_PARSE;
      p->res.diag_code = "parse";
      p->res.help = NULL;
      (void)snprintf(p->res.msg, sizeof p->res.msg, "out of memory");
    }
    return false;
  }
  *buf = nb;
  *cap = ncap;
  return true;
}

static void *p_alloc(Parser *p, size_t sz) {
  void *m = arena_alloc(p->arena, sz);
  if (m != NULL) {
    memset(m, 0, sz);
  }
  return m;
}

static void p_fail(Parser *p, StigiSpan sp, const char *fmt, ...) {
  if (p->failed) return;
  p->failed = true;
  p->res.status = STIGI_ERR_INVALID;
  p->res.diag_kind = STIGI_PARSE_ERR_PARSE;
  p->res.diag_code = "parse";
  p->res.help = NULL;
  p->res.err_span = sp;
  va_list ap;
  va_start(ap, fmt);
  (void)vsnprintf(p->res.msg, sizeof p->res.msg, fmt, ap);
  va_end(ap);
}

static void p_fail_oom(Parser *p) {
  if (p->failed) return;
  p->failed = true;
  p->res.status = STIGI_ERR_OOM;
  p->res.diag_kind = STIGI_PARSE_ERR_PARSE;
  p->res.diag_code = "parse";
  p->res.help = NULL;
  (void)snprintf(p->res.msg, sizeof p->res.msg, "out of memory");
}

static void p_fail_not_in_lang(Parser *p, StigiSpan sp, const char *kw) {
  if (p->failed) return;
  p->failed = true;
  p->res.status = STIGI_ERR_INVALID;
  p->res.diag_kind = STIGI_PARSE_ERR_NOT_IN_LANG;
  p->res.diag_code = "not-in-language";
  p->res.help = stigi_help_not_in_language(kw);
  p->res.err_span = sp;
  (void)snprintf(p->res.msg, sizeof p->res.msg, "'%s' is not in L0", kw != NULL ? kw : "construct");
}

static void p_fail_not_in_lang_tok(Parser *p, const StigiTok *t) {
  char kw[64];
  size_t n = t->len < sizeof(kw) - 1 ? t->len : sizeof(kw) - 1;
  memcpy(kw, t->ptr, n);
  kw[n] = '\0';
  if (t->kind == TOK_FLOAT) {
    p_fail_not_in_lang(p, t->span, "float");
  } else if (t->kind == TOK_HASH) {
    p_fail_not_in_lang(p, t->span, "#");
  } else if (t->kind == TOK_STR_INTERP) {
    p_fail_not_in_lang(p, t->span, "string-interpolation");
  } else if (t->kind == TOK_RARROW) {
    p_fail_not_in_lang(p, t->span, "lambda");
  } else if (t->kind == TOK_TYPE_ARROW) {
    p_fail_not_in_lang(p, t->span, "type-lambda");
  } else if (t->kind == TOK_ELLIPSIS) {
    p_fail_not_in_lang(p, t->span, "varargs");
  } else {
    p_fail_not_in_lang(p, t->span, kw);
  }
}

static const StigiTok *peek_raw(const Parser *p, uint32_t offset) {
  uint32_t idx = p->pos + offset;
  if (idx >= p->count) return &p->toks[p->count - 1];
  return &p->toks[idx];
}

static void skip_comments(Parser *p) {
  while (p->pos < p->count && p->toks[p->pos].kind == TOK_COMMENT) {
    p->pos++;
  }
}

static const StigiTok *peek(Parser *p) {
  skip_comments(p);
  return peek_raw(p, 0);
}

static const StigiTok *peek_next_non_comment(Parser *p) {
  skip_comments(p);
  uint32_t i = p->pos + 1;
  while (i < p->count && p->toks[i].kind == TOK_COMMENT) i++;
  if (i >= p->count) return &p->toks[p->count - 1];
  return &p->toks[i];
}

static void skip_newlines(Parser *p) {
  for (;;) {
    skip_comments(p);
    if (p->pos < p->count && p->toks[p->pos].kind == TOK_NEWLINE) {
      p->pos++;
      continue;
    }
    break;
  }
}

/* Peek next token skipping newlines and comments, without advancing p->pos */
static const StigiTok *peek_skip_nl(const Parser *p) {
  uint32_t i = p->pos;
  while (i < p->count && (p->toks[i].kind == TOK_NEWLINE || p->toks[i].kind == TOK_COMMENT)) {
    i++;
  }
  if (i >= p->count) return &p->toks[p->count - 1];
  return &p->toks[i];
}

static const StigiTok *advance(Parser *p) {
  skip_comments(p);
  const StigiTok *t = peek_raw(p, 0);
  if (t->kind != TOK_EOF) p->pos++;
  return t;
}

static bool check_not_in_lang(Parser *p) {
  const StigiTok *t = peek(p);
  if (t->kind == TOK_NOT_IN_LANG || t->kind == TOK_FLOAT || t->kind == TOK_HASH ||
      t->kind == TOK_STR_INTERP || t->kind == TOK_RARROW ||
      t->kind == TOK_TYPE_ARROW || t->kind == TOK_ELLIPSIS) {
    p_fail_not_in_lang_tok(p, t);
    return true;
  }
  return false;
}

static void skip_head_cont_nl(Parser *p, uint32_t decl_col) {
  if (peek(p)->kind != TOK_NEWLINE) return;
  const StigiTok *nxt = peek_skip_nl(p);
  if (nxt->span.col <= decl_col) return;
  StigiTokKind k = nxt->kind;
  if (k == TOK_LBRACKET || k == TOK_LBRACE || k == TOK_LPAREN ||
      k == TOK_LT || k == TOK_COLON || k == TOK_ASSIGN || k == TOK_KW_FOR) {
    skip_newlines(p);
  }
}

static bool expect(Parser *p, StigiTokKind k) {
  if (p->failed) return false;
  if (check_not_in_lang(p)) return false;
  const StigiTok *t = peek(p);
  if (t->kind != k) {
    p_fail(p, t->span, "expected '%s', got '%s'", stigi_tok_kind_name(k),
           stigi_tok_kind_name(t->kind));
    return false;
  }
  advance(p);
  return true;
}

static StigiStr tok_str(const StigiTok *t) {
  StigiStr s;
  s.ptr = t->ptr;
  s.len = t->len;
  return s;
}

static bool str_eq(StigiStr s, const char *lit) {
  size_t n = strlen(lit);
  return s.len == n && memcmp(s.ptr, lit, n) == 0;
}

static bool tok_is_ident_eq(const StigiTok *t, const char *lit) {
  size_t n = strlen(lit);
  return t->kind == TOK_IDENT && t->len == n && memcmp(t->ptr, lit, n) == 0;
}

static bool is_ident_like(StigiTokKind k) {
  return k == TOK_IDENT || k == TOK_KW_START || k == TOK_KW_STATE || k == TOK_KW_STOP;
}

static StigiStr parse_name(Parser *p) {
  StigiStr empty = {NULL, 0};
  if (p->failed || check_not_in_lang(p)) return empty;
  const StigiTok *t = peek(p);
  if (!is_ident_like(t->kind)) {
    p_fail(p, t->span, "expected identifier, got '%s'", stigi_tok_kind_name(t->kind));
    return empty;
  }
  advance(p);
  return tok_str(t);
}

static StigiStr parse_binder_name(Parser *p) {
  StigiStr empty = {NULL, 0};
  if (p->failed || check_not_in_lang(p)) return empty;
  const StigiTok *t = peek(p);
  if (is_ident_like(t->kind) || t->kind == TOK_UNDERSCORE) {
    advance(p);
    return tok_str(t);
  }
  p_fail(p, t->span, "expected binder name or '_', got '%s'", stigi_tok_kind_name(t->kind));
  return empty;
}

/* Forward declarations */
static StigiType *parse_type(Parser *p);
static StigiExpr *parse_expr(Parser *p, uint32_t base_col);
static StigiExpr *parse_block_or_expr(Parser *p, uint32_t base_col);
static StigiDecl *parse_decl(Parser *p, bool is_pub, uint32_t decl_col);
static StigiFactGroup parse_fact_group(Parser *p);
static StigiFactItem parse_fact_item(Parser *p, bool allow_evidence);

/* ---- Integer, char, and string literal parsing ---- */

static uint64_t parse_u64_lit(const char *s, uint32_t len, bool *out_overflow) {
  uint64_t val = 0;
  bool ov = false;
  if (len >= 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
    for (uint32_t i = 2; i < len; i++) {
      unsigned char c = (unsigned char)s[i];
      uint64_t d = 0;
      if (c >= '0' && c <= '9') d = (uint64_t)(c - '0');
      else if (c >= 'a' && c <= 'f') d = (uint64_t)(c - 'a' + 10);
      else if (c >= 'A' && c <= 'F') d = (uint64_t)(c - 'A' + 10);
      if (val > (UINT64_MAX >> 4)) {
        ov = true;
      } else {
        val = (val << 4) | d;
      }
    }
  } else {
    for (uint32_t i = 0; i < len; i++) {
      uint64_t d = (uint64_t)(s[i] - '0');
      if (val > UINT64_MAX / 10ULL || (val == UINT64_MAX / 10ULL && d > UINT64_MAX % 10ULL)) {
        ov = true;
      } else {
        val = val * 10ULL + d;
      }
    }
  }
  if (out_overflow != NULL) *out_overflow = ov;
  return val;
}

static uint8_t hex_nibble(char c) {
  if (c >= '0' && c <= '9') return (uint8_t)(c - '0');
  if (c >= 'a' && c <= 'f') return (uint8_t)(c - 'a' + 10);
  if (c >= 'A' && c <= 'F') return (uint8_t)(c - 'A' + 10);
  return 0;
}

static uint8_t decode_escape(const char *s, uint32_t len, uint32_t *pos) {
  if (*pos >= len) return 0;
  char c = s[(*pos)++];
  switch (c) {
    case 'n': return (uint8_t)'\n';
    case 't': return (uint8_t)'\t';
    case '\\': return (uint8_t)'\\';
    case '"': return (uint8_t)'"';
    case '\'': return (uint8_t)'\'';
    case '0': return 0;
    case 'x':
      if (*pos + 2 <= len) {
        uint8_t hi = hex_nibble(s[(*pos)++]);
        uint8_t lo = hex_nibble(s[(*pos)++]);
        return (uint8_t)((hi << 4) | lo);
      }
      return 0;
    default:
      return (uint8_t)c;
  }
}

static uint8_t parse_char_lit(const char *s, uint32_t len) {
  if (len < 3) return 0;
  uint32_t p = 1;
  if (s[p] == '\\') {
    p++;
    return decode_escape(s, len - 1, &p);
  }
  return (uint8_t)s[p];
}

static uint32_t decoded_str_len(const char *s, uint32_t len) {
  if (len < 2) return 0;
  uint32_t p = 1;
  uint32_t end = len - 1;
  uint32_t count = 0;
  while (p < end) {
    if (s[p] == '\\') {
      p++;
      (void)decode_escape(s, end, &p);
    } else {
      p++;
    }
    count++;
  }
  return count;
}

/* ---- Types ---- */

static StigiType *parse_type(Parser *p) {
  if (p->failed || check_not_in_lang(p)) return NULL;
  const StigiTok *t = peek(p);
  if (t->kind == TOK_LBRACKET) {
    p_fail_not_in_lang(p, t->span, "type-lambda");
    return NULL;
  }
  if (!p_enter_depth(p, t->span)) return NULL;
  if (t->kind != TOK_IDENT) {
    p_fail(p, t->span, "expected type name, got '%s'", stigi_tok_kind_name(t->kind));
    p_leave_depth(p);
    return NULL;
  }
  advance(p);
  if (tok_is_ident_eq(t, "ACell")) {
    p_fail_not_in_lang(p, t->span, "ACell");
    p_leave_depth(p);
    return NULL;
  }

  StigiType *ty = p_alloc(p, sizeof *ty);
  if (ty == NULL) {
    p_fail_oom(p);
    p_leave_depth(p);
    return NULL;
  }
  ty->span = t->span;
  ty->name = tok_str(t);
  if (str_eq(ty->name, "Array")) ty->kind = STIGI_TYPE_ARRAY;
  else if (str_eq(ty->name, "Cell")) ty->kind = STIGI_TYPE_CELL;
  else if (str_eq(ty->name, "Ptr")) ty->kind = STIGI_TYPE_PTR;
  else ty->kind = STIGI_TYPE_NAMED;

  /* Optional [Type, ...] */
  if (peek(p)->kind == TOK_LBRACKET) {
    advance(p);
    skip_newlines(p);
    StigiType **args = NULL;
    uint32_t cnt = 0, cap = 0;
    if (peek(p)->kind != TOK_RBRACKET) {
      for (;;) {
        skip_newlines(p);
        StigiType *a = parse_type(p);
        if (p->failed) { free(args); p_leave_depth(p); return NULL; }
        if (cnt == cap && !p_grow(p, (void **)&args, &cap, sizeof *args)) {
          free(args); p_leave_depth(p); return NULL;
        }
        args[cnt++] = a;
        skip_newlines(p);
        if (peek(p)->kind == TOK_COMMA) {
          advance(p);
          skip_newlines(p);
        } else {
          break;
        }
      }
    }
    skip_newlines(p);
    if (!expect(p, TOK_RBRACKET)) { free(args); p_leave_depth(p); return NULL; }
    if (cnt > 0) {
      ty->type_args = p_alloc(p, (size_t)cnt * sizeof *ty->type_args);
      if (ty->type_args == NULL) { free(args); p_fail_oom(p); p_leave_depth(p); return NULL; }
      memcpy(ty->type_args, args, (size_t)cnt * sizeof *args);
      ty->type_arg_count = cnt;
    }
    free(args);
  }

  /* Optional (lenTerm, ...) */
  if (peek(p)->kind == TOK_LPAREN) {
    advance(p);
    skip_newlines(p);
    StigiExpr **largs = NULL;
    uint32_t cnt = 0, cap = 0;
    if (peek(p)->kind != TOK_RPAREN) {
      for (;;) {
        skip_newlines(p);
        StigiExpr *e = parse_expr(p, 0);
        if (p->failed) { free(largs); p_leave_depth(p); return NULL; }
        if (cnt == cap && !p_grow(p, (void **)&largs, &cap, sizeof *largs)) {
          free(largs); p_leave_depth(p); return NULL;
        }
        largs[cnt++] = e;
        skip_newlines(p);
        if (peek(p)->kind == TOK_COMMA) {
          advance(p);
          skip_newlines(p);
        } else {
          break;
        }
      }
    }
    skip_newlines(p);
    if (!expect(p, TOK_RPAREN)) { free(largs); p_leave_depth(p); return NULL; }
    if (cnt > 0) {
      ty->len_args = p_alloc(p, (size_t)cnt * sizeof *ty->len_args);
      if (ty->len_args == NULL) { free(largs); p_fail_oom(p); p_leave_depth(p); return NULL; }
      memcpy(ty->len_args, largs, (size_t)cnt * sizeof *largs);
      ty->len_arg_count = cnt;
    }
    free(largs);
  }

  /* Optional {regionName} */
  if (peek(p)->kind == TOK_LBRACE) {
    advance(p);
    skip_newlines(p);
    ty->cap_arg = parse_name(p);
    skip_newlines(p);
    if (!expect(p, TOK_RBRACE)) { p_leave_depth(p); return NULL; }
  }

  p_leave_depth(p);
  return ty;
}

/* ---- Fact groups & Rule literals ---- */

static StigiExpr *parse_fact_term(Parser *p) {
  if (p->failed || check_not_in_lang(p)) return NULL;
  const StigiTok *t = peek(p);
  StigiExpr *e = p_alloc(p, sizeof *e);
  if (e == NULL) { p_fail_oom(p); return NULL; }
  e->span = t->span;
  if (t->kind == TOK_MINUS) {
    advance(p);
    const StigiTok *nt = peek(p);
    if (nt->kind != TOK_INT) {
      p_fail(p, nt->span, "expected integer literal after '-' in fact term");
      return NULL;
    }
    advance(p);
    StigiExpr *lit = p_alloc(p, sizeof *lit);
    if (lit == NULL) { p_fail_oom(p); return NULL; }
    lit->kind = STIGI_EXPR_LIT_INT;
    lit->span = nt->span;
    lit->u.lit_int.raw = tok_str(nt);
    lit->u.lit_int.value = parse_u64_lit(nt->ptr, nt->len, &lit->u.lit_int.overflow);
    e->kind = STIGI_EXPR_UNOP;
    e->u.unop.op = STIGI_OP_NEG;
    e->u.unop.operand = lit;
    return e;
  }
  if (t->kind == TOK_INT) {
    advance(p);
    e->kind = STIGI_EXPR_LIT_INT;
    e->u.lit_int.raw = tok_str(t);
    e->u.lit_int.value = parse_u64_lit(t->ptr, t->len, &e->u.lit_int.overflow);
    return e;
  }
  if (t->kind == TOK_UNDERSCORE) {
    advance(p);
    e->kind = STIGI_EXPR_LOCAL;
    e->u.ident.name = tok_str(t);
    return e;
  }
  if (is_ident_like(t->kind)) {
    advance(p);
    bool has_dot = memchr(t->ptr, '.', t->len) != NULL;
    e->kind = has_dot ? STIGI_EXPR_GLOBAL : STIGI_EXPR_LOCAL;
    e->u.ident.name = tok_str(t);
    return e;
  }
  if (t->kind == TOK_LPAREN) {
    advance(p);
    skip_newlines(p);
    StigiExpr *inner = parse_expr(p, 0);
    skip_newlines(p);
    if (!expect(p, TOK_RPAREN)) return NULL;
    return inner;
  }
  p_fail(p, t->span, "expected fact term, got '%s'", stigi_tok_kind_name(t->kind));
  return NULL;
}

static bool is_cmp_tok(StigiTokKind k, StigiOp *out_op) {
  switch (k) {
    case TOK_LT: *out_op = STIGI_OP_LT; return true;
    case TOK_LE: *out_op = STIGI_OP_LE; return true;
    case TOK_GT: *out_op = STIGI_OP_GT; return true;
    case TOK_GE: *out_op = STIGI_OP_GE; return true;
    case TOK_EQ: *out_op = STIGI_OP_EQ; return true;
    case TOK_NE: *out_op = STIGI_OP_NE; return true;
    default: return false;
  }
}

static bool is_arith_tok(StigiTokKind k, StigiOp *out_op) {
  switch (k) {
    case TOK_PLUS: *out_op = STIGI_OP_ADD; return true;
    case TOK_MINUS: *out_op = STIGI_OP_SUB; return true;
    case TOK_STAR: *out_op = STIGI_OP_MUL; return true;
    case TOK_SLASH: *out_op = STIGI_OP_DIV; return true;
    default: return false;
  }
}

static StigiFactItem parse_fact_item(Parser *p, bool allow_evidence) {
  StigiFactItem item;
  memset(&item, 0, sizeof item);
  if (p->failed || check_not_in_lang(p)) return item;
  item.span = peek(p)->span;
  if (!p_enter_depth(p, item.span)) return item;

  if (peek(p)->kind == TOK_KW_NOT) {
    advance(p);
    item.is_negated = true;
    /* Multi-atom denial: not (atom1, atom2, ...) (facts.md 6) */
    if (peek(p)->kind == TOK_LPAREN) {
      advance(p);
      skip_newlines(p);
      item.kind = STIGI_FACT_DENY;
      StigiFactItem *atoms = NULL;
      uint32_t cnt = 0, cap = 0;
      if (peek(p)->kind == TOK_RPAREN) {
        p_fail(p, peek(p)->span, "denial 'not (...)' requires at least one atom");
        p_leave_depth(p);
        return item;
      }
      for (;;) {
        skip_newlines(p);
        StigiFactItem sub = parse_fact_item(p, false);
        if (p->failed) { free(atoms); p_leave_depth(p); return item; }
        if (sub.kind != STIGI_FACT_ATOM || sub.is_negated) {
          free(atoms);
          p_fail(p, sub.span, "denial 'not (...)' accepts only positive atoms");
          p_leave_depth(p);
          return item;
        }
        if (cnt == cap && !p_grow(p, (void **)&atoms, &cap, sizeof *atoms)) {
          free(atoms); p_leave_depth(p); return item;
        }
        atoms[cnt++] = sub;
        skip_newlines(p);
        if (peek(p)->kind == TOK_COMMA) {
          advance(p);
          skip_newlines(p);
        } else {
          break;
        }
      }
      skip_newlines(p);
      if (!expect(p, TOK_RPAREN)) { free(atoms); p_leave_depth(p); return item; }
      item.deny_atoms = p_alloc(p, (size_t)cnt * sizeof *atoms);
      if (item.deny_atoms == NULL) { free(atoms); p_fail_oom(p); p_leave_depth(p); return item; }
      memcpy(item.deny_atoms, atoms, (size_t)cnt * sizeof *atoms);
      item.deny_atom_count = cnt;
      free(atoms);
      p_leave_depth(p);
      return item;
    }
  }

  /* Check for evidence binder: `p: atom(...)` */
  if (allow_evidence && !item.is_negated && is_ident_like(peek(p)->kind) &&
      peek_next_non_comment(p)->kind == TOK_COLON) {
    item.evidence_name = tok_str(advance(p));
    advance(p); /* ':' */
  }

  /* Check for atom `Name(t1, t2)` vs comparison `t1 < t2` vs arith `x = t1 + t2` */
  if (is_ident_like(peek(p)->kind) && peek_next_non_comment(p)->kind == TOK_LPAREN) {
    item.kind = STIGI_FACT_ATOM;
    item.atom_name = tok_str(advance(p));
    advance(p); /* '(' */
    skip_newlines(p);
    StigiExpr **args = NULL;
    uint32_t cnt = 0, cap = 0;
    if (peek(p)->kind != TOK_RPAREN) {
      for (;;) {
        skip_newlines(p);
        StigiExpr *arg = parse_expr(p, 0);
        if (p->failed) { free(args); p_leave_depth(p); return item; }
        if (cnt == cap && !p_grow(p, (void **)&args, &cap, sizeof *args)) {
          free(args); p_leave_depth(p); return item;
        }
        args[cnt++] = arg;
        skip_newlines(p);
        if (peek(p)->kind == TOK_COMMA) {
          advance(p);
          skip_newlines(p);
        } else {
          break;
        }
      }
    }
    skip_newlines(p);
    if (!expect(p, TOK_RPAREN)) { free(args); p_leave_depth(p); return item; }
    if (cnt > 0) {
      item.args = p_alloc(p, (size_t)cnt * sizeof *item.args);
      if (item.args == NULL) { free(args); p_fail_oom(p); p_leave_depth(p); return item; }
      memcpy(item.args, args, (size_t)cnt * sizeof *args);
      item.arg_count = cnt;
    }
    free(args);
    p_leave_depth(p);
    return item;
  }

  if (item.is_negated) {
    p_fail(p, peek(p)->span, "expected atom or '(...)' after 'not'");
    p_leave_depth(p);
    return item;
  }

  if (is_ident_like(peek(p)->kind) && peek_next_non_comment(p)->kind == TOK_ASSIGN) {
    item.kind = STIGI_FACT_ARITH;
    item.lhs_var = tok_str(advance(p));
    advance(p); /* '=' */
    item.lhs = parse_fact_term(p);
    if (p->failed) { p_leave_depth(p); return item; }
    StigiOp op = STIGI_OP_ADD;
    if (!is_arith_tok(peek(p)->kind, &op)) {
      p_fail(p, peek(p)->span, "expected arithmetic operator in rule literal");
      p_leave_depth(p);
      return item;
    }
    advance(p);
    item.op = op;
    item.rhs = parse_fact_term(p);
    p_leave_depth(p);
    return item;
  }

  /* Comparison: term cmp term */
  StigiExpr *lhs = parse_fact_term(p);
  if (p->failed) { p_leave_depth(p); return item; }
  StigiOp cmp_op = STIGI_OP_EQ;
  if (!is_cmp_tok(peek(p)->kind, &cmp_op)) {
    p_fail(p, peek(p)->span, "expected atom or comparison in fact group");
    p_leave_depth(p);
    return item;
  }
  advance(p);
  item.kind = STIGI_FACT_CMP;
  item.lhs = lhs;
  item.op = cmp_op;
  item.rhs = parse_fact_term(p);
  p_leave_depth(p);
  return item;
}

static StigiFactGroup parse_fact_group(Parser *p) {
  StigiFactGroup fg;
  memset(&fg, 0, sizeof fg);
  if (p->failed) return fg;
  fg.span = peek(p)->span;
  if (!expect(p, TOK_LT)) return fg;
  skip_newlines(p);

  StigiFactItem *items = NULL;
  uint32_t cnt = 0, cap = 0;
  if (peek(p)->kind != TOK_GT) {
    for (;;) {
      skip_newlines(p);
      StigiFactItem it = parse_fact_item(p, true);
      if (p->failed) { free(items); return fg; }
      if (cnt == cap && !p_grow(p, (void **)&items, &cap, sizeof *items)) {
        free(items); return fg;
      }
      items[cnt++] = it;
      skip_newlines(p);
      if (peek(p)->kind == TOK_COMMA) {
        advance(p);
        skip_newlines(p);
      } else {
        break;
      }
    }
  }
  skip_newlines(p);
  if (!expect(p, TOK_GT)) { free(items); return fg; }
  if (cnt > 0) {
    fg.items = p_alloc(p, (size_t)cnt * sizeof *fg.items);
    if (fg.items == NULL) { free(items); p_fail_oom(p); return fg; }
    memcpy(fg.items, items, (size_t)cnt * sizeof *items);
    fg.count = cnt;
  }
  free(items);
  return fg;
}

/* ---- Parameter groups ---- */

static bool parse_param_list(Parser *p, StigiTokKind open_k, StigiTokKind close_k,
                             StigiParam **out_params, uint32_t *out_count) {
  if (!expect(p, open_k)) return false;
  skip_newlines(p);
  StigiParam *arr = NULL;
  uint32_t cnt = 0, cap = 0;
  if (peek(p)->kind != close_k) {
    for (;;) {
      skip_newlines(p);
      StigiParam pr;
      memset(&pr, 0, sizeof pr);
      pr.span = peek(p)->span;
      pr.name = parse_binder_name(p);
      if (p->failed) { free(arr); return false; }
      if (!expect(p, TOK_COLON)) { free(arr); return false; }
      pr.type = parse_type(p);
      if (p->failed) { free(arr); return false; }
      if (peek(p)->kind == TOK_STAR) {
        free(arr);
        p_fail_not_in_lang(p, peek(p)->span, "varargs");
        return false;
      }
      if (cnt == cap && !p_grow(p, (void **)&arr, &cap, sizeof *arr)) {
        free(arr); return false;
      }
      arr[cnt++] = pr;
      skip_newlines(p);
      if (peek(p)->kind == TOK_COMMA) {
        advance(p);
        skip_newlines(p);
      } else {
        break;
      }
    }
  }
  skip_newlines(p);
  if (!expect(p, close_k)) { free(arr); return false; }
  if (cnt > 0) {
    *out_params = p_alloc(p, (size_t)cnt * sizeof **out_params);
    if (*out_params == NULL) { free(arr); p_fail_oom(p); return false; }
    memcpy(*out_params, arr, (size_t)cnt * sizeof *arr);
    *out_count = cnt;
  } else {
    *out_params = NULL;
    *out_count = 0;
  }
  free(arr);
  return true;
}

static bool parse_cap_group(Parser *p, StigiCapParam **out_caps, uint32_t *out_count) {
  if (!expect(p, TOK_LBRACE)) return false;
  skip_newlines(p);
  StigiCapParam *arr = NULL;
  uint32_t cnt = 0, cap = 0;
  if (peek(p)->kind != TOK_RBRACE) {
    for (;;) {
      skip_newlines(p);
      StigiCapParam cp;
      memset(&cp, 0, sizeof cp);
      cp.span = peek(p)->span;
      if (is_ident_like(peek(p)->kind) && peek_next_non_comment(p)->kind == TOK_COLON) {
        cp.name = tok_str(advance(p));
        advance(p); /* ':' */
      }
      cp.cap_type = parse_type(p);
      if (p->failed) { free(arr); return false; }
      if (cnt == cap && !p_grow(p, (void **)&arr, &cap, sizeof *arr)) {
        free(arr); return false;
      }
      arr[cnt++] = cp;
      skip_newlines(p);
      if (peek(p)->kind == TOK_COMMA) {
        advance(p);
        skip_newlines(p);
      } else {
        break;
      }
    }
  }
  skip_newlines(p);
  if (!expect(p, TOK_RBRACE)) { free(arr); return false; }
  if (cnt > 0) {
    *out_caps = p_alloc(p, (size_t)cnt * sizeof **out_caps);
    if (*out_caps == NULL) { free(arr); p_fail_oom(p); return false; }
    memcpy(*out_caps, arr, (size_t)cnt * sizeof *arr);
    *out_count = cnt;
  } else {
    *out_caps = NULL;
    *out_count = 0;
  }
  free(arr);
  return true;
}

/* ---- Patterns ---- */

static StigiPat parse_pattern(Parser *p) {
  StigiPat pat;
  memset(&pat, 0, sizeof pat);
  if (p->failed || check_not_in_lang(p)) return pat;
  const StigiTok *t = peek(p);
  pat.span = t->span;

  if (t->kind == TOK_UNDERSCORE) {
    advance(p);
    pat.kind = STIGI_PAT_WILD;
    pat.name = tok_str(t);
    return pat;
  }
  if (t->kind == TOK_INT) {
    advance(p);
    pat.kind = STIGI_PAT_INT;
    pat.name = tok_str(t);
    pat.int_val = parse_u64_lit(t->ptr, t->len, &pat.int_overflow);
    return pat;
  }
  if (t->kind == TOK_KW_TRUE || t->kind == TOK_KW_FALSE) {
    advance(p);
    pat.kind = STIGI_PAT_BOOL;
    pat.name = tok_str(t);
    pat.bool_val = (t->kind == TOK_KW_TRUE);
    return pat;
  }
  if (is_ident_like(t->kind)) {
    advance(p);
    pat.name = tok_str(t);
    bool is_upper_start = (t->len > 0 && t->ptr[0] >= 'A' && t->ptr[0] <= 'Z');
    if (peek(p)->kind == TOK_LPAREN) {
      pat.kind = STIGI_PAT_CTOR;
      pat.has_parens = true;
      advance(p);
      skip_newlines(p);
      StigiStr *fields = NULL;
      uint32_t cnt = 0, cap = 0;
      if (peek(p)->kind != TOK_RPAREN) {
        for (;;) {
          skip_newlines(p);
          StigiStr fn = parse_binder_name(p);
          if (p->failed) { free(fields); return pat; }
          if (cnt == cap && !p_grow(p, (void **)&fields, &cap, sizeof *fields)) {
            free(fields); return pat;
          }
          fields[cnt++] = fn;
          skip_newlines(p);
          if (peek(p)->kind == TOK_COMMA) {
            advance(p);
            skip_newlines(p);
          } else {
            break;
          }
        }
      }
      skip_newlines(p);
      if (!expect(p, TOK_RPAREN)) { free(fields); return pat; }
      if (cnt > 0) {
        pat.fields = p_alloc(p, (size_t)cnt * sizeof *pat.fields);
        if (pat.fields == NULL) { free(fields); p_fail_oom(p); return pat; }
        memcpy(pat.fields, fields, (size_t)cnt * sizeof *fields);
        pat.field_count = cnt;
      }
      free(fields);
      return pat;
    }
    pat.kind = is_upper_start ? STIGI_PAT_CTOR : STIGI_PAT_VAR;
    return pat;
  }

  p_fail(p, t->span, "expected pattern, got '%s'", stigi_tok_kind_name(t->kind));
  return pat;
}

/* ---- Expressions ---- */

static StigiExpr *parse_primary(Parser *p, uint32_t base_col) {
  if (p->failed || check_not_in_lang(p)) return NULL;
  const StigiTok *t = peek(p);
  if (!p_enter_depth(p, t->span)) return NULL;

  if (t->kind == TOK_KW_FOR) {
    p_fail_not_in_lang(p, t->span, "for");
    p_leave_depth(p);
    return NULL;
  }

  if (t->kind == TOK_INT) {
    advance(p);
    StigiExpr *e = p_alloc(p, sizeof *e);
    if (e == NULL) { p_fail_oom(p); p_leave_depth(p); return NULL; }
    e->kind = STIGI_EXPR_LIT_INT;
    e->span = t->span;
    e->u.lit_int.raw = tok_str(t);
    e->u.lit_int.value = parse_u64_lit(t->ptr, t->len, &e->u.lit_int.overflow);
    p_leave_depth(p);
    return e;
  }
  if (t->kind == TOK_CHAR) {
    advance(p);
    StigiExpr *e = p_alloc(p, sizeof *e);
    if (e == NULL) { p_fail_oom(p); p_leave_depth(p); return NULL; }
    e->kind = STIGI_EXPR_LIT_CHAR;
    e->span = t->span;
    e->u.lit_char.raw = tok_str(t);
    e->u.lit_char.value = parse_char_lit(t->ptr, t->len);
    p_leave_depth(p);
    return e;
  }
  if (t->kind == TOK_STRING) {
    advance(p);
    StigiExpr *e = p_alloc(p, sizeof *e);
    if (e == NULL) { p_fail_oom(p); p_leave_depth(p); return NULL; }
    e->kind = STIGI_EXPR_LIT_STR;
    e->span = t->span;
    e->u.lit_str.raw = tok_str(t);
    e->u.lit_str.decoded_len = decoded_str_len(t->ptr, t->len);
    p_leave_depth(p);
    return e;
  }
  if (t->kind == TOK_KW_TRUE || t->kind == TOK_KW_FALSE) {
    advance(p);
    StigiExpr *e = p_alloc(p, sizeof *e);
    if (e == NULL) { p_fail_oom(p); p_leave_depth(p); return NULL; }
    e->kind = STIGI_EXPR_LIT_BOOL;
    e->span = t->span;
    e->u.lit_bool.value = (t->kind == TOK_KW_TRUE);
    p_leave_depth(p);
    return e;
  }
  if (is_ident_like(t->kind)) {
    advance(p);
    StigiExpr *e = p_alloc(p, sizeof *e);
    if (e == NULL) { p_fail_oom(p); p_leave_depth(p); return NULL; }
    bool has_dot = memchr(t->ptr, '.', t->len) != NULL;
    e->kind = has_dot ? STIGI_EXPR_GLOBAL : STIGI_EXPR_LOCAL;
    e->span = t->span;
    e->u.ident.name = tok_str(t);
    p_leave_depth(p);
    return e;
  }
  if (t->kind == TOK_LPAREN) {
    advance(p);
    skip_newlines(p);
    if (peek(p)->kind == TOK_RPAREN) {
      advance(p);
      if (peek(p)->kind == TOK_ARROW) {
        p_fail_not_in_lang(p, peek(p)->span, "lambda");
        p_leave_depth(p);
        return NULL;
      }
      StigiExpr *e = p_alloc(p, sizeof *e);
      if (e == NULL) { p_fail_oom(p); p_leave_depth(p); return NULL; }
      e->kind = STIGI_EXPR_LIT_UNIT;
      e->span = t->span;
      p_leave_depth(p);
      return e;
    }
    /* Detect Scala/Rust lambda `(x: T) => ...` */
    if ((is_ident_like(peek(p)->kind) || peek(p)->kind == TOK_UNDERSCORE) &&
        peek_next_non_comment(p)->kind == TOK_COLON) {
      p_fail_not_in_lang(p, t->span, "lambda");
      p_leave_depth(p);
      return NULL;
    }
    StigiExpr *inner = parse_expr(p, base_col);
    skip_newlines(p);
    if (!p->failed && peek(p)->kind == TOK_COMMA) {
      p_fail_not_in_lang(p, peek(p)->span, "lambda");
      p_leave_depth(p);
      return NULL;
    }
    if (!expect(p, TOK_RPAREN)) { p_leave_depth(p); return NULL; }
    if (peek(p)->kind == TOK_ARROW) {
      p_fail_not_in_lang(p, peek(p)->span, "lambda");
      p_leave_depth(p);
      return NULL;
    }
    p_leave_depth(p);
    return inner;
  }

  p_fail(p, t->span, "expected expression, got '%s'", stigi_tok_kind_name(t->kind));
  p_leave_depth(p);
  return NULL;
}

static StigiExpr *parse_postfix(Parser *p, uint32_t base_col) {
  StigiExpr *e = parse_primary(p, base_col);
  if (p->failed) return NULL;

  for (;;) {
    const StigiTok *t = peek(p);
    /* Call or generic application: [typeArgs] {capArgs} (args), where (...) is optional when [...] or {...} is present */
    if (t->kind == TOK_LBRACKET || t->kind == TOK_LBRACE || t->kind == TOK_LPAREN) {
      StigiExpr *call = p_alloc(p, sizeof *call);
      if (call == NULL) { p_fail_oom(p); return NULL; }
      call->kind = STIGI_EXPR_CALL;
      call->span = e->span;
      call->u.call.callee = e;

      if (peek(p)->kind == TOK_LBRACKET) {
        advance(p);
        skip_newlines(p);
        StigiType **targs = NULL;
        uint32_t cnt = 0, cap = 0;
        if (peek(p)->kind != TOK_RBRACKET) {
          for (;;) {
            skip_newlines(p);
            StigiType *ta = parse_type(p);
            if (p->failed) { free(targs); return NULL; }
            if (cnt == cap && !p_grow(p, (void **)&targs, &cap, sizeof *targs)) {
              free(targs); return NULL;
            }
            targs[cnt++] = ta;
            skip_newlines(p);
            if (peek(p)->kind == TOK_COMMA) { advance(p); skip_newlines(p); }
            else break;
          }
        }
        skip_newlines(p);
        if (!expect(p, TOK_RBRACKET)) { free(targs); return NULL; }
        if (cnt > 0) {
          call->u.call.type_args = p_alloc(p, (size_t)cnt * sizeof *targs);
          if (call->u.call.type_args == NULL) { free(targs); p_fail_oom(p); return NULL; }
          memcpy(call->u.call.type_args, targs, (size_t)cnt * sizeof *targs);
          call->u.call.type_arg_count = cnt;
        }
        free(targs);
      }

      if (peek(p)->kind == TOK_LBRACE) {
        advance(p);
        skip_newlines(p);
        StigiStr *cargs = NULL;
        uint32_t cnt = 0, cap = 0;
        if (peek(p)->kind != TOK_RBRACE) {
          for (;;) {
            skip_newlines(p);
            StigiStr cn = parse_name(p);
            if (p->failed) { free(cargs); return NULL; }
            if (cnt == cap && !p_grow(p, (void **)&cargs, &cap, sizeof *cargs)) {
              free(cargs); return NULL;
            }
            cargs[cnt++] = cn;
            skip_newlines(p);
            if (peek(p)->kind == TOK_COMMA) { advance(p); skip_newlines(p); }
            else break;
          }
        }
        skip_newlines(p);
        if (!expect(p, TOK_RBRACE)) { free(cargs); return NULL; }
        if (cnt > 0) {
          call->u.call.cap_args = p_alloc(p, (size_t)cnt * sizeof *cargs);
          if (call->u.call.cap_args == NULL) { free(cargs); p_fail_oom(p); return NULL; }
          memcpy(call->u.call.cap_args, cargs, (size_t)cnt * sizeof *cargs);
          call->u.call.cap_arg_count = cnt;
        }
        free(cargs);
      }

      if (peek(p)->kind == TOK_LPAREN) {
        call->u.call.has_parens = true;
        advance(p);
        skip_newlines(p);
        StigiExpr **args = NULL;
        uint32_t cnt = 0, cap = 0;
        if (peek(p)->kind != TOK_RPAREN) {
          for (;;) {
            skip_newlines(p);
            StigiExpr *a = parse_expr(p, base_col);
            if (p->failed) { free(args); return NULL; }
            if (cnt == cap && !p_grow(p, (void **)&args, &cap, sizeof *args)) {
              free(args); return NULL;
            }
            args[cnt++] = a;
            skip_newlines(p);
            if (peek(p)->kind == TOK_COMMA) { advance(p); skip_newlines(p); }
            else break;
          }
        }
        skip_newlines(p);
        if (!expect(p, TOK_RPAREN)) { free(args); return NULL; }
        if (cnt > 0) {
          call->u.call.args = p_alloc(p, (size_t)cnt * sizeof *args);
          if (call->u.call.args == NULL) { free(args); p_fail_oom(p); return NULL; }
          memcpy(call->u.call.args, args, (size_t)cnt * sizeof *args);
          call->u.call.arg_count = cnt;
        }
        free(args);
      }
      e = call;
      continue;
    }

    /* expr@<facts> */
    if (t->kind == TOK_AT) {
      advance(p);
      StigiFactGroup fg = parse_fact_group(p);
      if (p->failed) return NULL;
      StigiExpr *asrt = p_alloc(p, sizeof *asrt);
      if (asrt == NULL) { p_fail_oom(p); return NULL; }
      asrt->kind = STIGI_EXPR_ASSERT;
      asrt->span = e->span;
      asrt->u.assert_expr.expr = e;
      asrt->u.assert_expr.facts = fg;
      e = asrt;
      continue;
    }

    break;
  }

  return e;
}

static StigiExpr *parse_unary(Parser *p, uint32_t base_col) {
  if (p->failed || check_not_in_lang(p)) return NULL;
  const StigiTok *t = peek(p);
  if (t->kind == TOK_BANG || t->kind == TOK_MINUS) {
    if (!p_enter_depth(p, t->span)) return NULL;
    advance(p);
    StigiExpr *opnd = parse_unary(p, base_col);
    p_leave_depth(p);
    if (p->failed) return NULL;
    StigiExpr *u = p_alloc(p, sizeof *u);
    if (u == NULL) { p_fail_oom(p); return NULL; }
    u->kind = STIGI_EXPR_UNOP;
    u->span = t->span;
    u->u.unop.op = (t->kind == TOK_BANG) ? STIGI_OP_NOT : STIGI_OP_NEG;
    u->u.unop.operand = opnd;
    return u;
  }
  return parse_postfix(p, base_col);
}

static int binop_prec(StigiTokKind k, StigiOp *out_op) {
  switch (k) {
    case TOK_OR: *out_op = STIGI_OP_OR; return 1;
    case TOK_AND: *out_op = STIGI_OP_AND; return 2;
    case TOK_EQ: *out_op = STIGI_OP_EQ; return 3;
    case TOK_NE: *out_op = STIGI_OP_NE; return 3;
    case TOK_LT: *out_op = STIGI_OP_LT; return 3;
    case TOK_LE: *out_op = STIGI_OP_LE; return 3;
    case TOK_GT: *out_op = STIGI_OP_GT; return 3;
    case TOK_GE: *out_op = STIGI_OP_GE; return 3;
    case TOK_PLUS: *out_op = STIGI_OP_ADD; return 4;
    case TOK_MINUS: *out_op = STIGI_OP_SUB; return 4;
    case TOK_STAR: *out_op = STIGI_OP_MUL; return 5;
    case TOK_SLASH: *out_op = STIGI_OP_DIV; return 5;
    case TOK_PERCENT: *out_op = STIGI_OP_MOD; return 5;
    default: return 0;
  }
}

static StigiExpr *parse_binop(Parser *p, uint32_t base_col, int min_prec) {
  StigiExpr *lhs = parse_unary(p, base_col);
  if (p->failed) return NULL;

  for (;;) {
    StigiOp op = STIGI_OP_ADD;
    int prec = binop_prec(peek(p)->kind, &op);
    if (prec < min_prec) break;
    advance(p);
    if (peek(p)->kind == TOK_NEWLINE && peek_skip_nl(p)->span.col > base_col) {
      skip_newlines(p);
    }
    StigiExpr *rhs = parse_binop(p, base_col, prec + 1);
    if (p->failed) return NULL;
    StigiExpr *b = p_alloc(p, sizeof *b);
    if (b == NULL) { p_fail_oom(p); return NULL; }
    b->kind = STIGI_EXPR_BINOP;
    b->span = lhs->span;
    b->u.binop.op = op;
    b->u.binop.lhs = lhs;
    b->u.binop.rhs = rhs;
    lhs = b;
  }
  return lhs;
}

static StigiExpr *parse_match(Parser *p, uint32_t base_col) {
  StigiExpr *e = parse_binop(p, base_col, 1);
  if (p->failed) return NULL;

  while (peek(p)->kind == TOK_KW_MATCH) {
    advance(p);
    StigiExpr *m = p_alloc(p, sizeof *m);
    if (m == NULL) { p_fail_oom(p); return NULL; }
    m->kind = STIGI_EXPR_MATCH;
    m->span = e->span;
    m->u.match_expr.scrutinee = e;

    skip_newlines(p);
    const StigiTok *first_arm = peek(p);
    if (first_arm->kind != TOK_KW_CASE) {
      p_fail(p, first_arm->span, "expected 'case' arm after 'match'");
      return NULL;
    }
    uint32_t arm_col = first_arm->span.col;
    StigiMatchArm *arms = NULL;
    uint32_t cnt = 0, cap = 0;

    for (;;) {
      const StigiTok *nxt = peek_skip_nl(p);
      if (nxt->kind != TOK_KW_CASE || nxt->span.col != arm_col) break;
      skip_newlines(p);
      StigiMatchArm arm;
      memset(&arm, 0, sizeof arm);
      arm.span = advance(p)->span; /* 'case' */
      arm.pat = parse_pattern(p);
      if (p->failed) { free(arms); return NULL; }
      if (!expect(p, TOK_ARROW)) { free(arms); return NULL; }
      arm.body = parse_block_or_expr(p, arm_col);
      if (p->failed) { free(arms); return NULL; }
      if (cnt == cap && !p_grow(p, (void **)&arms, &cap, sizeof *arms)) {
        free(arms); return NULL;
      }
      arms[cnt++] = arm;
    }
    if (cnt > 0) {
      m->u.match_expr.arms = p_alloc(p, (size_t)cnt * sizeof *arms);
      if (m->u.match_expr.arms == NULL) { free(arms); p_fail_oom(p); return NULL; }
      memcpy(m->u.match_expr.arms, arms, (size_t)cnt * sizeof *arms);
      m->u.match_expr.arm_count = cnt;
    }
    free(arms);
    e = m;
  }
  return e;
}

static StigiExpr *parse_expr(Parser *p, uint32_t base_col) {
  if (p->failed || check_not_in_lang(p)) return NULL;
  const StigiTok *t = peek(p);
  if (!p_enter_depth(p, t->span)) return NULL;

  if (t->kind == TOK_KW_IF) {
    uint32_t if_col = t->span.col;
    uint32_t blk_col = (base_col < if_col) ? base_col : if_col;
    advance(p);
    StigiExpr *cond = parse_expr(p, blk_col);
    if (peek(p)->kind == TOK_NEWLINE && peek_skip_nl(p)->kind == TOK_KW_THEN) {
      skip_newlines(p);
    }
    if (!expect(p, TOK_KW_THEN)) { p_leave_depth(p); return NULL; }
    StigiExpr *then_br = parse_block_or_expr(p, blk_col);
    if (p->failed) { p_leave_depth(p); return NULL; }
    if (peek(p)->kind == TOK_NEWLINE && peek_skip_nl(p)->kind == TOK_KW_ELSE &&
        (peek_skip_nl(p)->span.col == if_col || peek_skip_nl(p)->span.col == base_col)) {
      skip_newlines(p);
    }
    if (!expect(p, TOK_KW_ELSE)) { p_leave_depth(p); return NULL; }
    StigiExpr *else_br = parse_block_or_expr(p, blk_col);
    if (p->failed) { p_leave_depth(p); return NULL; }
    StigiExpr *ife = p_alloc(p, sizeof *ife);
    if (ife == NULL) { p_fail_oom(p); p_leave_depth(p); return NULL; }
    ife->kind = STIGI_EXPR_IF;
    ife->span = t->span;
    ife->u.if_expr.cond = cond;
    ife->u.if_expr.then_br = then_br;
    ife->u.if_expr.else_br = else_br;
    p_leave_depth(p);
    return ife;
  }

  if (t->kind == TOK_KW_WHILE) {
    uint32_t w_col = t->span.col;
    uint32_t blk_col = (base_col < w_col) ? base_col : w_col;
    advance(p);
    StigiExpr *cond = parse_expr(p, blk_col);
    if (p->failed) { p_leave_depth(p); return NULL; }
    if (peek(p)->kind == TOK_NEWLINE && peek_skip_nl(p)->kind == TOK_KW_DO) {
      skip_newlines(p);
    }
    if (!expect(p, TOK_KW_DO)) { p_leave_depth(p); return NULL; }
    StigiExpr *body = parse_block_or_expr(p, blk_col);
    if (p->failed) { p_leave_depth(p); return NULL; }
    StigiExpr *wh = p_alloc(p, sizeof *wh);
    if (wh == NULL) { p_fail_oom(p); p_leave_depth(p); return NULL; }
    wh->kind = STIGI_EXPR_WHILE;
    wh->span = t->span;
    wh->u.while_expr.cond = cond;
    wh->u.while_expr.decreases = NULL;
    wh->u.while_expr.body = body;
    p_leave_depth(p);
    return wh;
  }

  StigiExpr *res = parse_match(p, base_col);
  if (!p->failed && peek(p)->kind == TOK_ARROW) {
    p_fail_not_in_lang(p, peek(p)->span, "lambda");
    p_leave_depth(p);
    return NULL;
  }
  p_leave_depth(p);
  return res;
}

static StigiExpr *parse_stmt(Parser *p, uint32_t stmt_col) {
  if (p->failed || check_not_in_lang(p)) return NULL;
  const StigiTok *t = peek(p);

  if (t->kind == TOK_KW_FOR) {
    p_fail_not_in_lang(p, t->span, "for");
    return NULL;
  }

  /* Local declarations inside NegTest blocks */
  if (t->kind == TOK_KW_DEF || t->kind == TOK_KW_LEMMA || t->kind == TOK_KW_ENUM ||
      t->kind == TOK_KW_TYPE || t->kind == TOK_KW_CAPABILITY || t->kind == TOK_KW_HANDLER ||
      t->kind == TOK_KW_EXTERN || t->kind == TOK_KW_RELATION || t->kind == TOK_KW_RULE) {
    StigiDecl *d = parse_decl(p, false, stmt_col);
    if (p->failed) return NULL;
    StigiExpr *ld = p_alloc(p, sizeof *ld);
    if (ld == NULL) { p_fail_oom(p); return NULL; }
    ld->kind = STIGI_EXPR_LOCAL_DECL;
    ld->span = t->span;
    ld->u.local_decl.decl = d;
    return ld;
  }

  /* val pattern [: Type] = expr */
  if (t->kind == TOK_KW_VAL) {
    advance(p);
    StigiExpr *vs = p_alloc(p, sizeof *vs);
    if (vs == NULL) { p_fail_oom(p); return NULL; }
    vs->kind = STIGI_EXPR_VAL;
    vs->span = t->span;
    vs->u.val_stmt.pat = parse_pattern(p);
    if (p->failed) return NULL;
    if (peek(p)->kind == TOK_COLON) {
      advance(p);
      vs->u.val_stmt.type = parse_type(p);
      if (p->failed) return NULL;
    }
    if (!expect(p, TOK_ASSIGN)) return NULL;
    vs->u.val_stmt.init = parse_block_or_expr(p, stmt_col);
    return vs;
  }

  /* var name [@<facts>] [: Type] = expr */
  if (t->kind == TOK_KW_VAR) {
    advance(p);
    StigiExpr *vs = p_alloc(p, sizeof *vs);
    if (vs == NULL) { p_fail_oom(p); return NULL; }
    vs->kind = STIGI_EXPR_VAR;
    vs->span = t->span;
    vs->u.var_stmt.name = parse_name(p);
    if (p->failed) return NULL;
    if (peek(p)->kind == TOK_AT) {
      advance(p);
      vs->u.var_stmt.has_inv = true;
      vs->u.var_stmt.inv = parse_fact_group(p);
      if (p->failed) return NULL;
    }
    if (peek(p)->kind == TOK_COLON) {
      advance(p);
      vs->u.var_stmt.type = parse_type(p);
      if (p->failed) return NULL;
    }
    if (!expect(p, TOK_ASSIGN)) return NULL;
    vs->u.var_stmt.init = parse_block_or_expr(p, stmt_col);
    return vs;
  }

  /* handle [name =] Handler [typeArgs] {capArgs} [(args)] [: block] */
  if (t->kind == TOK_KW_HANDLE) {
    advance(p);
    StigiExpr *hs = p_alloc(p, sizeof *hs);
    if (hs == NULL) { p_fail_oom(p); return NULL; }
    hs->kind = STIGI_EXPR_HANDLE;
    hs->span = t->span;
    if (is_ident_like(peek(p)->kind) && peek_next_non_comment(p)->kind == TOK_ASSIGN) {
      hs->u.handle_stmt.binder = tok_str(advance(p));
      advance(p); /* '=' */
    }
    hs->u.handle_stmt.handler_name = parse_name(p);
    if (p->failed) return NULL;
    if (peek(p)->kind == TOK_LBRACKET) {
      advance(p);
      skip_newlines(p);
      StigiType **targs = NULL;
      uint32_t cnt = 0, cap = 0;
      if (peek(p)->kind != TOK_RBRACKET) {
        for (;;) {
          skip_newlines(p);
          StigiType *ta = parse_type(p);
          if (p->failed) { free(targs); return NULL; }
          if (cnt == cap && !p_grow(p, (void **)&targs, &cap, sizeof *targs)) {
            free(targs); return NULL;
          }
          targs[cnt++] = ta;
          skip_newlines(p);
          if (peek(p)->kind == TOK_COMMA) { advance(p); skip_newlines(p); }
          else break;
        }
      }
      skip_newlines(p);
      if (!expect(p, TOK_RBRACKET)) { free(targs); return NULL; }
      if (cnt > 0) {
        hs->u.handle_stmt.type_args = p_alloc(p, (size_t)cnt * sizeof *targs);
        if (hs->u.handle_stmt.type_args == NULL) { free(targs); p_fail_oom(p); return NULL; }
        memcpy(hs->u.handle_stmt.type_args, targs, (size_t)cnt * sizeof *targs);
        hs->u.handle_stmt.type_arg_count = cnt;
      }
      free(targs);
    }
    if (peek(p)->kind == TOK_LBRACE) {
      advance(p);
      skip_newlines(p);
      StigiStr *cargs = NULL;
      uint32_t cnt = 0, cap = 0;
      if (peek(p)->kind != TOK_RBRACE) {
        for (;;) {
          skip_newlines(p);
          StigiStr cn = parse_name(p);
          if (p->failed) { free(cargs); return NULL; }
          if (cnt == cap && !p_grow(p, (void **)&cargs, &cap, sizeof *cargs)) {
            free(cargs); return NULL;
          }
          cargs[cnt++] = cn;
          skip_newlines(p);
          if (peek(p)->kind == TOK_COMMA) { advance(p); skip_newlines(p); }
          else break;
        }
      }
      skip_newlines(p);
      if (!expect(p, TOK_RBRACE)) { free(cargs); return NULL; }
      if (cnt > 0) {
        hs->u.handle_stmt.cap_args = p_alloc(p, (size_t)cnt * sizeof *cargs);
        if (hs->u.handle_stmt.cap_args == NULL) { free(cargs); p_fail_oom(p); return NULL; }
        memcpy(hs->u.handle_stmt.cap_args, cargs, (size_t)cnt * sizeof *cargs);
        hs->u.handle_stmt.cap_arg_count = cnt;
      }
      free(cargs);
    }
    if (peek(p)->kind == TOK_LPAREN) {
      hs->u.handle_stmt.has_parens = true;
      advance(p);
      skip_newlines(p);
      StigiExpr **args = NULL;
      uint32_t cnt = 0, cap = 0;
      if (peek(p)->kind != TOK_RPAREN) {
        for (;;) {
          skip_newlines(p);
          StigiExpr *a = parse_expr(p, stmt_col);
          if (p->failed) { free(args); return NULL; }
          if (cnt == cap && !p_grow(p, (void **)&args, &cap, sizeof *args)) {
            free(args); return NULL;
          }
          args[cnt++] = a;
          skip_newlines(p);
          if (peek(p)->kind == TOK_COMMA) { advance(p); skip_newlines(p); }
          else break;
        }
      }
      skip_newlines(p);
      if (!expect(p, TOK_RPAREN)) { free(args); return NULL; }
      if (cnt > 0) {
        hs->u.handle_stmt.args = p_alloc(p, (size_t)cnt * sizeof *args);
        if (hs->u.handle_stmt.args == NULL) { free(args); p_fail_oom(p); return NULL; }
        memcpy(hs->u.handle_stmt.args, args, (size_t)cnt * sizeof *args);
        hs->u.handle_stmt.arg_count = cnt;
      }
      free(args);
    }
    if (peek(p)->kind == TOK_COLON) {
      advance(p);
      hs->u.handle_stmt.body = parse_block_or_expr(p, stmt_col);
    }
    return hs;
  }

  /* Bare @<facts> */
  if (t->kind == TOK_AT) {
    advance(p);
    StigiFactGroup fg = parse_fact_group(p);
    if (p->failed) return NULL;
    StigiExpr *asrt = p_alloc(p, sizeof *asrt);
    if (asrt == NULL) { p_fail_oom(p); return NULL; }
    asrt->kind = STIGI_EXPR_ASSERT;
    asrt->span = t->span;
    asrt->u.assert_expr.expr = NULL;
    asrt->u.assert_expr.facts = fg;
    return asrt;
  }

  /* Assignment: name = expr */
  if (is_ident_like(t->kind) && peek_next_non_comment(p)->kind == TOK_ASSIGN) {
    StigiStr name = tok_str(advance(p));
    advance(p); /* '=' */
    StigiExpr *val = parse_block_or_expr(p, stmt_col);
    if (p->failed) return NULL;
    StigiExpr *as = p_alloc(p, sizeof *as);
    if (as == NULL) { p_fail_oom(p); return NULL; }
    as->kind = STIGI_EXPR_ASSIGN;
    as->span = t->span;
    as->u.assign_stmt.name = name;
    as->u.assign_stmt.value = val;
    return as;
  }

  /* General expression, or index write `expr(idx) := val` */
  StigiExpr *e = parse_expr(p, stmt_col);
  if (p->failed) return NULL;
  if (peek(p)->kind == TOK_COLON_EQ) {
    if (e->kind != STIGI_EXPR_CALL || !e->u.call.has_parens || e->u.call.arg_count != 1 ||
        e->u.call.type_arg_count != 0 || e->u.call.cap_arg_count != 0) {
      p_fail(p, peek(p)->span, "':=' requires an indexed element target 'arr(idx)'");
      return NULL;
    }
    advance(p); /* ':=' */
    StigiExpr *val = parse_block_or_expr(p, stmt_col);
    if (p->failed) return NULL;
    StigiExpr *iw = p_alloc(p, sizeof *iw);
    if (iw == NULL) { p_fail_oom(p); return NULL; }
    iw->kind = STIGI_EXPR_INDEX_WRITE;
    iw->span = e->span;
    iw->u.index_expr.target = e->u.call.callee;
    iw->u.index_expr.index = e->u.call.args[0];
    iw->u.index_expr.value = val;
    return iw;
  }
  return e;
}

static StigiExpr *parse_block_or_expr(Parser *p, uint32_t base_col) {
  if (p->failed) return NULL;
  if (!p_enter_depth(p, peek(p)->span)) return NULL;
  if (peek(p)->kind != TOK_NEWLINE) {
    StigiExpr *s = parse_stmt(p, base_col);
    p_leave_depth(p);
    return s;
  }
  const StigiTok *first = peek_skip_nl(p);
  if (first->kind == TOK_EOF || first->span.col <= base_col) {
    p_fail(p, first->span, "expected indented block");
    p_leave_depth(p);
    return NULL;
  }
  skip_newlines(p);
  uint32_t block_col = peek(p)->span.col;

  StigiExpr **stmts = NULL;
  uint32_t cnt = 0, cap = 0;

  for (;;) {
    const StigiTok *nxt = peek_skip_nl(p);
    if (nxt->kind == TOK_EOF || nxt->span.col < block_col) break;
    if (nxt->span.col > block_col) {
      p_fail(p, nxt->span, "unexpected indentation");
      free(stmts);
      p_leave_depth(p);
      return NULL;
    }
    skip_newlines(p);
    StigiExpr *s = parse_stmt(p, block_col);
    if (p->failed) { free(stmts); p_leave_depth(p); return NULL; }
    if (cnt == cap && !p_grow(p, (void **)&stmts, &cap, sizeof *stmts)) {
      free(stmts); p_leave_depth(p); return NULL;
    }
    stmts[cnt++] = s;
  }

  StigiExpr *blk = p_alloc(p, sizeof *blk);
  if (blk == NULL) { free(stmts); p_fail_oom(p); p_leave_depth(p); return NULL; }
  blk->kind = STIGI_EXPR_BLOCK;
  blk->span = cnt > 0 ? stmts[0]->span : first->span;
  if (cnt > 0) {
    blk->u.block.stmts = p_alloc(p, (size_t)cnt * sizeof *stmts);
    if (blk->u.block.stmts == NULL) { free(stmts); p_fail_oom(p); p_leave_depth(p); return NULL; }
    memcpy(blk->u.block.stmts, stmts, (size_t)cnt * sizeof *stmts);
    blk->u.block.count = cnt;
  }
  free(stmts);
  p_leave_depth(p);
  return blk;
}

/* ---- Declarations ---- */

static bool parse_def_head(Parser *p, bool is_lemma, uint32_t decl_col, StigiDefHead *h) {
  memset(h, 0, sizeof *h);

  skip_head_cont_nl(p, decl_col);
  /* Optional [typeGroup] */
  if (peek(p)->kind == TOK_LBRACKET) {
    if (!parse_param_list(p, TOK_LBRACKET, TOK_RBRACKET, &h->type_params, &h->type_param_count)) {
      return false;
    }
  }

  /* Zero or more {capGroup} or <factGroup> before (valGroup) */
  StigiHeadGroup *pre = NULL;
  uint32_t pre_cnt = 0, pre_cap = 0;
  for (;;) {
    skip_head_cont_nl(p, decl_col);
    if (peek(p)->kind != TOK_LBRACE && peek(p)->kind != TOK_LT) break;
    StigiHeadGroup g;
    memset(&g, 0, sizeof g);
    g.span = peek(p)->span;
    if (peek(p)->kind == TOK_LBRACE) {
      g.is_fact = false;
      if (!parse_cap_group(p, &g.caps, &g.cap_count)) { free(pre); return false; }
    } else {
      g.is_fact = true;
      g.facts = parse_fact_group(p);
      if (p->failed) { free(pre); return false; }
    }
    if (pre_cnt == pre_cap && !p_grow(p, (void **)&pre, &pre_cap, sizeof *pre)) {
      free(pre); return false;
    }
    pre[pre_cnt++] = g;
  }
  if (pre_cnt > 0) {
    h->pre_groups = p_alloc(p, (size_t)pre_cnt * sizeof *pre);
    if (h->pre_groups == NULL) { free(pre); p_fail_oom(p); return false; }
    memcpy(h->pre_groups, pre, (size_t)pre_cnt * sizeof *pre);
    h->pre_group_count = pre_cnt;
  }
  free(pre);

  skip_head_cont_nl(p, decl_col);
  /* Value parameter group (...) */
  if (peek(p)->kind == TOK_LPAREN) {
    h->has_val_group = true;
    if (!parse_param_list(p, TOK_LPAREN, TOK_RPAREN, &h->params, &h->param_count)) {
      return false;
    }
  }

  /* Zero or more <factGroup> after (valGroup), before ':' */
  StigiFactGroup *post_val = NULL;
  uint32_t pv_cnt = 0, pv_cap = 0;
  for (;;) {
    skip_head_cont_nl(p, decl_col);
    if (peek(p)->kind != TOK_LT) break;
    StigiFactGroup fg = parse_fact_group(p);
    if (p->failed) { free(post_val); return false; }
    if (pv_cnt == pv_cap && !p_grow(p, (void **)&post_val, &pv_cap, sizeof *post_val)) {
      free(post_val); return false;
    }
    post_val[pv_cnt++] = fg;
  }
  if (pv_cnt > 0) {
    h->post_val_facts = p_alloc(p, (size_t)pv_cnt * sizeof *post_val);
    if (h->post_val_facts == NULL) { free(post_val); p_fail_oom(p); return false; }
    memcpy(h->post_val_facts, post_val, (size_t)pv_cnt * sizeof *post_val);
    h->post_val_fact_count = pv_cnt;
  }
  free(post_val);

  skip_head_cont_nl(p, decl_col);
  if (!expect(p, TOK_COLON)) return false;

  if (!is_lemma) {
    h->ret_type = parse_type(p);
    if (p->failed) return false;
  }

  /* Zero or more exit <factGroup> (at least one for lemma def) */
  StigiFactGroup *ex = NULL;
  uint32_t ex_cnt = 0, ex_cap = 0;
  for (;;) {
    skip_head_cont_nl(p, decl_col);
    if (peek(p)->kind != TOK_LT) break;
    StigiFactGroup fg = parse_fact_group(p);
    if (p->failed) { free(ex); return false; }
    if (ex_cnt == ex_cap && !p_grow(p, (void **)&ex, &ex_cap, sizeof *ex)) {
      free(ex); return false;
    }
    ex[ex_cnt++] = fg;
  }
  if (is_lemma && ex_cnt == 0) {
    free(ex);
    p_fail(p, peek(p)->span, "lemma def requires an exit fact group after ':'");
    return false;
  }
  if (ex_cnt > 0) {
    h->exit_facts = p_alloc(p, (size_t)ex_cnt * sizeof *ex);
    if (h->exit_facts == NULL) { free(ex); p_fail_oom(p); return false; }
    memcpy(h->exit_facts, ex, (size_t)ex_cnt * sizeof *ex);
    h->exit_fact_count = ex_cnt;
  }
  free(ex);

  skip_head_cont_nl(p, decl_col);
  return true;
}

static StigiDecl *parse_rule_decl(Parser *p, bool is_pub, uint32_t decl_col) {
  StigiSpan sp = advance(p)->span; /* 'rule' */
  if (!expect(p, TOK_KW_DEF)) return NULL;
  StigiDecl *d = p_alloc(p, sizeof *d);
  if (d == NULL) { p_fail_oom(p); return NULL; }
  d->kind = STIGI_DECL_RULE;
  d->span = sp;
  d->is_pub = is_pub;
  d->name = parse_name(p);
  if (p->failed || !expect(p, TOK_COLON)) return NULL;
  d->u.rule_decl.head = parse_fact_item(p, false);
  if (p->failed) return NULL;
  if (d->u.rule_decl.head.kind != STIGI_FACT_ATOM || d->u.rule_decl.head.is_negated) {
    p_fail(p, d->u.rule_decl.head.span, "rule head must be a positive atom");
    return NULL;
  }

  if (peek(p)->kind == TOK_ASSIGN) {
    advance(p);
    if (peek(p)->kind == TOK_NEWLINE) {
      const StigiTok *nxt = peek_skip_nl(p);
      if (nxt->span.col > decl_col) {
        d->u.rule_decl.body_multiline = true;
        skip_newlines(p);
      }
    }
    StigiFactItem *lits = NULL;
    uint32_t cnt = 0, cap = 0;
    for (;;) {
      StigiFactItem it = parse_fact_item(p, false);
      if (p->failed) { free(lits); return NULL; }
      if (cnt == cap && !p_grow(p, (void **)&lits, &cap, sizeof *lits)) {
        free(lits); return NULL;
      }
      lits[cnt++] = it;
      if (peek(p)->kind == TOK_COMMA) {
        advance(p);
        if (peek(p)->kind == TOK_NEWLINE && peek_skip_nl(p)->span.col > decl_col) {
          skip_newlines(p);
        }
      } else {
        break;
      }
    }
    if (cnt > 0) {
      d->u.rule_decl.body = p_alloc(p, (size_t)cnt * sizeof *lits);
      if (d->u.rule_decl.body == NULL) { free(lits); p_fail_oom(p); return NULL; }
      memcpy(d->u.rule_decl.body, lits, (size_t)cnt * sizeof *lits);
      d->u.rule_decl.body_count = cnt;
    }
    free(lits);
  }
  return d;
}

static StigiDecl *parse_decl(Parser *p, bool is_pub, uint32_t decl_col) {
  if (p->failed || check_not_in_lang(p)) return NULL;
  const StigiTok *t = peek(p);
  if (!p_enter_depth(p, t->span)) return NULL;

  if (t->kind == TOK_KW_DEF) {
    advance(p);
    StigiDecl *d = p_alloc(p, sizeof *d);
    if (d == NULL) { p_fail_oom(p); p_leave_depth(p); return NULL; }
    d->kind = STIGI_DECL_DEF;
    d->span = t->span;
    d->is_pub = is_pub;
    d->name = parse_name(p);
    if (p->failed || !parse_def_head(p, false, decl_col, &d->u.def_decl.head)) {
      p_leave_depth(p);
      return NULL;
    }
    if (!expect(p, TOK_ASSIGN)) { p_leave_depth(p); return NULL; }
    d->u.def_decl.body = parse_block_or_expr(p, decl_col);
    p_leave_depth(p);
    return d;
  }

  if (t->kind == TOK_KW_LEMMA) {
    advance(p);
    if (!expect(p, TOK_KW_DEF)) { p_leave_depth(p); return NULL; }
    StigiDecl *d = p_alloc(p, sizeof *d);
    if (d == NULL) { p_fail_oom(p); p_leave_depth(p); return NULL; }
    d->kind = STIGI_DECL_LEMMA_DEF;
    d->span = t->span;
    d->is_pub = is_pub;
    d->name = parse_name(p);
    if (p->failed || !parse_def_head(p, true, decl_col, &d->u.def_decl.head)) {
      p_leave_depth(p);
      return NULL;
    }
    if (!expect(p, TOK_ASSIGN)) { p_leave_depth(p); return NULL; }
    d->u.def_decl.body = parse_block_or_expr(p, decl_col);
    p_leave_depth(p);
    return d;
  }

  if (t->kind == TOK_KW_VAL) {
    advance(p);
    StigiDecl *d = p_alloc(p, sizeof *d);
    if (d == NULL) { p_fail_oom(p); p_leave_depth(p); return NULL; }
    d->kind = STIGI_DECL_VAL;
    d->span = t->span;
    d->is_pub = is_pub;
    d->name = parse_name(p);
    if (p->failed) { p_leave_depth(p); return NULL; }
    skip_head_cont_nl(p, decl_col);
    if (peek(p)->kind == TOK_LBRACE) {
      if (!parse_cap_group(p, &d->u.val_decl.caps, &d->u.val_decl.cap_count)) {
        p_leave_depth(p);
        return NULL;
      }
    }
    skip_head_cont_nl(p, decl_col);
    if (!expect(p, TOK_COLON)) { p_leave_depth(p); return NULL; }
    d->u.val_decl.type = parse_type(p);
    skip_head_cont_nl(p, decl_col);
    if (p->failed || !expect(p, TOK_ASSIGN)) { p_leave_depth(p); return NULL; }
    d->u.val_decl.body = parse_block_or_expr(p, decl_col);
    if (p->failed) { p_leave_depth(p); return NULL; }

    /* Detect Test and NegTest (language.md 6) */
    if (d->u.val_decl.cap_count == 1 && d->u.val_decl.caps[0].name.len == 0 &&
        d->u.val_decl.type != NULL) {
      if (str_eq(d->u.val_decl.caps[0].cap_type->name, "Stigi.Test") &&
          str_eq(d->u.val_decl.type->name, "Stigi.Test.Result")) {
        d->kind = STIGI_DECL_TEST;
      } else if (str_eq(d->u.val_decl.caps[0].cap_type->name, "Stigi.NegTest") &&
                 str_eq(d->u.val_decl.type->name, "Never")) {
        d->kind = STIGI_DECL_NEG_TEST;
      }
    }
    p_leave_depth(p);
    return d;
  }

  if (t->kind == TOK_KW_ENUM) {
    advance(p);
    StigiDecl *d = p_alloc(p, sizeof *d);
    if (d == NULL) { p_fail_oom(p); p_leave_depth(p); return NULL; }
    d->kind = STIGI_DECL_ENUM;
    d->span = t->span;
    d->is_pub = is_pub;
    d->name = parse_name(p);
    if (p->failed) { p_leave_depth(p); return NULL; }
    if (peek(p)->kind == TOK_LBRACKET) {
      if (!parse_param_list(p, TOK_LBRACKET, TOK_RBRACKET,
                            &d->u.enum_decl.type_params, &d->u.enum_decl.type_param_count)) {
        p_leave_depth(p);
        return NULL;
      }
    }
    if (!expect(p, TOK_COLON)) { p_leave_depth(p); return NULL; }
    skip_newlines(p);
    const StigiTok *fc = peek(p);
    if (fc->kind != TOK_KW_CASE || fc->span.col <= decl_col) {
      p_fail(p, fc->span, "expected indented 'case' in enum");
      p_leave_depth(p);
      return NULL;
    }
    uint32_t case_col = fc->span.col;
    StigiEnumCase *cases = NULL;
    uint32_t cnt = 0, cap = 0;
    for (;;) {
      const StigiTok *nxt = peek_skip_nl(p);
      if (nxt->kind != TOK_KW_CASE || nxt->span.col != case_col) break;
      skip_newlines(p);
      StigiEnumCase ec;
      memset(&ec, 0, sizeof ec);
      ec.span = advance(p)->span; /* 'case' */
      ec.name = parse_name(p);
      if (p->failed) { free(cases); p_leave_depth(p); return NULL; }
      if (peek(p)->kind == TOK_LPAREN) {
        ec.has_parens = true;
        if (!parse_param_list(p, TOK_LPAREN, TOK_RPAREN, &ec.fields, &ec.field_count)) {
          free(cases);
          p_leave_depth(p);
          return NULL;
        }
        skip_head_cont_nl(p, case_col);
        if (peek(p)->kind == TOK_LT) {
          ec.has_facts = true;
          ec.facts = parse_fact_group(p);
          if (p->failed) { free(cases); p_leave_depth(p); return NULL; }
        }
      }
      if (cnt == cap && !p_grow(p, (void **)&cases, &cap, sizeof *cases)) {
        free(cases); p_leave_depth(p); return NULL;
      }
      cases[cnt++] = ec;
    }
    if (cnt > 0) {
      d->u.enum_decl.cases = p_alloc(p, (size_t)cnt * sizeof *cases);
      if (d->u.enum_decl.cases == NULL) { free(cases); p_fail_oom(p); p_leave_depth(p); return NULL; }
      memcpy(d->u.enum_decl.cases, cases, (size_t)cnt * sizeof *cases);
      d->u.enum_decl.case_count = cnt;
    }
    free(cases);
    p_leave_depth(p);
    return d;
  }

  if (t->kind == TOK_KW_TYPE) {
    advance(p);
    StigiDecl *d = p_alloc(p, sizeof *d);
    if (d == NULL) { p_fail_oom(p); p_leave_depth(p); return NULL; }
    d->kind = STIGI_DECL_TYPE;
    d->span = t->span;
    d->is_pub = is_pub;
    d->name = parse_name(p);
    if (p->failed) { p_leave_depth(p); return NULL; }
    skip_head_cont_nl(p, decl_col);
    if (peek(p)->kind == TOK_LBRACKET) {
      if (!parse_param_list(p, TOK_LBRACKET, TOK_RBRACKET,
                            &d->u.type_decl.type_params, &d->u.type_decl.type_param_count)) {
        p_leave_depth(p);
        return NULL;
      }
    }
    skip_head_cont_nl(p, decl_col);
    if (peek(p)->kind == TOK_LPAREN) {
      if (!parse_param_list(p, TOK_LPAREN, TOK_RPAREN,
                            &d->u.type_decl.val_params, &d->u.type_decl.val_param_count)) {
        p_leave_depth(p);
        return NULL;
      }
    }
    skip_head_cont_nl(p, decl_col);
    if (peek(p)->kind == TOK_LBRACE) {
      if (!parse_cap_group(p, &d->u.type_decl.cap_params, &d->u.type_decl.cap_param_count)) {
        p_leave_depth(p);
        return NULL;
      }
    }
    skip_head_cont_nl(p, decl_col);
    if (!expect(p, TOK_ASSIGN)) { p_leave_depth(p); return NULL; }
    if (peek(p)->kind == TOK_NEWLINE && peek_skip_nl(p)->span.col > decl_col) {
      skip_newlines(p);
    }
    d->u.type_decl.rhs = parse_type(p);
    if (p->failed) { p_leave_depth(p); return NULL; }
    skip_head_cont_nl(p, decl_col);
    if (peek(p)->kind == TOK_LT) {
      d->u.type_decl.has_facts = true;
      d->u.type_decl.facts = parse_fact_group(p);
    }
    p_leave_depth(p);
    return d;
  }

  if (t->kind == TOK_KW_CAPABILITY) {
    advance(p);
    StigiDecl *d = p_alloc(p, sizeof *d);
    if (d == NULL) { p_fail_oom(p); p_leave_depth(p); return NULL; }
    d->kind = STIGI_DECL_CAPABILITY;
    d->span = t->span;
    d->is_pub = is_pub;
    d->name = parse_name(p);
    if (p->failed) { p_leave_depth(p); return NULL; }
    if (peek(p)->kind == TOK_LBRACKET) {
      if (!parse_param_list(p, TOK_LBRACKET, TOK_RBRACKET,
                            &d->u.cap_decl.type_params, &d->u.cap_decl.type_param_count)) {
        p_leave_depth(p);
        return NULL;
      }
    }
    if (!expect(p, TOK_COLON)) { p_leave_depth(p); return NULL; }
    const StigiTok *fm = peek_skip_nl(p);
    /* Marker capability (e.g. `capability Stigi.Mut:` with 0 operations, ruling S42) */
    if (fm->kind == TOK_EOF || fm->span.col <= decl_col) {
      p_leave_depth(p);
      return d;
    }
    skip_newlines(p);
    uint32_t body_col = peek(p)->span.col;

    if (peek(p)->kind == TOK_KW_STATE && peek(p)->span.col == body_col) {
      advance(p);
      StigiStr *sts = NULL;
      uint32_t cnt = 0, cap = 0;
      for (;;) {
        StigiStr sn = parse_name(p);
        if (p->failed) { free(sts); p_leave_depth(p); return NULL; }
        if (cnt == cap && !p_grow(p, (void **)&sts, &cap, sizeof *sts)) {
          free(sts); p_leave_depth(p); return NULL;
        }
        sts[cnt++] = sn;
        if (peek(p)->kind == TOK_COMMA) advance(p);
        else break;
      }
      d->u.cap_decl.states = p_alloc(p, (size_t)cnt * sizeof *sts);
      if (d->u.cap_decl.states == NULL) { free(sts); p_fail_oom(p); p_leave_depth(p); return NULL; }
      memcpy(d->u.cap_decl.states, sts, (size_t)cnt * sizeof *sts);
      d->u.cap_decl.state_count = cnt;
      free(sts);

      skip_newlines(p);
      if (!expect(p, TOK_KW_START)) { p_leave_depth(p); return NULL; }
      d->u.cap_decl.start_state = parse_name(p);
      if (p->failed) { p_leave_depth(p); return NULL; }

      skip_newlines(p);
      if (!expect(p, TOK_KW_STOP)) { p_leave_depth(p); return NULL; }
      StigiStr *stops = NULL;
      cnt = 0; cap = 0;
      for (;;) {
        StigiStr sn = parse_name(p);
        if (p->failed) { free(stops); p_leave_depth(p); return NULL; }
        if (cnt == cap && !p_grow(p, (void **)&stops, &cap, sizeof *stops)) {
          free(stops); p_leave_depth(p); return NULL;
        }
        stops[cnt++] = sn;
        if (peek(p)->kind == TOK_COMMA) advance(p);
        else break;
      }
      d->u.cap_decl.stop_states = p_alloc(p, (size_t)cnt * sizeof *stops);
      if (d->u.cap_decl.stop_states == NULL) { free(stops); p_fail_oom(p); p_leave_depth(p); return NULL; }
      memcpy(d->u.cap_decl.stop_states, stops, (size_t)cnt * sizeof *stops);
      d->u.cap_decl.stop_state_count = cnt;
      free(stops);
    }

    StigiDecl **members = NULL;
    uint32_t mcnt = 0, mcap = 0;
    for (;;) {
      const StigiTok *nxt = peek_skip_nl(p);
      if ((nxt->kind != TOK_KW_DEF && nxt->kind != TOK_KW_RULE) || nxt->span.col != body_col) {
        break;
      }
      skip_newlines(p);
      StigiDecl *m = NULL;
      if (peek(p)->kind == TOK_KW_DEF) {
        StigiSpan osp = advance(p)->span;
        m = p_alloc(p, sizeof *m);
        if (m == NULL) { free(members); p_fail_oom(p); p_leave_depth(p); return NULL; }
        m->kind = STIGI_DECL_DEF;
        m->span = osp;
        m->name = parse_name(p);
        if (p->failed || !parse_def_head(p, false, body_col, &m->u.def_decl.head)) {
          free(members);
          p_leave_depth(p);
          return NULL;
        }
      } else {
        m = parse_rule_decl(p, false, body_col);
        if (p->failed) { free(members); p_leave_depth(p); return NULL; }
      }
      if (mcnt == mcap && !p_grow(p, (void **)&members, &mcap, sizeof *members)) {
        free(members); p_leave_depth(p); return NULL;
      }
      members[mcnt++] = m;
    }
    if (mcnt > 0) {
      d->u.cap_decl.members = p_alloc(p, (size_t)mcnt * sizeof *members);
      if (d->u.cap_decl.members == NULL) { free(members); p_fail_oom(p); p_leave_depth(p); return NULL; }
      memcpy(d->u.cap_decl.members, members, (size_t)mcnt * sizeof *members);
      d->u.cap_decl.member_count = mcnt;
    }
    free(members);
    p_leave_depth(p);
    return d;
  }

  if (t->kind == TOK_KW_HANDLER) {
    advance(p);
    StigiDecl *d = p_alloc(p, sizeof *d);
    if (d == NULL) { p_fail_oom(p); p_leave_depth(p); return NULL; }
    d->kind = STIGI_DECL_HANDLER;
    d->span = t->span;
    d->is_pub = is_pub;
    d->name = parse_name(p);
    if (p->failed) { p_leave_depth(p); return NULL; }
    skip_head_cont_nl(p, decl_col);
    if (peek(p)->kind == TOK_LBRACKET) {
      if (!parse_param_list(p, TOK_LBRACKET, TOK_RBRACKET,
                            &d->u.handler_decl.type_params, &d->u.handler_decl.type_param_count)) {
        p_leave_depth(p);
        return NULL;
      }
    }
    skip_head_cont_nl(p, decl_col);
    if (peek(p)->kind == TOK_LBRACE) {
      if (!parse_cap_group(p, &d->u.handler_decl.cap_params, &d->u.handler_decl.cap_param_count)) {
        p_leave_depth(p);
        return NULL;
      }
    }
    skip_head_cont_nl(p, decl_col);
    if (peek(p)->kind == TOK_LPAREN) {
      d->u.handler_decl.has_val_parens = true;
      if (!parse_param_list(p, TOK_LPAREN, TOK_RPAREN,
                            &d->u.handler_decl.val_params, &d->u.handler_decl.val_param_count)) {
        p_leave_depth(p);
        return NULL;
      }
    }
    skip_head_cont_nl(p, decl_col);
    if (!expect(p, TOK_KW_FOR)) { p_leave_depth(p); return NULL; }
    d->u.handler_decl.for_cap_type = parse_type(p);
    if (p->failed || d->u.handler_decl.for_cap_type == NULL) { p_leave_depth(p); return NULL; }
    d->u.handler_decl.for_cap = d->u.handler_decl.for_cap_type->name;
    if (!expect(p, TOK_COLON)) { p_leave_depth(p); return NULL; }
    skip_newlines(p);
    const StigiTok *fm = peek(p);
    if (fm->span.col <= decl_col) {
      p_fail(p, fm->span, "expected indented handler body");
      p_leave_depth(p);
      return NULL;
    }
    uint32_t body_col = fm->span.col;

    StigiExpr **vars = NULL;
    uint32_t vcnt = 0, vcap = 0;
    for (;;) {
      const StigiTok *nxt = peek_skip_nl(p);
      if (nxt->kind != TOK_KW_VAR || nxt->span.col != body_col) break;
      skip_newlines(p);
      StigiExpr *v = parse_stmt(p, body_col);
      if (p->failed) { free(vars); p_leave_depth(p); return NULL; }
      if (vcnt == vcap && !p_grow(p, (void **)&vars, &vcap, sizeof *vars)) {
        free(vars); p_leave_depth(p); return NULL;
      }
      vars[vcnt++] = v;
    }
    if (vcnt > 0) {
      d->u.handler_decl.vars = p_alloc(p, (size_t)vcnt * sizeof *vars);
      if (d->u.handler_decl.vars == NULL) { free(vars); p_fail_oom(p); p_leave_depth(p); return NULL; }
      memcpy(d->u.handler_decl.vars, vars, (size_t)vcnt * sizeof *vars);
      d->u.handler_decl.var_count = vcnt;
    }
    free(vars);

    StigiDecl **defs = NULL;
    uint32_t dcnt = 0, dcap = 0;
    for (;;) {
      const StigiTok *nxt = peek_skip_nl(p);
      if (nxt->kind != TOK_KW_DEF || nxt->span.col != body_col) break;
      skip_newlines(p);
      StigiDecl *op = parse_decl(p, false, body_col);
      if (p->failed) { free(defs); p_leave_depth(p); return NULL; }
      if (dcnt == dcap && !p_grow(p, (void **)&defs, &dcap, sizeof *defs)) {
        free(defs); p_leave_depth(p); return NULL;
      }
      defs[dcnt++] = op;
    }
    if (dcnt > 0) {
      d->u.handler_decl.defs = p_alloc(p, (size_t)dcnt * sizeof *defs);
      if (d->u.handler_decl.defs == NULL) { free(defs); p_fail_oom(p); p_leave_depth(p); return NULL; }
      memcpy(d->u.handler_decl.defs, defs, (size_t)dcnt * sizeof *defs);
      d->u.handler_decl.def_count = dcnt;
    }
    free(defs);
    p_leave_depth(p);
    return d;
  }

  if (t->kind == TOK_KW_EXTERN) {
    advance(p);
    const StigiTok *abi = peek(p);
    if (abi->kind != TOK_STRING) {
      p_fail(p, abi->span, "expected ABI string after 'extern'");
      p_leave_depth(p);
      return NULL;
    }
    advance(p);
    if (abi->len != 3 || memcmp(abi->ptr, "\"c\"", 3) != 0) {
      p_fail_not_in_lang(p, abi->span, "extern-abi");
      p_leave_depth(p);
      return NULL;
    }
    StigiDecl *d = p_alloc(p, sizeof *d);
    if (d == NULL) { p_fail_oom(p); p_leave_depth(p); return NULL; }
    d->kind = STIGI_DECL_EXTERN;
    d->span = t->span;
    d->is_pub = is_pub;
    d->u.extern_decl.abi = tok_str(abi);

    if (peek(p)->kind == TOK_KW_DEF) {
      advance(p);
      d->u.extern_decl.is_val = false;
      d->name = parse_name(p);
      if (p->failed || !parse_def_head(p, false, decl_col, &d->u.extern_decl.head)) {
        p_leave_depth(p);
        return NULL;
      }
      if (peek(p)->kind == TOK_ASSIGN) {
        advance(p);
        d->u.extern_decl.body = parse_block_or_expr(p, decl_col);
      }
      p_leave_depth(p);
      return d;
    }
    if (peek(p)->kind == TOK_KW_VAL) {
      advance(p);
      d->u.extern_decl.is_val = true;
      d->name = parse_name(p);
      if (p->failed || !expect(p, TOK_COLON)) { p_leave_depth(p); return NULL; }
      d->u.extern_decl.val_type = parse_type(p);
      p_leave_depth(p);
      return d;
    }
    p_fail(p, peek(p)->span, "expected 'def' or 'val' after extern \"c\"");
    p_leave_depth(p);
    return NULL;
  }

  if (t->kind == TOK_KW_RELATION) {
    advance(p);
    StigiDecl *d = p_alloc(p, sizeof *d);
    if (d == NULL) { p_fail_oom(p); p_leave_depth(p); return NULL; }
    d->kind = STIGI_DECL_RELATION;
    d->span = t->span;
    d->is_pub = is_pub;
    d->name = parse_name(p);
    if (p->failed) { p_leave_depth(p); return NULL; }
    if (!parse_param_list(p, TOK_LPAREN, TOK_RPAREN,
                          &d->u.relation_decl.columns, &d->u.relation_decl.column_count)) {
      p_leave_depth(p);
      return NULL;
    }
    p_leave_depth(p);
    return d;
  }

  if (t->kind == TOK_KW_RULE) {
    StigiDecl *rd = parse_rule_decl(p, is_pub, decl_col);
    p_leave_depth(p);
    return rd;
  }

  if (t->kind == TOK_KW_FOR) {
    p_fail_not_in_lang(p, t->span, "for");
    p_leave_depth(p);
    return NULL;
  }

  p_fail(p, t->span, "expected declaration, got '%s'", stigi_tok_kind_name(t->kind));
  p_leave_depth(p);
  return NULL;
}

StigiParseResult stigi_parse_tokens(const StigiTok *toks, uint32_t count, Arena *arena) {
  Parser p;
  memset(&p, 0, sizeof p);
  p.arena = arena;
  p.res.status = STIGI_OK;
  p.res.diag_kind = STIGI_PARSE_OK;

  if (toks == NULL || count == 0 || arena == NULL) {
    p.res.status = STIGI_ERR_INVALID;
    p.res.diag_kind = STIGI_PARSE_ERR_PARSE;
    p.res.diag_code = "parse";
    (void)snprintf(p.res.msg, sizeof p.res.msg, "invalid arguments");
    return p.res;
  }

  p.toks = toks;
  p.count = count;
  p.pos = 0;

  StigiUnit *unit = p_alloc(&p, sizeof *unit);
  if (unit == NULL) {
    p_fail_oom(&p);
    return p.res;
  }

  skip_newlines(&p);

  /* Optional // prompt: line */
  if (peek(&p)->kind == TOK_PROMPT) {
    const StigiTok *pt = advance(&p);
    unit->has_prompt = true;
    const char *txt = pt->ptr + 10;
    uint32_t tlen = pt->len - 10;
    if (tlen > 0 && txt[0] == ' ') {
      txt++;
      tlen--;
    }
    unit->prompt.ptr = txt;
    unit->prompt.len = tlen;
    skip_newlines(&p);
  }

  /* Zero or more alias declarations */
  StigiAlias *aliases = NULL;
  uint32_t acnt = 0, acap = 0;
  while (peek(&p)->kind == TOK_KW_ALIAS) {
    StigiAlias al;
    memset(&al, 0, sizeof al);
    al.span = advance(&p)->span;
    al.name = parse_name(&p);
    if (p.failed || !expect(&p, TOK_ASSIGN)) { free(aliases); return p.res; }
    al.target = parse_name(&p);
    if (p.failed) { free(aliases); return p.res; }
    if (acnt == acap && !p_grow(&p, (void **)&aliases, &acap, sizeof *aliases)) {
      free(aliases); return p.res;
    }
    aliases[acnt++] = al;
    skip_newlines(&p);
  }
  if (acnt > 0) {
    unit->aliases = p_alloc(&p, (size_t)acnt * sizeof *aliases);
    if (unit->aliases == NULL) { free(aliases); p_fail_oom(&p); return p.res; }
    memcpy(unit->aliases, aliases, (size_t)acnt * sizeof *aliases);
    unit->alias_count = acnt;
  }
  free(aliases);

  bool is_pub = false;
  uint32_t decl_col = peek(&p)->span.col;
  if (peek(&p)->kind == TOK_KW_PUB) {
    is_pub = true;
    advance(&p);
  }

  unit->decl = parse_decl(&p, is_pub, decl_col);
  if (p.failed) return p.res;

  skip_newlines(&p);
  if (peek(&p)->kind != TOK_EOF) {
    if (!check_not_in_lang(&p)) {
      p_fail(&p, peek(&p)->span, "unexpected trailing token '%s'",
             stigi_tok_kind_name(peek(&p)->kind));
    }
    return p.res;
  }

  p.res.unit = unit;
  return p.res;
}

StigiParseResult stigi_parse_unit(const char *src, size_t len, Arena *arena) {
  StigiParseResult res;
  memset(&res, 0, sizeof res);
  if (src == NULL || arena == NULL) {
    res.status = STIGI_ERR_INVALID;
    res.diag_kind = STIGI_PARSE_ERR_PARSE;
    res.diag_code = "parse";
    (void)snprintf(res.msg, sizeof res.msg, "invalid arguments");
    return res;
  }
  StigiTok *toks = NULL;
  uint32_t count = 0;
  Diag ldiag;
  diag_clear(&ldiag);
  Status st = stigi_lex_all(src, len, &toks, &count, arena, &ldiag);
  if (st != STIGI_OK) {
    res.status = st;
    res.diag_kind = STIGI_PARSE_ERR_PARSE;
    res.diag_code = "parse";
    (void)snprintf(res.msg, sizeof res.msg, "%s", ldiag.msg);
    return res;
  }
  return stigi_parse_tokens(toks, count, arena);
}
