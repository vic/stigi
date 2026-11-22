#include "tenet_gate.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- Byte-encoded forbidden word tables (tenet-gate.md) ---- */

typedef struct {
  const uint8_t *bytes;
  uint8_t len;
} ByteWord;

static const uint8_t w_00[] = {97, 98, 111, 114, 116};
static const uint8_t w_01[] = {97, 98, 111, 114, 116, 115};
static const uint8_t w_02[] = {97, 98, 111, 114, 116, 101, 100};
static const uint8_t w_03[] = {97, 98, 111, 114, 116, 105, 110, 103};
static const uint8_t w_04[] = {116, 114, 97, 112};
static const uint8_t w_05[] = {116, 114, 97, 112, 115};
static const uint8_t w_06[] = {116, 114, 97, 112, 112, 101, 100};
static const uint8_t w_07[] = {116, 114, 97, 112, 112, 105, 110, 103};
static const uint8_t w_08[] = {117, 110, 114, 101, 97, 99, 104, 97, 98, 108, 101};
static const uint8_t w_09[] = {116, 111, 100, 111};
static const uint8_t w_10[] = {116, 111, 100, 111, 115};
static const uint8_t w_11[] = {102, 105, 120, 109, 101};
static const uint8_t w_12[] = {102, 105, 120, 109, 101, 115};
static const uint8_t w_13[] = {115, 107, 105, 112};
static const uint8_t w_14[] = {115, 107, 105, 112, 115};
static const uint8_t w_15[] = {115, 107, 105, 112, 112, 101, 100};
static const uint8_t w_16[] = {115, 107, 105, 112, 112, 105, 110, 103};
static const uint8_t w_17[] = {108, 97, 116, 101, 114};
static const uint8_t w_18[] = {115, 116, 117, 98};
static const uint8_t w_19[] = {115, 116, 117, 98, 115};
static const uint8_t w_20[] = {115, 116, 117, 98, 98, 101, 100};
static const uint8_t w_21[] = {115, 116, 117, 98, 98, 105, 110, 103};
static const uint8_t w_22[] = {112, 108, 97, 99, 101, 104, 111, 108, 100, 101, 114};
static const uint8_t w_23[] = {112, 108, 97, 99, 101, 104, 111, 108, 100, 101, 114, 115};
static const uint8_t w_24[] = {117, 110, 115, 117, 112, 112, 111, 114, 116, 101, 100};
static const uint8_t w_25[] = {112, 97, 110, 105, 99};
static const uint8_t w_26[] = {112, 97, 110, 105, 99, 115};
static const uint8_t w_27[] = {112, 97, 110, 105, 99, 107, 101, 100};

static const ByteWord k_forbidden_words[] = {
  {w_00, sizeof w_00}, {w_01, sizeof w_01}, {w_02, sizeof w_02}, {w_03, sizeof w_03},
  {w_04, sizeof w_04}, {w_05, sizeof w_05}, {w_06, sizeof w_06}, {w_07, sizeof w_07},
  {w_08, sizeof w_08}, {w_09, sizeof w_09}, {w_10, sizeof w_10}, {w_11, sizeof w_11},
  {w_12, sizeof w_12}, {w_13, sizeof w_13}, {w_14, sizeof w_14}, {w_15, sizeof w_15},
  {w_16, sizeof w_16}, {w_17, sizeof w_17}, {w_18, sizeof w_18}, {w_19, sizeof w_19},
  {w_20, sizeof w_20}, {w_21, sizeof w_21}, {w_22, sizeof w_22}, {w_23, sizeof w_23},
  {w_24, sizeof w_24}, {w_25, sizeof w_25}, {w_26, sizeof w_26}, {w_27, sizeof w_27}
};

/* Ruling S117: exit call targets / emitted C exit( tokens only */
static const uint8_t x_00[] = {101, 120, 105, 116};
static const uint8_t x_01[] = {101, 120, 105, 116, 115};
static const uint8_t x_02[] = {101, 120, 105, 116, 101, 100};
static const uint8_t x_03[] = {101, 120, 105, 116, 105, 110, 103};

static const ByteWord k_exit_words[] = {
  {x_00, sizeof x_00}, {x_01, sizeof x_01}, {x_02, sizeof x_02}, {x_03, sizeof x_03}
};

static const char *const k_short_allow[] = {
  "of", "at", "to", "in", "is", "by", "or", "on", "if", "as"
};

size_t stigi_gate_word_count(void) {
  return sizeof(k_forbidden_words) / sizeof(k_forbidden_words[0]);
}

