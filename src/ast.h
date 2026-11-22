#ifndef STIGI_AST_H
#define STIGI_AST_H

#include "common.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
  uint32_t offset;
  uint32_t len;
  uint32_t line; /* 1-based */
  uint32_t col;  /* 0-based */
} StigiSpan;

typedef struct {
  const char *ptr;
  uint32_t len;
} StigiStr;

typedef struct StigiType StigiType;
typedef struct StigiExpr StigiExpr;
typedef struct StigiDecl StigiDecl;

/* ---- Types ---- */

typedef enum {
  STIGI_TYPE_NAMED = 0,
  STIGI_TYPE_ARRAY,
  STIGI_TYPE_CELL,
  STIGI_TYPE_PTR
} StigiTypeKind;

struct StigiType {
  StigiTypeKind kind;
  StigiSpan span;
  StigiStr name;
  StigiType **type_args;
  uint32_t type_arg_count;
  StigiExpr **len_args;
  uint32_t len_arg_count;
  StigiStr cap_arg; /* optional {r} */
};

/* ---- Fact groups ---- */

typedef enum {
  STIGI_FACT_ATOM = 0,
  STIGI_FACT_CMP,
  STIGI_FACT_ARITH,
  STIGI_FACT_DENY
} StigiFactKind;

typedef enum {
  STIGI_OP_ADD = 0,
  STIGI_OP_SUB,
  STIGI_OP_MUL,
  STIGI_OP_DIV,
  STIGI_OP_MOD,
  STIGI_OP_LT,
  STIGI_OP_LE,
  STIGI_OP_GT,
  STIGI_OP_GE,
  STIGI_OP_EQ,
  STIGI_OP_NE,
  STIGI_OP_AND,
  STIGI_OP_OR,
  STIGI_OP_NOT,
  STIGI_OP_NEG
} StigiOp;

typedef struct StigiFactItem StigiFactItem;

struct StigiFactItem {
  StigiFactKind kind;
  StigiSpan span;
  bool is_negated;
  StigiStr evidence_name;     /* optional <p: atom> */
  StigiStr atom_name;         /* for ATOM */
  StigiExpr **args;           /* for ATOM */
  uint32_t arg_count;
  StigiStr lhs_var;           /* for ARITH: name = lhs op rhs */
  StigiExpr *lhs;             /* for CMP / ARITH */
  StigiOp op;
  StigiExpr *rhs;
  StigiFactItem *deny_atoms;  /* for DENY: not (a1, a2, ...) */
  uint32_t deny_atom_count;
};

typedef struct {
  StigiSpan span;
  StigiFactItem *items;
  uint32_t count;
} StigiFactGroup;

/* ---- Telescope parameters ---- */

typedef struct {
  StigiSpan span;
  StigiStr name;
  StigiType *type;
} StigiParam;

typedef struct {
  StigiSpan span;
  StigiStr name; /* empty if unnamed, e.g. {Stigi.Mut} */
  StigiType *cap_type;
} StigiCapParam;

typedef struct {
  StigiSpan span;
  bool is_fact; /* false = cap group {..}, true = fact group <..> */
  StigiCapParam *caps;
  uint32_t cap_count;
  StigiFactGroup facts;
} StigiHeadGroup;

/* ---- Patterns & Match Arms ---- */

typedef enum {
  STIGI_PAT_WILD = 0,
  STIGI_PAT_VAR,
  STIGI_PAT_INT,
  STIGI_PAT_BOOL,
  STIGI_PAT_CTOR
} StigiPatKind;

typedef struct {
  StigiPatKind kind;
  StigiSpan span;
  StigiStr name;
  uint64_t int_val;
  bool int_overflow;
  bool bool_val;
  StigiStr *fields; /* for CTOR: name or "_" */
  uint32_t field_count;
  bool has_parens;
} StigiPat;

typedef struct {
  StigiSpan span;
  StigiPat pat;
  StigiExpr *body;
} StigiMatchArm;

