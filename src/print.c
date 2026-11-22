#include "print.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
  char *buf;
  size_t len;
  size_t cap;
  bool oom;
} PrintBuf;

static void pb_grow(PrintBuf *b, size_t add) {
  if (b->oom) return;
  if (add > SIZE_MAX - b->len - 1U) {
    b->oom = true;
    return;
  }
  size_t need = b->len + add + 1U;
  if (need <= b->cap) return;
  size_t ncap = b->cap == 0 ? 256U : b->cap;
  while (ncap < need) {
    if (ncap > SIZE_MAX / 2U) {
      ncap = need;
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

static void pb_mem(PrintBuf *b, const char *s, size_t n) {
  if (n == 0 || b->oom) return;
  pb_grow(b, n);
  if (b->oom) return;
  memcpy(b->buf + b->len, s, n);
  b->len += n;
  b->buf[b->len] = '\0';
}

static void pb_cstr(PrintBuf *b, const char *s) {
  if (s == NULL) return;
  pb_mem(b, s, strlen(s));
}

static void pb_str(PrintBuf *b, StigiStr s) {
  if (s.ptr == NULL || s.len == 0) return;
  pb_mem(b, s.ptr, s.len);
}

static void pb_char(PrintBuf *b, char c) {
  pb_mem(b, &c, 1);
}

static void pb_indent(PrintBuf *b, uint32_t indent) {
  for (uint32_t i = 0; i < indent; i++) {
    pb_char(b, ' ');
  }
}

/* Forward declarations */
static void print_type(PrintBuf *b, const StigiType *ty);
static void print_expr(PrintBuf *b, const StigiExpr *e, uint32_t indent);
static void print_decl(PrintBuf *b, const StigiDecl *d, uint32_t indent);

static const char *op_str(StigiOp op) {
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
  return "?";
}

static int op_prec(StigiOp op) {
  switch (op) {
    case STIGI_OP_OR: return 1;
    case STIGI_OP_AND: return 2;
    case STIGI_OP_EQ:
    case STIGI_OP_NE:
    case STIGI_OP_LT:
    case STIGI_OP_LE:
    case STIGI_OP_GT:
    case STIGI_OP_GE: return 3;
    case STIGI_OP_ADD:
    case STIGI_OP_SUB: return 4;
    case STIGI_OP_MUL:
    case STIGI_OP_DIV:
    case STIGI_OP_MOD: return 5;
    default: return 6;
  }
}

static void print_type(PrintBuf *b, const StigiType *ty) {
  if (ty == NULL) return;
  pb_str(b, ty->name);
  if (ty->type_arg_count > 0) {
    pb_char(b, '[');
    for (uint32_t i = 0; i < ty->type_arg_count; i++) {
      if (i > 0) pb_cstr(b, ", ");
      print_type(b, ty->type_args[i]);
    }
    pb_char(b, ']');
  }
  if (ty->len_arg_count > 0) {
    pb_char(b, '(');
    for (uint32_t i = 0; i < ty->len_arg_count; i++) {
      if (i > 0) pb_cstr(b, ", ");
      print_expr(b, ty->len_args[i], 0);
    }
    pb_char(b, ')');
  }
  if (ty->cap_arg.len > 0) {
    pb_char(b, '{');
    pb_str(b, ty->cap_arg);
    pb_char(b, '}');
  }
}

static void print_fact_term_expr(PrintBuf *b, const StigiExpr *e) {
  if (e == NULL) return;
  bool par = (e->kind == STIGI_EXPR_BINOP || e->kind == STIGI_EXPR_IF ||
              e->kind == STIGI_EXPR_WHILE || e->kind == STIGI_EXPR_MATCH ||
              e->kind == STIGI_EXPR_BLOCK);
  if (par) pb_char(b, '(');
  print_expr(b, e, 0);
  if (par) pb_char(b, ')');
}

static void print_fact_item(PrintBuf *b, const StigiFactItem *it) {
  if (it->kind == STIGI_FACT_DENY) {
    pb_cstr(b, "not (");
    for (uint32_t i = 0; i < it->deny_atom_count; i++) {
      if (i > 0) pb_cstr(b, ", ");
      print_fact_item(b, &it->deny_atoms[i]);
    }
    pb_char(b, ')');
    return;
  }
  if (it->is_negated) pb_cstr(b, "not ");
  if (it->evidence_name.len > 0) {
    pb_str(b, it->evidence_name);
    pb_cstr(b, ": ");
  }
  if (it->kind == STIGI_FACT_ATOM) {
    pb_str(b, it->atom_name);
    pb_char(b, '(');
    for (uint32_t i = 0; i < it->arg_count; i++) {
      if (i > 0) pb_cstr(b, ", ");
      print_expr(b, it->args[i], 0);
    }
    pb_char(b, ')');
  } else if (it->kind == STIGI_FACT_CMP) {
    print_fact_term_expr(b, it->lhs);
    pb_char(b, ' ');
    pb_cstr(b, op_str(it->op));
    pb_char(b, ' ');
    print_fact_term_expr(b, it->rhs);
  } else if (it->kind == STIGI_FACT_ARITH) {
    pb_str(b, it->lhs_var);
    pb_cstr(b, " = ");
    print_fact_term_expr(b, it->lhs);
    pb_char(b, ' ');
    pb_cstr(b, op_str(it->op));
    pb_char(b, ' ');
    print_fact_term_expr(b, it->rhs);
  }
}

static void print_fact_group(PrintBuf *b, const StigiFactGroup *fg) {
  pb_char(b, '<');
  for (uint32_t i = 0; i < fg->count; i++) {
    if (i > 0) pb_cstr(b, ", ");
    print_fact_item(b, &fg->items[i]);
  }
  pb_char(b, '>');
}

static void print_params(PrintBuf *b, char open_c, char close_c,
                         const StigiParam *params, uint32_t count) {
  pb_char(b, open_c);
  for (uint32_t i = 0; i < count; i++) {
    if (i > 0) pb_cstr(b, ", ");
    pb_str(b, params[i].name);
    pb_cstr(b, ": ");
    print_type(b, params[i].type);
  }
  pb_char(b, close_c);
}

static void print_cap_group(PrintBuf *b, const StigiCapParam *caps, uint32_t count) {
  pb_char(b, '{');
  for (uint32_t i = 0; i < count; i++) {
    if (i > 0) pb_cstr(b, ", ");
    if (caps[i].name.len > 0) {
      pb_str(b, caps[i].name);
      pb_cstr(b, ": ");
    }
    print_type(b, caps[i].cap_type);
  }
  pb_char(b, '}');
}

static void print_pat(PrintBuf *b, const StigiPat *pat) {
  pb_str(b, pat->name);
  if (pat->kind == STIGI_PAT_CTOR && pat->has_parens) {
    pb_char(b, '(');
    for (uint32_t i = 0; i < pat->field_count; i++) {
      if (i > 0) pb_cstr(b, ", ");
      pb_str(b, pat->fields[i]);
    }
    pb_char(b, ')');
  }
}

static bool expr_is_multiline(const StigiExpr *e) {
  if (e == NULL) return false;
  switch (e->kind) {
    case STIGI_EXPR_BLOCK:
    case STIGI_EXPR_MATCH:
    case STIGI_EXPR_WHILE:
      return true;
    case STIGI_EXPR_IF:
      return expr_is_multiline(e->u.if_expr.then_br) ||
             expr_is_multiline(e->u.if_expr.else_br);
    case STIGI_EXPR_HANDLE:
      return e->u.handle_stmt.body != NULL && expr_is_multiline(e->u.handle_stmt.body);
    case STIGI_EXPR_VAL:
      return expr_is_multiline(e->u.val_stmt.init);
    case STIGI_EXPR_VAR:
      return expr_is_multiline(e->u.var_stmt.init);
    case STIGI_EXPR_ASSIGN:
      return expr_is_multiline(e->u.assign_stmt.value);
    case STIGI_EXPR_INDEX_WRITE:
      return expr_is_multiline(e->u.index_expr.value);
    default:
      return false;
  }
}

static bool expr_needs_atom_parens(const StigiExpr *e) {
  if (e == NULL) return false;
  switch (e->kind) {
    case STIGI_EXPR_BINOP:
    case STIGI_EXPR_UNOP:
    case STIGI_EXPR_IF:
    case STIGI_EXPR_WHILE:
    case STIGI_EXPR_MATCH:
    case STIGI_EXPR_BLOCK:
    case STIGI_EXPR_VAL:
    case STIGI_EXPR_VAR:
    case STIGI_EXPR_ASSIGN:
    case STIGI_EXPR_HANDLE:
    case STIGI_EXPR_INDEX_WRITE:
      return true;
    default:
      return false;
  }
}

static void print_body_after_sep(PrintBuf *b, const StigiExpr *body, uint32_t indent) {
  if (body == NULL) return;
  if (body->kind == STIGI_EXPR_BLOCK) {
    pb_char(b, '\n');
    for (uint32_t i = 0; i < body->u.block.count; i++) {
      if (i > 0) pb_char(b, '\n');
      pb_indent(b, indent + 2);
      print_expr(b, body->u.block.stmts[i], indent + 2);
    }
  } else {
    pb_char(b, ' ');
    print_expr(b, body, indent);
  }
}

static void print_expr(PrintBuf *b, const StigiExpr *e, uint32_t indent) {
  if (e == NULL) return;
  switch (e->kind) {
    case STIGI_EXPR_LOCAL:
    case STIGI_EXPR_GLOBAL:
      pb_str(b, e->u.ident.name);
      break;
    case STIGI_EXPR_LIT_INT:
      pb_str(b, e->u.lit_int.raw);
      break;
    case STIGI_EXPR_LIT_CHAR:
      pb_str(b, e->u.lit_char.raw);
      break;
    case STIGI_EXPR_LIT_STR:
      pb_str(b, e->u.lit_str.raw);
      break;
    case STIGI_EXPR_LIT_BOOL:
      pb_cstr(b, e->u.lit_bool.value ? "true" : "false");
      break;
    case STIGI_EXPR_LIT_UNIT:
      pb_cstr(b, "()");
      break;
    case STIGI_EXPR_CALL: {
      bool cpar = expr_needs_atom_parens(e->u.call.callee);
      if (cpar) pb_char(b, '(');
      print_expr(b, e->u.call.callee, indent);
      if (cpar) pb_char(b, ')');
      if (e->u.call.type_arg_count > 0) {
        pb_char(b, '[');
        for (uint32_t i = 0; i < e->u.call.type_arg_count; i++) {
          if (i > 0) pb_cstr(b, ", ");
          print_type(b, e->u.call.type_args[i]);
        }
        pb_char(b, ']');
      }
      if (e->u.call.cap_arg_count > 0) {
        pb_char(b, '{');
        for (uint32_t i = 0; i < e->u.call.cap_arg_count; i++) {
          if (i > 0) pb_cstr(b, ", ");
          pb_str(b, e->u.call.cap_args[i]);
        }
        pb_char(b, '}');
      }
      if (e->u.call.has_parens || e->u.call.arg_count > 0) {
        pb_char(b, '(');
        for (uint32_t i = 0; i < e->u.call.arg_count; i++) {
          if (i > 0) pb_cstr(b, ", ");
          print_expr(b, e->u.call.args[i], indent);
        }
        pb_char(b, ')');
      }
      break;
    }
    case STIGI_EXPR_BINOP: {
      int p = op_prec(e->u.binop.op);
      const StigiExpr *lh = e->u.binop.lhs;
      const StigiExpr *rh = e->u.binop.rhs;
      bool lpar = (lh->kind == STIGI_EXPR_IF || lh->kind == STIGI_EXPR_WHILE ||
                   lh->kind == STIGI_EXPR_MATCH || lh->kind == STIGI_EXPR_BLOCK ||
                   (lh->kind == STIGI_EXPR_BINOP && op_prec(lh->u.binop.op) < p));
      bool rpar = (rh->kind == STIGI_EXPR_IF || rh->kind == STIGI_EXPR_WHILE ||
                   rh->kind == STIGI_EXPR_MATCH || rh->kind == STIGI_EXPR_BLOCK ||
                   (rh->kind == STIGI_EXPR_BINOP && op_prec(rh->u.binop.op) <= p));
      if (lpar) pb_char(b, '(');
      print_expr(b, lh, indent);
      if (lpar) pb_char(b, ')');
      pb_char(b, ' ');
      pb_cstr(b, op_str(e->u.binop.op));
      pb_char(b, ' ');
      if (rpar) pb_char(b, '(');
      print_expr(b, rh, indent);
      if (rpar) pb_char(b, ')');
      break;
    }
    case STIGI_EXPR_UNOP: {
      pb_cstr(b, op_str(e->u.unop.op));
      const StigiExpr *opnd = e->u.unop.operand;
      bool upar = (opnd->kind == STIGI_EXPR_BINOP || opnd->kind == STIGI_EXPR_IF ||
                   opnd->kind == STIGI_EXPR_WHILE || opnd->kind == STIGI_EXPR_MATCH ||
                   opnd->kind == STIGI_EXPR_BLOCK ||
                   (opnd->kind == STIGI_EXPR_UNOP && opnd->u.unop.op == STIGI_OP_NEG &&
                    e->u.unop.op == STIGI_OP_NEG));
      if (upar) pb_char(b, '(');
      print_expr(b, opnd, indent);
      if (upar) pb_char(b, ')');
      break;
    }
    case STIGI_EXPR_IF:
      pb_cstr(b, "if ");
      print_expr(b, e->u.if_expr.cond, indent);
      pb_cstr(b, " then");
      print_body_after_sep(b, e->u.if_expr.then_br, indent);
      if (expr_is_multiline(e->u.if_expr.then_br)) {
        pb_char(b, '\n');
        pb_indent(b, indent);
        pb_cstr(b, "else");
      } else {
        pb_cstr(b, " else");
      }
      print_body_after_sep(b, e->u.if_expr.else_br, indent);
      break;
    case STIGI_EXPR_WHILE:
      pb_cstr(b, "while ");
      print_expr(b, e->u.while_expr.cond, indent);
      pb_cstr(b, " do");
      print_body_after_sep(b, e->u.while_expr.body, indent);
      break;
    case STIGI_EXPR_MATCH: {
      const StigiExpr *sc = e->u.match_expr.scrutinee;
      bool spar = (sc->kind == STIGI_EXPR_IF || sc->kind == STIGI_EXPR_WHILE ||
                   sc->kind == STIGI_EXPR_MATCH || sc->kind == STIGI_EXPR_BLOCK);
      if (spar) pb_char(b, '(');
      print_expr(b, sc, indent);
      if (spar) pb_char(b, ')');
      pb_cstr(b, " match");
      for (uint32_t i = 0; i < e->u.match_expr.arm_count; i++) {
        pb_char(b, '\n');
        pb_indent(b, indent + 2);
        pb_cstr(b, "case ");
        print_pat(b, &e->u.match_expr.arms[i].pat);
        pb_cstr(b, " =>");
        print_body_after_sep(b, e->u.match_expr.arms[i].body, indent + 2);
      }
      break;
    }
    case STIGI_EXPR_VAL:
      pb_cstr(b, "val ");
      print_pat(b, &e->u.val_stmt.pat);
      if (e->u.val_stmt.type != NULL) {
        pb_cstr(b, ": ");
        print_type(b, e->u.val_stmt.type);
      }
      pb_cstr(b, " =");
      print_body_after_sep(b, e->u.val_stmt.init, indent);
      break;
    case STIGI_EXPR_VAR:
      pb_cstr(b, "var ");
      pb_str(b, e->u.var_stmt.name);
      if (e->u.var_stmt.has_inv) {
        pb_char(b, '@');
        print_fact_group(b, &e->u.var_stmt.inv);
      }
      if (e->u.var_stmt.type != NULL) {
        pb_cstr(b, ": ");
        print_type(b, e->u.var_stmt.type);
      }
      pb_cstr(b, " =");
      print_body_after_sep(b, e->u.var_stmt.init, indent);
      break;
    case STIGI_EXPR_ASSIGN:
      pb_str(b, e->u.assign_stmt.name);
      pb_cstr(b, " =");
      print_body_after_sep(b, e->u.assign_stmt.value, indent);
      break;
    case STIGI_EXPR_INDEX_READ: {
      bool tpar = expr_needs_atom_parens(e->u.index_expr.target);
      if (tpar) pb_char(b, '(');
      print_expr(b, e->u.index_expr.target, indent);
      if (tpar) pb_char(b, ')');
      pb_char(b, '(');
      print_expr(b, e->u.index_expr.index, indent);
      pb_char(b, ')');
      break;
    }
    case STIGI_EXPR_INDEX_WRITE: {
      bool tpar = expr_needs_atom_parens(e->u.index_expr.target);
      if (tpar) pb_char(b, '(');
      print_expr(b, e->u.index_expr.target, indent);
      if (tpar) pb_char(b, ')');
      pb_char(b, '(');
      print_expr(b, e->u.index_expr.index, indent);
      pb_cstr(b, ") :=");
      print_body_after_sep(b, e->u.index_expr.value, indent);
      break;
    }
    case STIGI_EXPR_HANDLE:
      pb_cstr(b, "handle ");
      if (e->u.handle_stmt.binder.len > 0) {
        pb_str(b, e->u.handle_stmt.binder);
        pb_cstr(b, " = ");
      }
      pb_str(b, e->u.handle_stmt.handler_name);
      if (e->u.handle_stmt.type_arg_count > 0) {
        pb_char(b, '[');
        for (uint32_t i = 0; i < e->u.handle_stmt.type_arg_count; i++) {
          if (i > 0) pb_cstr(b, ", ");
          print_type(b, e->u.handle_stmt.type_args[i]);
        }
        pb_char(b, ']');
      }
      if (e->u.handle_stmt.cap_arg_count > 0) {
        pb_char(b, '{');
        for (uint32_t i = 0; i < e->u.handle_stmt.cap_arg_count; i++) {
          if (i > 0) pb_cstr(b, ", ");
          pb_str(b, e->u.handle_stmt.cap_args[i]);
        }
        pb_char(b, '}');
      }
      if (e->u.handle_stmt.has_parens) {
        pb_char(b, '(');
        for (uint32_t i = 0; i < e->u.handle_stmt.arg_count; i++) {
          if (i > 0) pb_cstr(b, ", ");
          print_expr(b, e->u.handle_stmt.args[i], indent);
        }
        pb_char(b, ')');
      }
      if (e->u.handle_stmt.body != NULL) {
        pb_char(b, ':');
        print_body_after_sep(b, e->u.handle_stmt.body, indent);
      }
      break;
    case STIGI_EXPR_ASSERT:
      if (e->u.assert_expr.expr != NULL) {
        bool apar = expr_needs_atom_parens(e->u.assert_expr.expr);
        if (apar) pb_char(b, '(');
        print_expr(b, e->u.assert_expr.expr, indent);
        if (apar) pb_char(b, ')');
      }
      pb_char(b, '@');
      print_fact_group(b, &e->u.assert_expr.facts);
      break;
    case STIGI_EXPR_DENY:
      print_fact_item(b, &e->u.deny_expr.item);
      break;
    case STIGI_EXPR_BLOCK:
      for (uint32_t i = 0; i < e->u.block.count; i++) {
        if (i > 0) {
          pb_char(b, '\n');
          pb_indent(b, indent);
        }
        print_expr(b, e->u.block.stmts[i], indent);
      }
      break;
    case STIGI_EXPR_LOCAL_DECL:
      print_decl(b, e->u.local_decl.decl, indent);
      break;
  }
}

static void print_def_head(PrintBuf *b, const StigiDefHead *h, bool is_lemma) {
  bool had_pre = false;
  if (h->type_param_count > 0) {
    pb_char(b, ' ');
    print_params(b, '[', ']', h->type_params, h->type_param_count);
    had_pre = true;
  }
  for (uint32_t i = 0; i < h->pre_group_count; i++) {
    pb_char(b, ' ');
    if (h->pre_groups[i].is_fact) {
      print_fact_group(b, &h->pre_groups[i].facts);
    } else {
      print_cap_group(b, h->pre_groups[i].caps, h->pre_groups[i].cap_count);
    }
    had_pre = true;
  }
  if (h->has_val_group) {
    if (had_pre) pb_char(b, ' ');
    print_params(b, '(', ')', h->params, h->param_count);
  }
  for (uint32_t i = 0; i < h->post_val_fact_count; i++) {
    pb_char(b, ' ');
    print_fact_group(b, &h->post_val_facts[i]);
  }
  pb_char(b, ':');
  if (!is_lemma && h->ret_type != NULL) {
    pb_char(b, ' ');
    print_type(b, h->ret_type);
  }
  for (uint32_t i = 0; i < h->exit_fact_count; i++) {
    pb_char(b, ' ');
    print_fact_group(b, &h->exit_facts[i]);
  }
}

static void print_decl(PrintBuf *b, const StigiDecl *d, uint32_t indent) {
  if (d == NULL) return;
  if (d->is_pub) pb_cstr(b, "pub ");

  switch (d->kind) {
    case STIGI_DECL_DEF:
      pb_cstr(b, "def ");
      pb_str(b, d->name);
      print_def_head(b, &d->u.def_decl.head, false);
      if (d->u.def_decl.body != NULL) {
        pb_cstr(b, " =");
        print_body_after_sep(b, d->u.def_decl.body, indent);
      }
      break;
    case STIGI_DECL_LEMMA_DEF:
      pb_cstr(b, "lemma def ");
      pb_str(b, d->name);
      print_def_head(b, &d->u.def_decl.head, true);
      if (d->u.def_decl.body != NULL) {
        pb_cstr(b, " =");
        print_body_after_sep(b, d->u.def_decl.body, indent);
      }
      break;
    case STIGI_DECL_VAL:
    case STIGI_DECL_TEST:
    case STIGI_DECL_NEG_TEST:
      pb_cstr(b, "val ");
      pb_str(b, d->name);
      if (d->u.val_decl.cap_count > 0) {
        pb_char(b, ' ');
        print_cap_group(b, d->u.val_decl.caps, d->u.val_decl.cap_count);
      }
      pb_cstr(b, ": ");
      print_type(b, d->u.val_decl.type);
      pb_cstr(b, " =");
      print_body_after_sep(b, d->u.val_decl.body, indent);
      break;
    case STIGI_DECL_ENUM:
      pb_cstr(b, "enum ");
      pb_str(b, d->name);
      if (d->u.enum_decl.type_param_count > 0) {
        pb_char(b, ' ');
        print_params(b, '[', ']', d->u.enum_decl.type_params, d->u.enum_decl.type_param_count);
      }
      pb_char(b, ':');
      for (uint32_t i = 0; i < d->u.enum_decl.case_count; i++) {
        const StigiEnumCase *c = &d->u.enum_decl.cases[i];
        pb_char(b, '\n');
        pb_indent(b, indent + 2);
        pb_cstr(b, "case ");
        pb_str(b, c->name);
        if (c->has_parens) {
          print_params(b, '(', ')', c->fields, c->field_count);
        }
        if (c->has_facts) {
          pb_char(b, ' ');
          print_fact_group(b, &c->facts);
        }
      }
      break;
    case STIGI_DECL_TYPE:
      pb_cstr(b, "type ");
      pb_str(b, d->name);
      if (d->u.type_decl.type_param_count > 0) {
        pb_char(b, ' ');
        print_params(b, '[', ']', d->u.type_decl.type_params, d->u.type_decl.type_param_count);
      }
      if (d->u.type_decl.val_param_count > 0) {
        pb_char(b, ' ');
        print_params(b, '(', ')', d->u.type_decl.val_params, d->u.type_decl.val_param_count);
      }
      if (d->u.type_decl.cap_param_count > 0) {
        pb_char(b, ' ');
        print_cap_group(b, d->u.type_decl.cap_params, d->u.type_decl.cap_param_count);
      }
      pb_cstr(b, " = ");
      print_type(b, d->u.type_decl.rhs);
      if (d->u.type_decl.has_facts) {
        pb_char(b, ' ');
        print_fact_group(b, &d->u.type_decl.facts);
      }
      break;
    case STIGI_DECL_CAPABILITY:
      pb_cstr(b, "capability ");
      pb_str(b, d->name);
      if (d->u.cap_decl.type_param_count > 0) {
        pb_char(b, ' ');
        print_params(b, '[', ']', d->u.cap_decl.type_params, d->u.cap_decl.type_param_count);
      }
      pb_char(b, ':');
      if (d->u.cap_decl.state_count > 0) {
        pb_char(b, '\n');
        pb_indent(b, indent + 2);
        pb_cstr(b, "state ");
        for (uint32_t i = 0; i < d->u.cap_decl.state_count; i++) {
          if (i > 0) pb_cstr(b, ", ");
          pb_str(b, d->u.cap_decl.states[i]);
        }
        pb_char(b, '\n');
        pb_indent(b, indent + 2);
        pb_cstr(b, "start ");
        pb_str(b, d->u.cap_decl.start_state);
        pb_char(b, '\n');
        pb_indent(b, indent + 2);
        pb_cstr(b, "stop ");
        for (uint32_t i = 0; i < d->u.cap_decl.stop_state_count; i++) {
          if (i > 0) pb_cstr(b, ", ");
          pb_str(b, d->u.cap_decl.stop_states[i]);
        }
      }
      for (uint32_t i = 0; i < d->u.cap_decl.member_count; i++) {
        pb_char(b, '\n');
        pb_indent(b, indent + 2);
        print_decl(b, d->u.cap_decl.members[i], indent + 2);
      }
      break;
    case STIGI_DECL_HANDLER:
      pb_cstr(b, "handler ");
      pb_str(b, d->name);
      if (d->u.handler_decl.type_param_count > 0) {
        pb_char(b, ' ');
        print_params(b, '[', ']', d->u.handler_decl.type_params,
                     d->u.handler_decl.type_param_count);
      }
      if (d->u.handler_decl.cap_param_count > 0) {
        pb_char(b, ' ');
        print_cap_group(b, d->u.handler_decl.cap_params, d->u.handler_decl.cap_param_count);
      }
      if (d->u.handler_decl.has_val_parens) {
        if (d->u.handler_decl.type_param_count > 0 || d->u.handler_decl.cap_param_count > 0) {
          pb_char(b, ' ');
        }
        print_params(b, '(', ')', d->u.handler_decl.val_params,
                     d->u.handler_decl.val_param_count);
      }
      pb_cstr(b, " for ");
      if (d->u.handler_decl.for_cap_type != NULL) {
        print_type(b, d->u.handler_decl.for_cap_type);
      } else {
        pb_str(b, d->u.handler_decl.for_cap);
      }
      pb_char(b, ':');
      for (uint32_t i = 0; i < d->u.handler_decl.var_count; i++) {
        pb_char(b, '\n');
        pb_indent(b, indent + 2);
        print_expr(b, d->u.handler_decl.vars[i], indent + 2);
      }
      for (uint32_t i = 0; i < d->u.handler_decl.def_count; i++) {
        pb_char(b, '\n');
        pb_indent(b, indent + 2);
        print_decl(b, d->u.handler_decl.defs[i], indent + 2);
      }
      break;
    case STIGI_DECL_EXTERN:
      pb_cstr(b, "extern ");
      pb_str(b, d->u.extern_decl.abi);
      if (d->u.extern_decl.is_val) {
        pb_cstr(b, " val ");
        pb_str(b, d->name);
        pb_cstr(b, ": ");
        print_type(b, d->u.extern_decl.val_type);
      } else {
        pb_cstr(b, " def ");
        pb_str(b, d->name);
        print_def_head(b, &d->u.extern_decl.head, false);
        if (d->u.extern_decl.body != NULL) {
          pb_cstr(b, " =");
          print_body_after_sep(b, d->u.extern_decl.body, indent);
        }
      }
      break;
    case STIGI_DECL_RELATION:
      pb_cstr(b, "relation ");
      pb_str(b, d->name);
      print_params(b, '(', ')', d->u.relation_decl.columns, d->u.relation_decl.column_count);
      break;
    case STIGI_DECL_RULE:
      pb_cstr(b, "rule def ");
      pb_str(b, d->name);
      pb_cstr(b, ": ");
      print_fact_item(b, &d->u.rule_decl.head);
      if (d->u.rule_decl.body_count > 0) {
        if (d->u.rule_decl.body_multiline) {
          pb_cstr(b, " =\n");
          pb_indent(b, indent + 2);
        } else {
          pb_cstr(b, " = ");
        }
        for (uint32_t i = 0; i < d->u.rule_decl.body_count; i++) {
          if (i > 0) pb_cstr(b, ", ");
          print_fact_item(b, &d->u.rule_decl.body[i]);
        }
      }
      break;
  }
}

Status stigi_print_unit(const StigiUnit *unit, char **out_str, size_t *out_len,
                        Arena *arena, Diag *diag) {
  if (unit == NULL || out_str == NULL || arena == NULL) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_print_unit: invalid arguments");
  }
  PrintBuf b;
  memset(&b, 0, sizeof b);

  if (unit->has_prompt) {
    pb_cstr(&b, "// prompt: ");
    pb_str(&b, unit->prompt);
    pb_char(&b, '\n');
  }
  for (uint32_t i = 0; i < unit->alias_count; i++) {
    pb_cstr(&b, "alias ");
    pb_str(&b, unit->aliases[i].name);
    pb_cstr(&b, " = ");
    pb_str(&b, unit->aliases[i].target);
    pb_char(&b, '\n');
  }
  print_decl(&b, unit->decl, 0);
  pb_char(&b, '\n');

  if (b.oom) {
    free(b.buf);
    return diag_set(diag, STIGI_ERR_OOM, -1, "stigi_print_unit: out of memory");
  }

  char *res = arena_alloc(arena, b.len + 1U);
  if (res == NULL) {
    free(b.buf);
    return diag_set(diag, STIGI_ERR_OOM, -1, "stigi_print_unit: arena out of memory");
  }
  memcpy(res, b.buf, b.len + 1U);
  free(b.buf);
  *out_str = res;
  if (out_len != NULL) *out_len = b.len;
  return STIGI_OK;
}