size_t stigi_gate_word_decode(size_t idx, char *buf, size_t cap) {
  if (idx >= stigi_gate_word_count() || buf == NULL || cap == 0) return 0;
  size_t n = k_forbidden_words[idx].len;
  if (n + 1 > cap) n = cap - 1;
  memcpy(buf, k_forbidden_words[idx].bytes, n);
  buf[n] = '\0';
  return k_forbidden_words[idx].len;
}

size_t stigi_gate_exit_word_count(void) {
  return sizeof(k_exit_words) / sizeof(k_exit_words[0]);
}

size_t stigi_gate_exit_word_decode(size_t idx, char *buf, size_t cap) {
  if (idx >= stigi_gate_exit_word_count() || buf == NULL || cap == 0) return 0;
  size_t n = k_exit_words[idx].len;
  if (n + 1 > cap) n = cap - 1;
  memcpy(buf, k_exit_words[idx].bytes, n);
  buf[n] = '\0';
  return k_exit_words[idx].len;
}

void stigi_gate_findings_init(StigiGateFindings *f) {
  if (f == NULL) return;
  f->items = NULL;
  f->count = 0;
  f->cap = 0;
}

void stigi_gate_findings_free(StigiGateFindings *f) {
  if (f == NULL) return;
  free(f->items);
  f->items = NULL;
  f->count = 0;
  f->cap = 0;
}

const char *stigi_gate_code_name(StigiGateCode code) {
  switch (code) {
    case STIGI_GATE_FORBIDDEN_WORD: return "forbidden-word";
    case STIGI_GATE_BARE_POINTER: return "bare-pointer";
    case STIGI_GATE_EXPECT_FORM: return "expect-form";
    case STIGI_GATE_CRYPTIC_NAME: return "cryptic-name";
    case STIGI_GATE_CASE_PAIR: return "case-pair";
    case STIGI_GATE_PROMPT_MISSING: return "prompt-missing";
    case STIGI_GATE_NAME_CASE: return "name-case";
  }
  return "unknown";
}

static Status add_finding(StigiGateFindings *out, StigiGateCode code, StigiSpan span,
                          Diag *diag, const char *fmt, ...) {
  if (out == NULL) return STIGI_OK;
  if (out->count == out->cap) {
    uint32_t ncap = out->cap == 0 ? 8U : out->cap * 2U;
    if (ncap <= out->cap) return diag_set(diag, STIGI_ERR_OVERFLOW, -1, "findings overflow");
    StigiGateFinding *ni = realloc(out->items, (size_t)ncap * sizeof *ni);
    if (ni == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "findings out of memory");
    out->items = ni;
    out->cap = ncap;
  }
  StigiGateFinding *f = &out->items[out->count++];
  f->code = code;
  f->span = span;
  va_list ap;
  va_start(ap, fmt);
  (void)vsnprintf(f->msg, sizeof f->msg, fmt, ap);
  va_end(ap);
  return STIGI_OK;
}

static inline bool is_upper(unsigned char c) { return c >= 'A' && c <= 'Z'; }
static inline bool is_lower(unsigned char c) { return c >= 'a' && c <= 'z'; }
static inline bool is_alpha(unsigned char c) { return is_upper(c) || is_lower(c); }
static inline bool is_digit(unsigned char c) { return c >= '0' && c <= '9'; }
static inline unsigned char to_lower(unsigned char c) {
  return is_upper(c) ? (unsigned char)(c - 'A' + 'a') : c;
}

static bool match_byte_word_ci(const char *s, size_t len, const ByteWord *w) {
  if (len != w->len) return false;
  for (size_t i = 0; i < len; i++) {
    if (to_lower((unsigned char)s[i]) != w->bytes[i]) return false;
  }
  return true;
}

static bool is_general_forbidden(const char *s, size_t len) {
  size_t n = stigi_gate_word_count();
  for (size_t i = 0; i < n; i++) {
    if (match_byte_word_ci(s, len, &k_forbidden_words[i])) return true;
  }
  return false;
}

static bool is_exit_forbidden(const char *s, size_t len) {
  size_t n = stigi_gate_exit_word_count();
  for (size_t i = 0; i < n; i++) {
    if (match_byte_word_ci(s, len, &k_exit_words[i])) return true;
  }
  return false;
}

static bool is_short_allowed(const char *s, size_t len) {
  if (len != 2) return false;
  char low[2];
  low[0] = (char)to_lower((unsigned char)s[0]);
  low[1] = (char)to_lower((unsigned char)s[1]);
  size_t n = sizeof(k_short_allow) / sizeof(k_short_allow[0]);
  for (size_t i = 0; i < n; i++) {
    if (low[0] == k_short_allow[i][0] && low[1] == k_short_allow[i][1]) return true;
  }
  return false;
}