/* ---- Expressions & Statements ---- */

typedef enum {
  STIGI_EXPR_LOCAL = 0,
  STIGI_EXPR_GLOBAL,
  STIGI_EXPR_LIT_INT,
  STIGI_EXPR_LIT_CHAR,
  STIGI_EXPR_LIT_STR,
  STIGI_EXPR_LIT_BOOL,
  STIGI_EXPR_LIT_UNIT,
  STIGI_EXPR_CALL,
  STIGI_EXPR_BINOP,
  STIGI_EXPR_UNOP,
  STIGI_EXPR_IF,
  STIGI_EXPR_WHILE,
  STIGI_EXPR_MATCH,
  STIGI_EXPR_VAL,
  STIGI_EXPR_VAR,
  STIGI_EXPR_ASSIGN,
  STIGI_EXPR_INDEX_READ,
  STIGI_EXPR_INDEX_WRITE,
  STIGI_EXPR_HANDLE,
  STIGI_EXPR_ASSERT,
  STIGI_EXPR_DENY,
  STIGI_EXPR_BLOCK,
  STIGI_EXPR_LOCAL_DECL
} StigiExprKind;

struct StigiExpr {
  StigiExprKind kind;
  StigiSpan span;
  union {
    struct {
      StigiStr name;
    } ident;
    struct {
      uint64_t value;
      StigiStr raw;
      bool overflow;
    } lit_int;
    struct {
      uint8_t value;
      StigiStr raw;
    } lit_char;
    struct {
      StigiStr raw; /* includes quotes as written */
      uint32_t decoded_len;
    } lit_str;
    struct {
      bool value;
    } lit_bool;
    struct {
      StigiExpr *callee;
      StigiType **type_args;
      uint32_t type_arg_count;
      StigiStr *cap_args;
      uint32_t cap_arg_count;
      StigiExpr **args;
      uint32_t arg_count;
      bool has_parens;
    } call;
    struct {
      StigiOp op;
      StigiExpr *lhs;
      StigiExpr *rhs;
    } binop;
    struct {
      StigiOp op;
      StigiExpr *operand;
    } unop;
    struct {
      StigiExpr *cond;
      StigiExpr *then_br;
      StigiExpr *else_br;
    } if_expr;
    struct {
      StigiExpr *cond;
      StigiExpr *decreases; /* optional */
      StigiExpr *body;
    } while_expr;
    struct {
      StigiExpr *scrutinee;
      StigiMatchArm *arms;
      uint32_t arm_count;
    } match_expr;
    struct {
      StigiPat pat;
      StigiType *type; /* optional */
      StigiExpr *init;
    } val_stmt;
    struct {
      StigiStr name;
      bool has_inv;
      StigiFactGroup inv; /* var x@<..> */
      StigiType *type;    /* optional */
      StigiExpr *init;
    } var_stmt;
    struct {
      StigiStr name;
      StigiExpr *value;
    } assign_stmt;
    struct {
      StigiExpr *target;
      StigiExpr *index;
      StigiExpr *value; /* non-NULL for INDEX_WRITE */
    } index_expr;
    struct {
      StigiStr binder; /* optional */
      StigiStr handler_name;
      StigiType **type_args;
      uint32_t type_arg_count;
      StigiStr *cap_args;
      uint32_t cap_arg_count;
      StigiExpr **args;
      uint32_t arg_count;
      bool has_parens;
      StigiExpr *body; /* optional block */
    } handle_stmt;
    struct {
      StigiExpr *expr; /* NULL for bare @<..>, non-NULL for expr@<..> */
      StigiFactGroup facts;
    } assert_expr;
    struct {
      StigiFactItem item;
    } deny_expr;
    struct {
      StigiExpr **stmts;
      uint32_t count;
    } block;
    struct {
      StigiDecl *decl;
    } local_decl;
  } u;
};

/* ---- Declarations ---- */

