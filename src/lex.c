#include "lex.h"

#include <stdlib.h>
#include <string.h>

static inline bool is_ascii_alpha(unsigned char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static inline bool is_ascii_digit(unsigned char c) {
  return c >= '0' && c <= '9';
}

static inline bool is_ascii_hex(unsigned char c) {
  return is_ascii_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static inline bool is_ident_cont(unsigned char c) {
  return is_ascii_alpha(c) || is_ascii_digit(c) || c == '_';
}

void stigi_lex_init(StigiLexer *lex, const char *src, size_t len) {
  if (lex == NULL) return;
  lex->src = src != NULL ? src : "";
  lex->len = len;
  lex->pos = 0;
  lex->line = 1;
  lex->col = 0;
  lex->err_msg = NULL;
}

static StigiTokKind classify_word(const char *s, uint32_t n) {
  switch (n) {
    case 2:
      if (s[0] == 'd' && s[1] == 'o') return TOK_KW_DO;
      if (s[0] == 'i' && s[1] == 'f') return TOK_KW_IF;
      return TOK_IDENT;
    case 3:
      if (memcmp(s, "def", 3) == 0) return TOK_KW_DEF;
      if (memcmp(s, "for", 3) == 0) return TOK_KW_FOR;
      if (memcmp(s, "not", 3) == 0) return TOK_KW_NOT;
      if (memcmp(s, "pub", 3) == 0) return TOK_KW_PUB;
      if (memcmp(s, "val", 3) == 0) return TOK_KW_VAL;
      if (memcmp(s, "var", 3) == 0) return TOK_KW_VAR;
      if (memcmp(s, "try", 3) == 0) return TOK_NOT_IN_LANG;
      if (memcmp(s, "let", 3) == 0) return TOK_NOT_IN_LANG;
      return TOK_IDENT;
    case 4:
      if (memcmp(s, "case", 4) == 0) return TOK_KW_CASE;
      if (memcmp(s, "else", 4) == 0) return TOK_KW_ELSE;
      if (memcmp(s, "enum", 4) == 0) return TOK_KW_ENUM;
      if (memcmp(s, "rule", 4) == 0) return TOK_KW_RULE;
      if (memcmp(s, "stop", 4) == 0) return TOK_KW_STOP;
      if (memcmp(s, "then", 4) == 0) return TOK_KW_THEN;
      if (memcmp(s, "true", 4) == 0) return TOK_KW_TRUE;
      if (memcmp(s, "type", 4) == 0) return TOK_KW_TYPE;
      if (memcmp(s, "null", 4) == 0) return TOK_NOT_IN_LANG;
      if (memcmp(s, "goto", 4) == 0) return TOK_NOT_IN_LANG;
      if (memcmp(s, "lazy", 4) == 0) return TOK_NOT_IN_LANG;
      return TOK_IDENT;
    case 5:
      if (memcmp(s, "alias", 5) == 0) return TOK_KW_ALIAS;
      if (memcmp(s, "false", 5) == 0) return TOK_KW_FALSE;
      if (memcmp(s, "lemma", 5) == 0) return TOK_KW_LEMMA;
      if (memcmp(s, "match", 5) == 0) return TOK_KW_MATCH;
      if (memcmp(s, "start", 5) == 0) return TOK_KW_START;
      if (memcmp(s, "state", 5) == 0) return TOK_KW_STATE;
      if (memcmp(s, "while", 5) == 0) return TOK_KW_WHILE;
      if (memcmp(s, "class", 5) == 0) return TOK_NOT_IN_LANG;
      if (memcmp(s, "trait", 5) == 0) return TOK_KW_TRAIT;
      if (memcmp(s, "given", 5) == 0) return TOK_KW_GIVEN;
      if (memcmp(s, "throw", 5) == 0) return TOK_NOT_IN_LANG;
      if (memcmp(s, "catch", 5) == 0) return TOK_NOT_IN_LANG;
      if (memcmp(s, "async", 5) == 0) return TOK_NOT_IN_LANG;
      if (memcmp(s, "await", 5) == 0) return TOK_NOT_IN_LANG;
      if (memcmp(s, "break", 5) == 0) return TOK_NOT_IN_LANG;
      if (memcmp(s, "macro", 5) == 0) return TOK_NOT_IN_LANG;
      return TOK_IDENT;
    case 6:
      if (memcmp(s, "extern", 6) == 0) return TOK_KW_EXTERN;
      if (memcmp(s, "handle", 6) == 0) return TOK_KW_HANDLE;
      if (memcmp(s, "object", 6) == 0) return TOK_NOT_IN_LANG;
      if (memcmp(s, "return", 6) == 0) return TOK_NOT_IN_LANG;
      if (memcmp(s, "import", 6) == 0) return TOK_NOT_IN_LANG;
      if (memcmp(s, "unsafe", 6) == 0) return TOK_NOT_IN_LANG;
      if (memcmp(s, "resume", 6) == 0) return TOK_NOT_IN_LANG;
      if (memcmp(s, "opaque", 6) == 0) return TOK_NOT_IN_LANG;
      if (memcmp(s, "lambda", 6) == 0) return TOK_NOT_IN_LANG;
      if (memcmp(s, "module", 6) == 0) return TOK_NOT_IN_LANG;
      return TOK_IDENT;
    case 7:
      if (memcmp(s, "handler", 7) == 0) return TOK_KW_HANDLER;
      return TOK_IDENT;
    case 8:
      if (memcmp(s, "relation", 8) == 0) return TOK_KW_RELATION;
      if (memcmp(s, "continue", 8) == 0) return TOK_NOT_IN_LANG;
      return TOK_IDENT;
    case 10:
      if (memcmp(s, "capability", 10) == 0) return TOK_KW_CAPABILITY;
      return TOK_IDENT;
    default:
      return TOK_IDENT;
  }
}

static StigiTok make_tok(StigiTokKind kind, const char *src, size_t start, size_t end,
                         uint32_t line, uint32_t col) {
  StigiTok t;
  t.kind = kind;
  t.span.offset = (uint32_t)start;
  t.span.len = (uint32_t)(end - start);
  t.span.line = line;
  t.span.col = col;
  t.ptr = src + start;
  t.len = (uint32_t)(end - start);
  return t;
}

StigiTok stigi_lex_next(StigiLexer *lex) {
  const char *s = lex->src;
  size_t n = lex->len;
  size_t p = lex->pos;

  /* Skip spaces (tabs are an error per language.md 1) */
  while (p < n && s[p] == ' ') {
    p++;
    lex->col++;
  }
  lex->pos = p;

  if (p >= n) {
    return make_tok(TOK_EOF, s, p, p, lex->line, lex->col);
  }

  size_t start = p;
  uint32_t line = lex->line;
  uint32_t col = lex->col;
  unsigned char c = (unsigned char)s[p];

  if (c == '\n') {
    lex->pos = p + 1;
    lex->line++;
    lex->col = 0;
    return make_tok(TOK_NEWLINE, s, start, p + 1, line, col);
  }

  if (c == '\r') {
    if (p + 1 < n && s[p + 1] == '\n') {
      lex->pos = p + 2;
      lex->line++;
      lex->col = 0;
      return make_tok(TOK_NEWLINE, s, start, p + 2, line, col);
    }
    lex->pos = p + 1;
    lex->col++;
    lex->err_msg = "carriage return without newline";
    return make_tok(TOK_ERROR, s, start, p + 1, line, col);
  }

  if (c == '\t') {
    lex->pos = p + 1;
    lex->col++;
    lex->err_msg = "tab character is not allowed; use spaces";
    return make_tok(TOK_ERROR, s, start, p + 1, line, col);
  }

  /* Comments and // prompt: */
  if (c == '/' && p + 1 < n && s[p + 1] == '/') {
    p += 2;
    while (p < n && s[p] != '\n' && s[p] != '\r') {
      p++;
    }
    size_t tok_len = p - start;
    lex->pos = p;
    lex->col += (uint32_t)tok_len;
    if (tok_len >= 10 && memcmp(s + start, "// prompt:", 10) == 0) {
      return make_tok(TOK_PROMPT, s, start, p, line, col);
    }
    return make_tok(TOK_COMMENT, s, start, p, line, col);
  }

  /* Underscore: standalone '_' only; leading '_' in identifier is invalid */
  if (c == '_') {
    if (p + 1 >= n || !is_ident_cont((unsigned char)s[p + 1])) {
      lex->pos = p + 1;
      lex->col++;
      return make_tok(TOK_UNDERSCORE, s, start, p + 1, line, col);
    }
    p++;
    while (p < n && is_ident_cont((unsigned char)s[p])) p++;
    lex->pos = p;
    lex->col += (uint32_t)(p - start);
    lex->err_msg = "identifier cannot start with '_'";
    return make_tok(TOK_ERROR, s, start, p, line, col);
  }

  /* Identifier, dotted name, keyword, or string interpolation s"..." */
  if (is_ascii_alpha(c)) {
    p++;
    bool has_dot = false;
    while (p < n && is_ident_cont((unsigned char)s[p])) {
      p++;
    }
    if (s[p - 1] == '_') {
      lex->pos = p;
      lex->col += (uint32_t)(p - start);
      lex->err_msg = "identifier cannot end with '_'";
      return make_tok(TOK_ERROR, s, start, p, line, col);
    }
    while (p + 1 < n && s[p] == '.' && is_ascii_alpha((unsigned char)s[p + 1])) {
      has_dot = true;
      p++;
      while (p < n && is_ident_cont((unsigned char)s[p])) {
        p++;
      }
      if (s[p - 1] == '_') {
        lex->pos = p;
        lex->col += (uint32_t)(p - start);
        lex->err_msg = "identifier cannot end with '_'";
        return make_tok(TOK_ERROR, s, start, p, line, col);
      }
    }
    uint32_t tok_len = (uint32_t)(p - start);
    StigiTokKind kind = has_dot ? TOK_IDENT : classify_word(s + start, tok_len);
    if (!has_dot && kind == TOK_IDENT && p < n && s[p] == '"') {
      p++;
      while (p < n && s[p] != '"' && s[p] != '\n' && s[p] != '\r') {
        if (s[p] == '\\' && p + 1 < n && s[p + 1] != '\n' && s[p + 1] != '\r') p += 2;
        else p++;
      }
      if (p < n && s[p] == '"') p++;
      lex->pos = p;
      lex->col += (uint32_t)(p - start);
      return make_tok(TOK_STR_INTERP, s, start, p, line, col);
    }
    lex->pos = p;
    lex->col += tok_len;
    return make_tok(kind, s, start, p, line, col);
  }

  /* Integer (or float -> not-in-language) */
  if (is_ascii_digit(c)) {
    if (c == '0' && p + 1 < n && (s[p + 1] == 'x' || s[p + 1] == 'X')) {
      p += 2;
      if (p >= n || !is_ascii_hex((unsigned char)s[p])) {
        lex->pos = p;
        lex->col += (uint32_t)(p - start);
        lex->err_msg = "invalid hex literal";
        return make_tok(TOK_ERROR, s, start, p, line, col);
      }
      while (p < n && is_ascii_hex((unsigned char)s[p])) {
        p++;
      }
      if (p < n && (is_ascii_alpha((unsigned char)s[p]) || s[p] == '_')) {
        while (p < n && is_ident_cont((unsigned char)s[p])) p++;
        lex->pos = p;
        lex->col += (uint32_t)(p - start);
        lex->err_msg = "invalid hex integer literal suffix";
        return make_tok(TOK_ERROR, s, start, p, line, col);
      }
      lex->pos = p;
      lex->col += (uint32_t)(p - start);
      return make_tok(TOK_INT, s, start, p, line, col);
    }
    p++;
    while (p < n && is_ascii_digit((unsigned char)s[p])) {
      p++;
    }
    if (p + 1 < n && s[p] == '.' && is_ascii_digit((unsigned char)s[p + 1])) {
      p++;
      while (p < n && is_ascii_digit((unsigned char)s[p])) {
        p++;
      }
      lex->pos = p;
      lex->col += (uint32_t)(p - start);
      return make_tok(TOK_FLOAT, s, start, p, line, col);
    }
    if (p < n && (is_ascii_alpha((unsigned char)s[p]) || s[p] == '_')) {
      while (p < n && is_ident_cont((unsigned char)s[p])) p++;
      lex->pos = p;
      lex->col += (uint32_t)(p - start);
      lex->err_msg = "invalid integer literal suffix";
      return make_tok(TOK_ERROR, s, start, p, line, col);
    }
    lex->pos = p;
    lex->col += (uint32_t)(p - start);
    return make_tok(TOK_INT, s, start, p, line, col);
  }

  /* String literal (language.md 1: escapes \n \t \\ \" \' \0 \xHH only; no raw control chars) */
  if (c == '"') {
    p++;
    while (p < n && s[p] != '"' && s[p] != '\n' && s[p] != '\r') {
      if ((unsigned char)s[p] < 0x20U) {
        lex->pos = p + 1;
        lex->col += (uint32_t)(lex->pos - start);
        lex->err_msg = "unescaped control character in string literal";
        return make_tok(TOK_ERROR, s, start, lex->pos, line, col);
      }
      if (s[p] == '\\') {
        p++;
        if (p >= n || s[p] == '\n' || s[p] == '\r') {
          lex->pos = p;
          lex->col += (uint32_t)(p - start);
          lex->err_msg = "unterminated escape in string literal";
          return make_tok(TOK_ERROR, s, start, p, line, col);
        }
        char esc = s[p];
        if (esc == 'x') {
          if (p + 2 >= n || !is_ascii_hex((unsigned char)s[p + 1]) ||
              !is_ascii_hex((unsigned char)s[p + 2])) {
            lex->pos = p + 1;
            lex->col += (uint32_t)(lex->pos - start);
            lex->err_msg = "invalid hex escape in string literal";
            return make_tok(TOK_ERROR, s, start, lex->pos, line, col);
          }
          p += 3;
          continue;
        }
        if (esc == 'n' || esc == 't' || esc == '\\' || esc == '"' || esc == '\'' || esc == '0') {
          p++;
          continue;
        }
        lex->pos = p + 1;
        lex->col += (uint32_t)(lex->pos - start);
        lex->err_msg = "invalid escape sequence in string literal";
        return make_tok(TOK_ERROR, s, start, lex->pos, line, col);
      }
      p++;
    }
    if (p >= n || s[p] != '"') {
      lex->pos = p;
      lex->col += (uint32_t)(p - start);
      lex->err_msg = "unterminated string literal";
      return make_tok(TOK_ERROR, s, start, p, line, col);
    }
    p++;
    lex->pos = p;
    lex->col += (uint32_t)(p - start);
    return make_tok(TOK_STRING, s, start, p, line, col);
  }

  /* Byte (char) literal */
  if (c == '\'') {
    p++;
    if (p >= n || s[p] == '\n' || s[p] == '\r' || s[p] == '\'' || (unsigned char)s[p] < 0x20U) {
      lex->pos = p;
      lex->col += (uint32_t)(p - start);
      lex->err_msg = "invalid byte literal";
      return make_tok(TOK_ERROR, s, start, p, line, col);
    }
    if (s[p] == '\\') {
      p++;
      if (p >= n || s[p] == '\n' || s[p] == '\r') {
        lex->pos = p;
        lex->col += (uint32_t)(p - start);
        lex->err_msg = "unterminated byte literal";
        return make_tok(TOK_ERROR, s, start, p, line, col);
      }
      char esc = s[p];
      if (esc == 'x') {
        if (p + 2 >= n || !is_ascii_hex((unsigned char)s[p + 1]) ||
            !is_ascii_hex((unsigned char)s[p + 2])) {
          lex->pos = p + 1;
          lex->col += (uint32_t)(lex->pos - start);
          lex->err_msg = "invalid hex escape in byte literal";
          return make_tok(TOK_ERROR, s, start, lex->pos, line, col);
        }
        p += 3;
      } else if (esc == 'n' || esc == 't' || esc == '\\' || esc == '"' || esc == '\'' || esc == '0') {
        p++;
      } else {
        lex->pos = p + 1;
        lex->col += (uint32_t)(lex->pos - start);
        lex->err_msg = "invalid escape sequence in byte literal";
        return make_tok(TOK_ERROR, s, start, lex->pos, line, col);
      }
    } else {
      p++;
    }
    if (p >= n || s[p] != '\'') {
      lex->pos = p;
      lex->col += (uint32_t)(p - start);
      lex->err_msg = "unterminated byte literal";
      return make_tok(TOK_ERROR, s, start, p, line, col);
    }
    p++;
    lex->pos = p;
    lex->col += (uint32_t)(p - start);
    return make_tok(TOK_CHAR, s, start, p, line, col);
  }

  /* Punctuation and operators */
  StigiTokKind kind = TOK_ERROR;
  size_t advance = 1;
  switch (c) {
    case '(': kind = TOK_LPAREN; break;
    case ')': kind = TOK_RPAREN; break;
    case '[': kind = TOK_LBRACKET; break;
    case ']': kind = TOK_RBRACKET; break;
    case '{': kind = TOK_LBRACE; break;
    case '}': kind = TOK_RBRACE; break;
    case ',': kind = TOK_COMMA; break;
    case '@': kind = TOK_AT; break;
    case '+': kind = TOK_PLUS; break;
    case '-':
      if (p + 1 < n && s[p + 1] == '>') {
        kind = TOK_RARROW;
        advance = 2;
      } else {
        kind = TOK_MINUS;
      }
      break;
    case '*': kind = TOK_STAR; break;
    case '/': kind = TOK_SLASH; break;
    case '%': kind = TOK_PERCENT; break;
    case '#': kind = TOK_HASH; break;
    case '.':
      if (p + 2 < n && s[p + 1] == '.' && s[p + 2] == '.') {
        kind = TOK_ELLIPSIS;
        advance = 3;
      } else {
        lex->err_msg = "unexpected '.'";
      }
      break;
    case ':':
      if (p + 1 < n && s[p + 1] == '=') {
        kind = TOK_COLON_EQ;
        advance = 2;
      } else {
        kind = TOK_COLON;
      }
      break;
    case '=':
      if (p + 1 < n && s[p + 1] == '=') {
        kind = TOK_EQ;
        advance = 2;
      } else if (p + 2 < n && s[p + 1] == '>' && s[p + 2] == '>') {
        kind = TOK_TYPE_ARROW;
        advance = 3;
      } else if (p + 1 < n && s[p + 1] == '>') {
        kind = TOK_ARROW;
        advance = 2;
      } else {
        kind = TOK_ASSIGN;
      }
      break;
    case '<':
      if (p + 1 < n && s[p + 1] == '=') {
        kind = TOK_LE;
        advance = 2;
      } else {
        kind = TOK_LT;
      }
      break;
    case '>':
      if (p + 1 < n && s[p + 1] == '=') {
        kind = TOK_GE;
        advance = 2;
      } else {
        kind = TOK_GT;
      }
      break;
    case '!':
      if (p + 1 < n && s[p + 1] == '=') {
        kind = TOK_NE;
        advance = 2;
      } else {
        kind = TOK_BANG;
      }
      break;
    case '&':
      if (p + 1 < n && s[p + 1] == '&') {
        kind = TOK_AND;
        advance = 2;
      } else {
        lex->err_msg = "unexpected '&'; use '&&' for logical and";
      }
      break;
    case '|':
      if (p + 1 < n && s[p + 1] == '|') {
        kind = TOK_OR;
        advance = 2;
      } else if (p + 1 < n && s[p + 1] == '>') {
        kind = TOK_PIPE_GT;
        advance = 2;
      } else {
        lex->err_msg = "unexpected '|'; use '||' for logical or or '|>' for pipe";
      }
      break;
    default:
      lex->err_msg = "unexpected character";
      break;
  }

  lex->pos = p + advance;
  lex->col += (uint32_t)advance;
  return make_tok(kind, s, start, p + advance, line, col);
}

Status stigi_lex_all(const char *src, size_t len, StigiTok **out_toks, uint32_t *out_count,
                     Arena *arena, Diag *diag) {
  if (out_toks == NULL || out_count == NULL || arena == NULL || (src == NULL && len > 0)) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_lex_all: invalid arguments");
  }
  if (len > (size_t)UINT32_MAX) {
    return diag_set(diag, STIGI_ERR_OVERFLOW, -1, "stigi_lex_all: source exceeds 4 GiB");
  }
  StigiLexer lex;
  stigi_lex_init(&lex, src, len);

  size_t cap_sz = (len / 4U) + 32U;
  if (cap_sz < 64U) cap_sz = 64U;
  if (cap_sz > (1U << 20)) cap_sz = (1U << 20);
  uint32_t cap = (uint32_t)cap_sz;
  StigiTok *buf = malloc((size_t)cap * sizeof *buf);
  if (buf == NULL) {
    return diag_set(diag, STIGI_ERR_OOM, -1, "stigi_lex_all: out of memory");
  }
  uint32_t count = 0;

  for (;;) {
    StigiTok t = stigi_lex_next(&lex);
    if (count == cap) {
      if (cap > UINT32_MAX / 2U) {
        free(buf);
        return diag_set(diag, STIGI_ERR_OVERFLOW, -1, "stigi_lex_all: too many tokens");
      }
      uint32_t ncap = cap * 2U;
      StigiTok *nb = realloc(buf, (size_t)ncap * sizeof *nb);
      if (nb == NULL) {
        free(buf);
        return diag_set(diag, STIGI_ERR_OOM, -1, "stigi_lex_all: out of memory");
      }
      buf = nb;
      cap = ncap;
    }
    buf[count++] = t;
    if (t.kind == TOK_ERROR) {
      Status st = diag_set(diag, STIGI_ERR_INVALID, -1, "lex error at %u:%u: %s",
                           t.span.line, t.span.col,
                           lex.err_msg != NULL ? lex.err_msg : "invalid token");
      free(buf);
      return st;
    }
    if (t.kind == TOK_EOF) break;
  }

  StigiTok *res = arena_alloc(arena, (size_t)count * sizeof *res);
  if (res == NULL) {
    free(buf);
    return diag_set(diag, STIGI_ERR_OOM, -1, "stigi_lex_all: arena out of memory");
  }
  memcpy(res, buf, (size_t)count * sizeof *res);
  free(buf);
  *out_toks = res;
  *out_count = count;
  return STIGI_OK;
}

const char *stigi_tok_kind_name(StigiTokKind kind) {
  switch (kind) {
    case TOK_EOF: return "EOF";
    case TOK_NEWLINE: return "NEWLINE";
    case TOK_PROMPT: return "PROMPT";
    case TOK_COMMENT: return "COMMENT";
    case TOK_IDENT: return "IDENT";
    case TOK_UNDERSCORE: return "_";
    case TOK_INT: return "INT";
    case TOK_CHAR: return "CHAR";
    case TOK_STRING: return "STRING";
    case TOK_FLOAT: return "FLOAT";
    case TOK_HASH: return "#";
    case TOK_STR_INTERP: return "STRING_INTERPOLATION";
    case TOK_RARROW: return "->";
    case TOK_TYPE_ARROW: return "=>>";
    case TOK_ELLIPSIS: return "...";
    case TOK_NOT_IN_LANG: return "NOT_IN_LANG";
    case TOK_ERROR: return "ERROR";
    case TOK_KW_ALIAS: return "alias";
    case TOK_KW_CAPABILITY: return "capability";
    case TOK_KW_CASE: return "case";
    case TOK_KW_DEF: return "def";
    case TOK_KW_DO: return "do";
    case TOK_KW_ELSE: return "else";
    case TOK_KW_ENUM: return "enum";
    case TOK_KW_EXTERN: return "extern";
    case TOK_KW_FALSE: return "false";
    case TOK_KW_FOR: return "for";
    case TOK_KW_GIVEN: return "given";
    case TOK_KW_HANDLE: return "handle";
    case TOK_KW_HANDLER: return "handler";
    case TOK_KW_IF: return "if";
    case TOK_KW_LEMMA: return "lemma";
    case TOK_KW_MATCH: return "match";
    case TOK_KW_NOT: return "not";
    case TOK_KW_PUB: return "pub";
    case TOK_KW_RELATION: return "relation";
    case TOK_KW_RULE: return "rule";
    case TOK_KW_START: return "start";
    case TOK_KW_STATE: return "state";
    case TOK_KW_STOP: return "stop";
    case TOK_KW_THEN: return "then";
    case TOK_KW_TRAIT: return "trait";
    case TOK_KW_TRUE: return "true";
    case TOK_KW_TYPE: return "type";
    case TOK_KW_VAL: return "val";
    case TOK_KW_VAR: return "var";
    case TOK_KW_WHILE: return "while";
    case TOK_LPAREN: return "(";
    case TOK_RPAREN: return ")";
    case TOK_LBRACKET: return "[";
    case TOK_RBRACKET: return "]";
    case TOK_LBRACE: return "{";
    case TOK_RBRACE: return "}";
    case TOK_COMMA: return ",";
    case TOK_COLON: return ":";
    case TOK_ASSIGN: return "=";
    case TOK_COLON_EQ: return ":=";
    case TOK_ARROW: return "=>";
    case TOK_AT: return "@";
    case TOK_PLUS: return "+";
    case TOK_MINUS: return "-";
    case TOK_STAR: return "*";
    case TOK_SLASH: return "/";
    case TOK_PERCENT: return "%";
    case TOK_LT: return "<";
    case TOK_LE: return "<=";
    case TOK_GT: return ">";
    case TOK_GE: return ">=";
    case TOK_EQ: return "==";
    case TOK_NE: return "!=";
    case TOK_AND: return "&&";
    case TOK_OR: return "||";
    case TOK_PIPE_GT: return "|>";
    case TOK_BANG: return "!";
  }
  return "UNKNOWN";
}