/* Scan sub-words (split on non-letter and lower->upper transition) for general forbidden words */
static Status scan_forbidden_subwords(const char *s, size_t len, StigiSpan span,
                                      StigiGateFindings *out, Diag *diag) {
  size_t i = 0;
  while (i < len) {
    while (i < len && !is_alpha((unsigned char)s[i])) i++;
    if (i >= len) break;
    size_t start = i;
    i++;
    while (i < len && is_alpha((unsigned char)s[i])) {
      if (is_lower((unsigned char)s[i - 1]) && is_upper((unsigned char)s[i])) {
        break;
      }
      i++;
    }
    size_t wlen = i - start;
    if (is_general_forbidden(s + start, wlen)) {
      Status st = add_finding(out, STIGI_GATE_FORBIDDEN_WORD, span, diag,
                              "forbidden word '%.*s'", (int)wlen, s + start);
      if (st != STIGI_OK) return st;
    }
  }
  return STIGI_OK;
}

static bool tok_eq(const StigiTok *t, const char *lit) {
  size_t n = strlen(lit);
  return t->len == n && memcmp(t->ptr, lit, n) == 0;
}

/* Check cryptic-name on a single component (no dots) */
static Status check_component_cryptic(const char *s, size_t len, StigiSpan span,
                                      StigiGateFindings *out, Diag *diag) {
  if (len == 0) return STIGI_OK;
  if (is_digit((unsigned char)s[len - 1])) {
    return add_finding(out, STIGI_GATE_CRYPTIC_NAME, span, diag,
                       "name '%.*s' ends in a digit", (int)len, s);
  }
  size_t i = 0;
  while (i < len) {
    while (i < len && !is_alpha((unsigned char)s[i])) i++;
    if (i >= len) break;
    size_t start = i;
    i++;
    while (i < len && is_alpha((unsigned char)s[i])) {
      if (is_lower((unsigned char)s[i - 1]) && is_upper((unsigned char)s[i])) {
        break;
      }
      i++;
    }
    size_t wlen = i - start;
    if (wlen <= 2 && !is_short_allowed(s + start, wlen)) {
      return add_finding(out, STIGI_GATE_CRYPTIC_NAME, span, diag,
                         "cryptic sub-word '%.*s' in '%.*s'", (int)wlen, s + start,
                         (int)len, s);
    }
  }
  return STIGI_OK;
}

/* Check cryptic-name and name-case on a possibly dotted name.
 * expect_upper_last: true for type/enum/case/capability/handler/state, false for def/val/var/binder/relation/rule */
static Status check_name_rules(const StigiTok *tok, bool expect_upper_last,
                               bool check_cryptic, StigiGateFindings *out, Diag *diag) {
  const char *s = tok->ptr;
  size_t len = tok->len;
  size_t pos = 0;
  while (pos < len) {
    size_t dot = pos;
    while (dot < len && s[dot] != '.') dot++;
    bool is_last = (dot == len);
    size_t clen = dot - pos;
    if (clen > 0) {
      unsigned char first = (unsigned char)s[pos];
      if (!is_last) {
        if (!is_upper(first)) {
          Status st = add_finding(out, STIGI_GATE_NAME_CASE, tok->span, diag,
                                  "namespace component '%.*s' must start with uppercase",
                                  (int)clen, s + pos);
          if (st != STIGI_OK) return st;
        }
      } else {
        if (expect_upper_last && !is_upper(first)) {
          Status st = add_finding(out, STIGI_GATE_NAME_CASE, tok->span, diag,
                                  "name '%.*s' must start with uppercase",
                                  (int)clen, s + pos);
          if (st != STIGI_OK) return st;
        } else if (!expect_upper_last && !is_lower(first)) {
          Status st = add_finding(out, STIGI_GATE_NAME_CASE, tok->span, diag,
                                  "name '%.*s' must start with lowercase",
                                  (int)clen, s + pos);
          if (st != STIGI_OK) return st;
        }
      }
      if (check_cryptic) {
        Status st = check_component_cryptic(s + pos, clen, tok->span, out, diag);
        if (st != STIGI_OK) return st;
      }
    }
    pos = is_last ? len : dot + 1;
  }
  return STIGI_OK;
}

static uint32_t next_sig_tok(const StigiTok *toks, uint32_t count, uint32_t i) {
  while (i < count && (toks[i].kind == TOK_COMMENT || toks[i].kind == TOK_PROMPT)) {
    i++;
  }
  return i;
}

static uint32_t prev_sig_tok(const StigiTok *toks, uint32_t i) {
  while (i > 0) {
    i--;
    if (toks[i].kind != TOK_COMMENT && toks[i].kind != TOK_PROMPT) return i;
  }
  return UINT32_MAX;
}