typedef enum {
  STIGI_DECL_ENUM = 0,
  STIGI_DECL_TYPE,
  STIGI_DECL_DEF,
  STIGI_DECL_LEMMA_DEF,
  STIGI_DECL_VAL,
  STIGI_DECL_CAPABILITY,
  STIGI_DECL_HANDLER,
  STIGI_DECL_EXTERN,
  STIGI_DECL_RELATION,
  STIGI_DECL_RULE,
  STIGI_DECL_TEST,
  STIGI_DECL_NEG_TEST
} StigiDeclKind;

typedef struct {
  StigiSpan span;
  StigiStr name;
  StigiParam *fields;
  uint32_t field_count;
  bool has_parens;
  bool has_facts;
  StigiFactGroup facts;
} StigiEnumCase;

typedef struct {
  StigiParam *type_params;
  uint32_t type_param_count;
  StigiHeadGroup *pre_groups;
  uint32_t pre_group_count;
  StigiParam *params;
  uint32_t param_count;
  bool has_val_group;
  StigiFactGroup *post_val_facts;
  uint32_t post_val_fact_count;
  StigiType *ret_type; /* NULL for lemma def */
  StigiFactGroup *exit_facts;
  uint32_t exit_fact_count;
  StigiExpr *decreases; /* optional */
} StigiDefHead;

struct StigiDecl {
  StigiDeclKind kind;
  StigiSpan span;
  StigiStr name;
  bool is_pub;
  union {
    struct {
      StigiDefHead head;
      StigiExpr *body; /* NULL for capability op or bodyless extern */
    } def_decl;
    struct {
      StigiCapParam *caps;
      uint32_t cap_count;
      StigiType *type;
      StigiExpr *body;
    } val_decl;
    struct {
      StigiParam *type_params;
      uint32_t type_param_count;
      StigiEnumCase *cases;
      uint32_t case_count;
    } enum_decl;
    struct {
      StigiParam *type_params;
      uint32_t type_param_count;
      StigiParam *val_params;
      uint32_t val_param_count;
      StigiCapParam *cap_params;
      uint32_t cap_param_count;
      StigiType *rhs;
      bool has_facts;
      StigiFactGroup facts;
    } type_decl;
    struct {
      StigiParam *type_params;
      uint32_t type_param_count;
      StigiStr *states;
      uint32_t state_count;
      StigiStr start_state;
      StigiStr *stop_states;
      uint32_t stop_state_count;
      StigiDecl **members; /* operations (DECL_DEF without body) or rules */
      uint32_t member_count;
    } cap_decl;
    struct {
      StigiParam *type_params;
      uint32_t type_param_count;
      StigiCapParam *cap_params;
      uint32_t cap_param_count;
      StigiParam *val_params;
      uint32_t val_param_count;
      bool has_val_parens;
      StigiStr for_cap;
      StigiType *for_cap_type;
      StigiExpr **vars; /* STIGI_EXPR_VAR */
      uint32_t var_count;
      StigiDecl **defs; /* STIGI_DECL_DEF */
      uint32_t def_count;
    } handler_decl;
    struct {
      StigiStr abi;
      bool is_val;
      StigiDefHead head; /* for extern def */
      StigiType *val_type; /* for extern val */
      StigiExpr *body; /* optional for extern def */
    } extern_decl;
    struct {
      StigiParam *columns;
      uint32_t column_count;
    } relation_decl;
    struct {
      StigiFactItem head;
      StigiFactItem *body;
      uint32_t body_count;
      bool body_multiline;
    } rule_decl;
  } u;
};

/* ---- Unit ---- */

typedef struct {
  StigiSpan span;
  StigiStr name;
  StigiStr target;
} StigiAlias;

typedef struct {
  StigiStr prompt; /* text after "// prompt:" (without leading space if present) */
  bool has_prompt;
  StigiAlias *aliases;
  uint32_t alias_count;
  StigiDecl *decl;
} StigiUnit;

#endif
