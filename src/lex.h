#ifndef STIGI_LEX_H
#define STIGI_LEX_H

#include "ast.h"
#include "common.h"

#include <stddef.h>
#include <stdint.h>

typedef enum {
  TOK_EOF = 0,
  TOK_NEWLINE,
  TOK_PROMPT,      /* // prompt:... */
  TOK_COMMENT,     /* // ... */
  TOK_IDENT,       /* Foo, bar, A.B.c */
  TOK_UNDERSCORE,  /* _ */
  TOK_INT,         /* 123, 0x1a */
  TOK_CHAR,        /* 'a' */
  TOK_STRING,      /* "abc" */
  TOK_FLOAT,       /* 1.5 -> not-in-language */
  TOK_HASH,        /* # -> not-in-language */
  TOK_NOT_IN_LANG, /* class, trait, null, throw, try, catch, return, import, ... */
  TOK_ERROR,

  /* L0 Keywords */
  TOK_KW_ALIAS,
  TOK_KW_CAPABILITY,
  TOK_KW_CASE,
  TOK_KW_DEF,
  TOK_KW_DO,
  TOK_KW_ELSE,
  TOK_KW_ENUM,
  TOK_KW_EXTERN,
  TOK_KW_FALSE,
  TOK_KW_FOR,
  TOK_KW_HANDLE,
  TOK_KW_HANDLER,
  TOK_KW_IF,
  TOK_KW_LEMMA,
  TOK_KW_MATCH,
  TOK_KW_NOT,
  TOK_KW_PUB,
  TOK_KW_RELATION,
  TOK_KW_RULE,
  TOK_KW_START,
  TOK_KW_STATE,
  TOK_KW_STOP,
  TOK_KW_THEN,
  TOK_KW_TRUE,
  TOK_KW_TYPE,
  TOK_KW_VAL,
  TOK_KW_VAR,
  TOK_KW_WHILE,

  /* Punctuation & Operators */
  TOK_LPAREN,    /* ( */
  TOK_RPAREN,    /* ) */
  TOK_LBRACKET,  /* [ */
  TOK_RBRACKET,  /* ] */
  TOK_LBRACE,    /* { */
  TOK_RBRACE,    /* } */
  TOK_COMMA,     /* , */
  TOK_COLON,     /* : */
  TOK_ASSIGN,    /* = */
  TOK_COLON_EQ,  /* := */
  TOK_ARROW,     /* => */
  TOK_AT,        /* @ */
  TOK_PLUS,      /* + */
  TOK_MINUS,     /* - */
  TOK_STAR,      /* * */
  TOK_SLASH,     /* / */
  TOK_PERCENT,   /* % */
  TOK_LT,        /* < */
  TOK_LE,        /* <= */
  TOK_GT,        /* > */
  TOK_GE,        /* >= */
  TOK_EQ,        /* == */
  TOK_NE,        /* != */
  TOK_AND,       /* && */
  TOK_OR,        /* || */
  TOK_BANG       /* ! */
} StigiTokKind;

typedef struct {
  StigiTokKind kind;
  StigiSpan span;
  const char *ptr;
  uint32_t len;
} StigiTok;

typedef struct {
  const char *src;
  size_t len;
  size_t pos;
  uint32_t line;
  uint32_t col;
  const char *err_msg;
} StigiLexer;

void stigi_lex_init(StigiLexer *lex, const char *src, size_t len);
StigiTok stigi_lex_next(StigiLexer *lex);
Status stigi_lex_all(const char *src, size_t len, StigiTok **out_toks, uint32_t *out_count,
                     Arena *arena, Diag *diag);
const char *stigi_tok_kind_name(StigiTokKind kind);

#endif