static bool is_decl_intro_kw(StigiTokKind k) {
  return k == TOK_KW_DEF || k == TOK_KW_VAL || k == TOK_KW_VAR || k == TOK_KW_CASE ||
         k == TOK_KW_TYPE || k == TOK_KW_ENUM || k == TOK_KW_HANDLER ||
         k == TOK_KW_CAPABILITY || k == TOK_KW_RELATION || k == TOK_KW_RULE;
}

static Status check_expect_call(const StigiTok *toks, uint32_t count, uint32_t idx,
                                uint32_t expected_args, StigiGateFindings *out, Diag *diag) {
  uint32_t lp = next_sig_tok(toks, count, idx + 1);
  if (lp >= count || toks[lp].kind != TOK_LPAREN) {
    return add_finding(out, STIGI_GATE_EXPECT_FORM, toks[idx].span, diag,
                       "%.*s must be called with %u arguments",
                       (int)toks[idx].len, toks[idx].ptr, expected_args);
  }
  int depth = 0;
  uint32_t arg_count = 0;
  uint32_t cur_start = lp + 1;
  uint32_t last_arg_start = 0;
  uint32_t last_arg_end = 0;
  bool any_in_cur = false;

  uint32_t j = lp + 1;
  for (; j < count && toks[j].kind != TOK_EOF; j++) {
    StigiTokKind k = toks[j].kind;
    if (k == TOK_COMMENT || k == TOK_NEWLINE) continue;
    if (k == TOK_LPAREN || k == TOK_LBRACKET || k == TOK_LBRACE) {
      depth++;
      any_in_cur = true;
    } else if (k == TOK_RPAREN || k == TOK_RBRACKET || k == TOK_RBRACE) {
      if (depth == 0 && k == TOK_RPAREN) {
        if (any_in_cur) {
          arg_count++;
          last_arg_start = cur_start;
          last_arg_end = j;
        }
        break;
      }
      if (depth > 0) depth--;
    } else if (k == TOK_COMMA && depth == 0) {
      arg_count++;
      last_arg_start = cur_start;
      last_arg_end = j;
      cur_start = j + 1;
      any_in_cur = false;
    } else {
      any_in_cur = true;
    }
  }

  if (arg_count != expected_args) {
    return add_finding(out, STIGI_GATE_EXPECT_FORM, toks[idx].span, diag,
                       "%.*s requires %u arguments, got %u",
                       (int)toks[idx].len, toks[idx].ptr, expected_args, arg_count);
  }

  /* Verify last argument (reason) is a single non-empty string literal */
  uint32_t str_count = 0;
  uint32_t non_str_count = 0;
  uint32_t str_tok_idx = 0;
  for (uint32_t k = last_arg_start; k < last_arg_end; k++) {
    if (toks[k].kind == TOK_NEWLINE || toks[k].kind == TOK_COMMENT) continue;
    if (toks[k].kind == TOK_STRING) {
      str_count++;
      str_tok_idx = k;
    } else {
      non_str_count++;
    }
  }
  if (str_count != 1 || non_str_count != 0 || toks[str_tok_idx].len <= 2) {
    return add_finding(out, STIGI_GATE_EXPECT_FORM, toks[idx].span, diag,
                       "%.*s reason must be a non-empty text literal",
                       (int)toks[idx].len, toks[idx].ptr);
  }
  return STIGI_OK;
}

Status stigi_gate_check_tokens(const StigiTok *toks, uint32_t count,
                               StigiGateFindings *out, Diag *diag) {
  if (toks == NULL || out == NULL) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_gate_check_tokens: invalid arguments");
  }

  /* 1. prompt-missing: first non-newline token must be column-0 // prompt: with non-empty text */
  bool prompt_ok = false;
  StigiSpan first_span = {0, 0, 1, 0};
  for (uint32_t i = 0; i < count; i++) {
    if (toks[i].kind == TOK_NEWLINE) continue;
    first_span = toks[i].span;
    if (toks[i].kind == TOK_PROMPT && toks[i].span.col == 0 && toks[i].len > 10) {
      const char *p = toks[i].ptr + 10;
      uint32_t rem = toks[i].len - 10;
      for (uint32_t k = 0; k < rem; k++) {
        if (p[k] != ' ') {
          prompt_ok = true;
          break;
        }
      }
    }
    break;
  }
  if (!prompt_ok) {
    Status st = add_finding(out, STIGI_GATE_PROMPT_MISSING, first_span, diag,
                            "unit must start with column-0 '// prompt: TEXT'");
    if (st != STIGI_OK) return st;
  }

  /* Pre-scan: determine if unit is a `type` declaration with a fact group on its RHS */
  bool is_type_unit_with_facts = false;
  uint32_t type_eq_idx = UINT32_MAX;
  uint32_t type_end_idx = UINT32_MAX;
  for (uint32_t i = 0; i < count; i++) {
    if (toks[i].kind == TOK_KW_TYPE) {
      /* Find '=' on this declaration line, then check for '<' before newline/EOF */
      uint32_t j = i + 1;
      while (j < count && toks[j].kind != TOK_ASSIGN && toks[j].kind != TOK_NEWLINE &&
             toks[j].kind != TOK_EOF) {
        j++;
      }
      if (j < count && toks[j].kind == TOK_ASSIGN) {
        type_eq_idx = j;
        bool has_lt = false;
        uint32_t k = j + 1;
        while (k < count && toks[k].kind != TOK_NEWLINE && toks[k].kind != TOK_EOF) {
          if (toks[k].kind == TOK_LT) has_lt = true;
          k++;
        }
        type_end_idx = k;
        is_type_unit_with_facts = has_lt;
      }
      break;
    }
    if (toks[i].kind == TOK_KW_DEF || toks[i].kind == TOK_KW_VAL ||
        toks[i].kind == TOK_KW_ENUM || toks[i].kind == TOK_KW_CAPABILITY ||
        toks[i].kind == TOK_KW_HANDLER || toks[i].kind == TOK_KW_EXTERN ||
        toks[i].kind == TOK_KW_RELATION || toks[i].kind == TOK_KW_RULE ||
        toks[i].kind == TOK_KW_LEMMA) {
      break;
    }
  }

  /* Track bracket depth for parameter/binder scanning */
  int paren_depth = 0;
  int bracket_depth = 0;
  int brace_depth = 0;

  for (uint32_t i = 0; i < count; i++) {
    const StigiTok *t = &toks[i];
    if (t->kind == TOK_EOF) break;

    if (t->kind == TOK_LPAREN) paren_depth++;
    else if (t->kind == TOK_RPAREN && paren_depth > 0) paren_depth--;
    else if (t->kind == TOK_LBRACKET) bracket_depth++;
    else if (t->kind == TOK_RBRACKET && bracket_depth > 0) bracket_depth--;
    else if (t->kind == TOK_LBRACE) brace_depth++;
    else if (t->kind == TOK_RBRACE && brace_depth > 0) brace_depth--;

    /* 2. forbidden-word on identifiers, comments, prompts, string literals */
    if (t->kind == TOK_IDENT || t->kind == TOK_COMMENT ||
        t->kind == TOK_PROMPT || t->kind == TOK_STRING) {
      Status st = scan_forbidden_subwords(t->ptr, t->len, t->span, out, diag);
      if (st != STIGI_OK) return st;
    }

    /* 2b. S117: exit / exits / exited / exiting ONLY on call targets (including exit[T](...), exit{r}(...)) */
    if (t->kind == TOK_IDENT) {
      uint32_t nxt = next_sig_tok(toks, count, i + 1);
      uint32_t prv = prev_sig_tok(toks, i);
      bool prev_is_decl = (prv != UINT32_MAX && is_decl_intro_kw(toks[prv].kind));
      if (!prev_is_decl) {
        /* Skip balanced [...] and {...} groups before checking '(' */
        while (nxt < count && (toks[nxt].kind == TOK_LBRACKET || toks[nxt].kind == TOK_LBRACE)) {
          StigiTokKind open_k = toks[nxt].kind;
          StigiTokKind close_k = (open_k == TOK_LBRACKET) ? TOK_RBRACKET : TOK_RBRACE;
          int depth = 1;
          nxt++;
          while (nxt < count && depth > 0 && toks[nxt].kind != TOK_EOF) {
            if (toks[nxt].kind == open_k) depth++;
            else if (toks[nxt].kind == close_k) depth--;
            nxt++;
          }
          nxt = next_sig_tok(toks, count, nxt);
        }
      }
      if (!prev_is_decl && nxt < count && toks[nxt].kind == TOK_LPAREN) {
        /* Extract last dotted component */
        const char *last = t->ptr;
        uint32_t last_len = t->len;
        for (uint32_t k = 0; k < t->len; k++) {
          if (t->ptr[k] == '.') {
            last = t->ptr + k + 1;
            last_len = t->len - (k + 1);
          }
        }
        if (is_exit_forbidden(last, last_len)) {
          Status st = add_finding(out, STIGI_GATE_FORBIDDEN_WORD, t->span, diag,
                                  "forbidden call target '%.*s' (S117)",
                                  (int)t->len, t->ptr);
          if (st != STIGI_OK) return st;
        }
      }
    }

    /* 3. bare-pointer */
    if (t->kind == TOK_IDENT && tok_eq(t, "Ptr")) {
      bool allowed = is_type_unit_with_facts && i > type_eq_idx && i < type_end_idx;
      if (!allowed) {
        Status st = add_finding(out, STIGI_GATE_BARE_POINTER, t->span, diag,
                                "bare 'Ptr' is only allowed on the RHS of a 'type' with a fact group");
        if (st != STIGI_OK) return st;
      }
    }

    /* 4. expect-form */
    if (t->kind == TOK_IDENT && tok_eq(t, "Stigi.Test.expect")) {
      Status st = check_expect_call(toks, count, i, 3, out, diag);
      if (st != STIGI_OK) return st;
    } else if (t->kind == TOK_IDENT && tok_eq(t, "Stigi.NegTest.expect")) {
      Status st = check_expect_call(toks, count, i, 2, out, diag);
      if (st != STIGI_OK) return st;
    }

    /* 5. cryptic-name & name-case on unit names and binders */
    if (t->kind == TOK_KW_DEF || t->kind == TOK_KW_RELATION) {
      uint32_t n = next_sig_tok(toks, count, i + 1);
      if (n < count && toks[n].kind == TOK_IDENT) {
        Status st = check_name_rules(&toks[n], false, true, out, diag);
        if (st != STIGI_OK) return st;
      }
    } else if (t->kind == TOK_KW_VAL || t->kind == TOK_KW_VAR) {
      uint32_t n = next_sig_tok(toks, count, i + 1);
      if (n < count && toks[n].kind == TOK_IDENT) {
        uint32_t after = next_sig_tok(toks, count, n + 1);
        /* In `val Pair(a, b) = p`, `Pair` is a constructor pattern; check its binders */
        if (t->kind == TOK_KW_VAL && after < count && toks[after].kind == TOK_LPAREN) {
          Status st = check_name_rules(&toks[n], true, false, out, diag);
          if (st != STIGI_OK) return st;
          uint32_t b = next_sig_tok(toks, count, after + 1);
          while (b < count && toks[b].kind != TOK_RPAREN && toks[b].kind != TOK_EOF) {
            if (toks[b].kind == TOK_IDENT) {
              st = check_name_rules(&toks[b], false, true, out, diag);
              if (st != STIGI_OK) return st;
            }
            b = next_sig_tok(toks, count, b + 1);
          }
        } else {
          Status st = check_name_rules(&toks[n], false, true, out, diag);
          if (st != STIGI_OK) return st;
        }
      }
    } else if (t->kind == TOK_KW_ENUM || t->kind == TOK_KW_TYPE ||
               t->kind == TOK_KW_CAPABILITY || t->kind == TOK_KW_HANDLER) {
      uint32_t n = next_sig_tok(toks, count, i + 1);
      if (n < count && toks[n].kind == TOK_IDENT) {
        Status st = check_name_rules(&toks[n], true, true, out, diag);
        if (st != STIGI_OK) return st;
      }
    } else if (t->kind == TOK_KW_CASE) {
      /* Distinguish enum case vs match case: enum case has no '=>' on its line */
      uint32_t n = next_sig_tok(toks, count, i + 1);
      if (n < count && toks[n].kind == TOK_IDENT) {
        bool is_match_arm = false;
        for (uint32_t k = n + 1; k < count && toks[k].kind != TOK_NEWLINE &&
             toks[k].kind != TOK_EOF; k++) {
          if (toks[k].kind == TOK_ARROW) {
            is_match_arm = true;
            break;
          }
        }
        if (!is_match_arm) {
          Status st = check_name_rules(&toks[n], true, true, out, diag);
          if (st != STIGI_OK) return st;
        } else {
          /* Match arm pattern: check ctor binders `case Ctor(bi, x2) =>` or var pattern `case bi =>` */
          uint32_t after = next_sig_tok(toks, count, n + 1);
          if (after < count && toks[after].kind == TOK_LPAREN) {
            uint32_t b = next_sig_tok(toks, count, after + 1);
            while (b < count && toks[b].kind != TOK_RPAREN && toks[b].kind != TOK_EOF) {
              if (toks[b].kind == TOK_IDENT) {
                Status st = check_name_rules(&toks[b], false, true, out, diag);
                if (st != STIGI_OK) return st;
              }
              b = next_sig_tok(toks, count, b + 1);
            }
          } else if (toks[n].len > 0 && is_lower((unsigned char)toks[n].ptr[0])) {
            Status st = check_name_rules(&toks[n], false, true, out, diag);
            if (st != STIGI_OK) return st;
          }
        }
      }
    } else if (t->kind == TOK_KW_STATE || t->kind == TOK_KW_START || t->kind == TOK_KW_STOP) {
      uint32_t n = next_sig_tok(toks, count, i + 1);
      while (n < count && toks[n].kind == TOK_IDENT) {
        Status st = check_name_rules(&toks[n], true, true, out, diag);
        if (st != STIGI_OK) return st;
        uint32_t nxt = next_sig_tok(toks, count, n + 1);
        if (nxt < count && toks[nxt].kind == TOK_COMMA) {
          n = next_sig_tok(toks, count, nxt + 1);
        } else {
          break;
        }
      }
    } else if (t->kind == TOK_IDENT && paren_depth > 0 && bracket_depth == 0 && brace_depth == 0) {
      /* Parameter / field / column binder: `name: Type` inside `(...)` */
      uint32_t nxt = next_sig_tok(toks, count, i + 1);
      if (nxt < count && toks[nxt].kind == TOK_COLON) {
        Status st = check_name_rules(t, false, true, out, diag);
        if (st != STIGI_OK) return st;
      }
    }
  }

  return STIGI_OK;
}

Status stigi_gate_check_source(const char *src, size_t len,
                               StigiGateFindings *out, Diag *diag) {
  if (src == NULL || out == NULL) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_gate_check_source: invalid arguments");
  }
  Arena a;
  arena_init(&a);
  StigiTok *toks = NULL;
  uint32_t count = 0;
  Status st = stigi_lex_all(src, len, &toks, &count, &a, diag);
  if (st != STIGI_OK) {
    arena_free(&a);
    return st;
  }
  st = stigi_gate_check_tokens(toks, count, out, diag);
  arena_free(&a);
  return st;
}

Status stigi_gate_check_c_source(const char *src, size_t len,
                                 StigiGateFindings *out, Diag *diag) {
  if (src == NULL || out == NULL) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_gate_check_c_source: invalid arguments");
  }
  size_t i = 0;
  uint32_t line = 1;
  uint32_t col = 0;
  while (i < len) {
    unsigned char c = (unsigned char)src[i];
    if (c == '\n') {
      i++;
      line++;
      col = 0;
      continue;
    }
    if (is_alpha(c) || c == '_') {
      size_t start = i;
      uint32_t scol = col;
      while (i < len && (is_alpha((unsigned char)src[i]) || is_digit((unsigned char)src[i]) ||
                         src[i] == '_')) {
        i++;
        col++;
      }
      size_t id_len = i - start;
      StigiSpan sp = {(uint32_t)start, (uint32_t)id_len, line, scol};
      Status st = scan_forbidden_subwords(src + start, id_len, sp, out, diag);
      if (st != STIGI_OK) return st;

      /* S117: check emitted C `exit(` / `exits(` / `exited(` / `exiting(` */
      size_t k = i;
      while (k < len && (src[k] == ' ' || src[k] == '\t')) k++;
      if (k < len && src[k] == '(' && is_exit_forbidden(src + start, id_len)) {
        st = add_finding(out, STIGI_GATE_FORBIDDEN_WORD, sp, diag,
                         "forbidden C call '%.*s(' (S117)", (int)id_len, src + start);
        if (st != STIGI_OK) return st;
      }
      continue;
    }
    i++;
    col++;
  }
  return STIGI_OK;
}

static bool str_eq_ci(const char *a, const char *b) {
  while (*a != '\0' && *b != '\0') {
    if (to_lower((unsigned char)*a) != to_lower((unsigned char)*b)) return false;
    a++;
    b++;
  }
  return *a == '\0' && *b == '\0';
}

static uint64_t hash_ci(const char *s) {
  uint64_t h = 14695981039346656037ULL;
  for (; *s != '\0'; s++) {
    h ^= (uint8_t)to_lower((unsigned char)*s);
    h *= 1099511628211ULL;
  }
  h ^= h >> 30;
  h *= 0xbf58476d1ce4e5b9ULL;
  h ^= h >> 27;
  h *= 0x94d049bb133111ebULL;
  h ^= h >> 31;
  return h;
}

Status stigi_gate_check_case_pairs(const char *const *unit_names, size_t count,
                                   StigiGateFindings *out, Diag *diag) {
  if (out == NULL || (unit_names == NULL && count > 0)) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_gate_check_case_pairs: invalid arguments");
  }
  if (count < 2) return STIGI_OK;

  size_t cap = 16;
  while (cap < count * 2) cap *= 2;
  size_t *slots = calloc(cap, sizeof *slots);
  if (slots == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "case_pairs OOM");
  size_t mask = cap - 1;

  Status st = STIGI_OK;
  for (size_t i = 0; i < count && st == STIGI_OK; i++) {
    if (unit_names[i] == NULL) continue;
    uint64_t h = hash_ci(unit_names[i]);
    size_t idx = (size_t)(h & mask);
    while (slots[idx] != 0) {
      size_t prev = slots[idx] - 1;
      if (str_eq_ci(unit_names[prev], unit_names[i])) {
        if (strcmp(unit_names[prev], unit_names[i]) != 0) {
          StigiSpan sp = {0, 0, 1, 0};
          st = add_finding(out, STIGI_GATE_CASE_PAIR, sp, diag,
                           "case-pair collision between '%s' and '%s'",
                           unit_names[prev], unit_names[i]);
        }
        break;
      }
      idx = (idx + 1) & mask;
    }
    if (slots[idx] == 0) slots[idx] = i + 1;
  }
  free(slots);
  return st;
}

const char *stigi_help_not_in_language(const char *kw) {
  if (kw == NULL) return "This construct is not in L0; see vic/spec/language.md.";
  if (strcmp(kw, "class") == 0) {
    return "Stigi.Help: L0 has no 'class'; use 'enum' for data and top-level 'def' units.";
  }
  if (strcmp(kw, "object") == 0) {
    return "Stigi.Help: L0 has no 'object'; use a dotted namespace with top-level 'def' and 'val' units.";
  }
  if (strcmp(kw, "trait") == 0) {
    return "Stigi.Help: L0 has no 'trait'; use 'capability' and 'handler' for effectful interfaces.";
  }
  if (strcmp(kw, "given") == 0) {
    return "Stigi.Help: L0 has no 'given'; pass capabilities explicitly in {Cap} groups.";
  }
  if (strcmp(kw, "null") == 0) {
    return "Stigi.Help: L0 has no 'null'; use an enum with two cases (e.g. Some / None).";
  }
  if (strcmp(kw, "throw") == 0) {
    return "Stigi.Help: L0 has no 'throw'; return a failure case of an enum as a value.";
  }
  if (strcmp(kw, "try") == 0 || strcmp(kw, "catch") == 0) {
    return "Stigi.Help: L0 has no 'try'/'catch'; match on the returned result enum.";
  }
  if (strcmp(kw, "return") == 0) {
    return "Stigi.Help: L0 has no 'return'; the value of a block is its last expression.";
  }
  if (strcmp(kw, "import") == 0) {
    return "Stigi.Help: L0 has no 'import'; reference units by dotted name or declare a local 'alias'.";
  }
  if (strcmp(kw, "async") == 0 || strcmp(kw, "await") == 0) {
    return "Stigi.Help: L0 has no 'async'/'await'; use capabilities and handlers.";
  }
  if (strcmp(kw, "unsafe") == 0) {
    return "Stigi.Help: L0 has no 'unsafe'; Ptr[A](n) is allowed only in a Stigi.* type with facts.";
  }
  if (strcmp(kw, "goto") == 0) {
    return "Stigi.Help: L0 has no 'goto'; use structured if/else, match, and while.";
  }
  if (strcmp(kw, "for") == 0) {
    return "Stigi.Help: L0 has no 'for' comprehensions; use 'while' with an index and loop invariant.";
  }
  if (strcmp(kw, "lazy") == 0) {
    return "Stigi.Help: L0 has no 'lazy'; top-level 'val' is evaluated at admission.";
  }
  if (strcmp(kw, "let") == 0) {
    return "Stigi.Help: L0 has no 'let'; use 'val' for immutable bindings and 'var' for mutable locals.";
  }
  if (strcmp(kw, "resume") == 0) {
    return "Stigi.Help: L0 has no 'resume'; all L0 capability operations are tail-resumptive.";
  }
  if (strcmp(kw, "opaque") == 0) {
    return "Stigi.Help: L0 has no 'opaque'; use a nominal 'type' with a fact group.";
  }
  if (strcmp(kw, "lambda") == 0) {
    return "Stigi.Help: L0 has no lambdas; write a named 'def' unit instead.";
  }
  if (strcmp(kw, "float") == 0) {
    return "Stigi.Help: L0 has no floating-point literals; use exact integer types.";
  }
  if (strcmp(kw, "extern-abi") == 0) {
    return "Stigi.Help: L0 only supports extern \"c\"; no other ABI exists.";
  }
  if (strcmp(kw, "#") == 0) {
    return "Stigi.Help: L0 has no '#' blocks.";
  }
  return "Stigi.Help: This construct is not in L0; see vic/spec/language.md.";
}
