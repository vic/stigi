#define _POSIX_C_SOURCE 200809L

#include "door.h"
#include "blake3.h"
#include "eval.h"
#include "lex.h"
#include "parse.h"
#include "print.h"
#include "tenet_gate.h"
#include "types.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void stigi_door_result_init(StigiDoorResult *r) {
  if (r == NULL) return;
  memset(r, 0, sizeof *r);
  r->status = STIGI_OK;
}

void stigi_door_result_free(StigiDoorResult *r) {
  if (r == NULL) return;
  free(r->rejects);
  memset(r, 0, sizeof *r);
}

static Status add_reject(StigiDoorResult *out, const char *subject,
                         const char *code, const char *msg, Diag *diag) {
  if (out == NULL) return STIGI_OK;
  if (out->reject_count == out->reject_cap) {
    uint32_t ncap = out->reject_cap == 0 ? 8U : out->reject_cap * 2U;
    StigiDoorReject *nr = realloc(out->rejects, (size_t)ncap * sizeof *nr);
    if (nr == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "reject list OOM");
    out->rejects = nr;
    out->reject_cap = ncap;
  }
  StigiDoorReject *r = &out->rejects[out->reject_count++];
  (void)snprintf(r->subject, sizeof r->subject, "%s", subject != NULL ? subject : "");
  (void)snprintf(r->code, sizeof r->code, "%s", code != NULL ? code : "");
  (void)snprintf(r->msg, sizeof r->msg, "%s", msg != NULL ? msg : "");
  return STIGI_OK;
}

static const char *tupu_code_to_stigi(int dl_code) {
  switch (dl_code) {
    case TUPU_CODE_DATALOG_ARITH: return "datalog-arith";
    case TUPU_CODE_ENGINE_OUT_OF_MEMORY: return "engine-out-of-memory";
    case TUPU_CODE_DATALOG_BUDGET: return "datalog-budget";
    case TUPU_CODE_RULE_UNSAFE: return "rule-unsafe";
    case TUPU_CODE_NEGATION_CYCLE: return "negation-cycle";
    case TUPU_CODE_RELATION_ARITY: return "relation-arity";
    case TUPU_CODE_UNRESOLVED_NAME: return "unresolved-name";
    default: return "datalog-budget";
  }
}

/* ---- O(1) open-addressed key set cache ---- */

static uint64_t key_hash64(const uint8_t key[32]) {
  uint64_t h = 0;
  for (int i = 0; i < 8; i++) {
    h |= ((uint64_t)key[i]) << (i * 8);
  }
  return h;
}

static void keyset_free(StigiKeySet *ks) {
  if (ks == NULL) return;
  free(ks->slots);
  ks->slots = NULL;
  ks->count = 0;
  ks->cap = 0;
}

static bool keyset_contains(const StigiKeySet *ks, const uint8_t key[32]) {
  if (ks == NULL || ks->cap == 0) return false;
  uint32_t mask = ks->cap - 1U;
  uint32_t idx = (uint32_t)(key_hash64(key) & mask);
  while (ks->slots[idx].used) {
    if (memcmp(ks->slots[idx].key, key, 32) == 0) return true;
    idx = (idx + 1U) & mask;
  }
  return false;
}

static Status keyset_rehash(StigiKeySet *ks, uint32_t new_cap, Diag *diag) {
  StigiCacheSlot *ns = calloc(new_cap, sizeof *ns);
  if (ns == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "keyset OOM");
  uint32_t mask = new_cap - 1U;
  for (uint32_t i = 0; i < ks->cap; i++) {
    if (!ks->slots[i].used) continue;
    uint32_t idx = (uint32_t)(key_hash64(ks->slots[i].key) & mask);
    while (ns[idx].used) idx = (idx + 1U) & mask;
    ns[idx] = ks->slots[i];
  }
  free(ks->slots);
  ks->slots = ns;
  ks->cap = new_cap;
  return STIGI_OK;
}

/* Pre-reserve enough capacity so subsequent atomic inserts cannot fail with OOM */
static Status keyset_reserve(StigiKeySet *ks, uint32_t extra, Diag *diag) {
  uint64_t need = (uint64_t)ks->count + (uint64_t)extra;
  if (need == 0) return STIGI_OK;
  uint32_t cap = ks->cap == 0 ? 64U : ks->cap;
  while ((uint64_t)cap * 3ULL <= need * 4ULL) {
    if (cap > UINT32_MAX / 2U) return diag_set(diag, STIGI_ERR_OVERFLOW, -1, "keyset overflow");
    cap *= 2U;
  }
  if (cap != ks->cap) return keyset_rehash(ks, cap, diag);
  return STIGI_OK;
}

static Status keyset_insert(StigiKeySet *ks, const uint8_t key[32], Diag *diag) {
  if (keyset_contains(ks, key)) return STIGI_OK;
  Status st = keyset_reserve(ks, 1U, diag);
  if (st != STIGI_OK) return st;
  uint32_t mask = ks->cap - 1U;
  uint32_t idx = (uint32_t)(key_hash64(key) & mask);
  while (ks->slots[idx].used) idx = (idx + 1U) & mask;
  memcpy(ks->slots[idx].key, key, 32);
  ks->slots[idx].used = true;
  ks->count++;
  return STIGI_OK;
}

/* Order-independent hash of base rules + sorted in-image Rule & Relation verdict_keys */
static int cmp_key32(const void *a, const void *b) {
  return memcmp(a, b, 32);
}

static Status compute_active_rules_hash(const StigiDoor *door, const StigiImage *img,
                                        uint8_t out[32], Diag *diag) {
  size_t rule_cnt = 0;
  for (uint32_t i = 0; i < img->unit_count; i++) {
    StigiDeclKind k = img->units[i].ast->decl->kind;
    if (k == STIGI_DECL_RULE || k == STIGI_DECL_RELATION) rule_cnt++;
  }
  size_t buf_len = 32U + rule_cnt * 32U;
  uint8_t *buf = malloc(buf_len);
  if (buf == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "rules hash OOM");
  memcpy(buf, door->base_rules_hash, 32);
  size_t off = 32U;
  for (uint32_t i = 0; i < img->unit_count; i++) {
    StigiDeclKind k = img->units[i].ast->decl->kind;
    if (k == STIGI_DECL_RULE || k == STIGI_DECL_RELATION) {
      memcpy(buf + off, img->units[i].verdict_key, 32);
      off += 32U;
    }
  }
  if (rule_cnt > 1U) {
    qsort(buf + 32U, rule_cnt, 32U, cmp_key32);
  }
  blake3_hash(buf, buf_len, out);
  free(buf);
  return STIGI_OK;
}

/* Verdict key includes unit verdict_key, active rules hash, row_budget, AND kernel_mode */
static void effective_verdict_key(const uint8_t base_key[32], const uint8_t rules_hash[32],
                                  uint64_t budget, bool kernel_mode, uint8_t out[32]) {
  uint8_t buf[73];
  memcpy(buf, base_key, 32);
  memcpy(buf + 32, rules_hash, 32);
  for (int i = 0; i < 8; i++) {
    buf[64 + i] = (uint8_t)((budget >> (i * 8)) & 0xFFU);
  }
  buf[72] = kernel_mode ? 1U : 0U;
  blake3_hash(buf, sizeof buf, out);
}

/* ---- Preprocessor for base all.dl ---- */

static Status preprocess_dl(const char *path, char **out_buf, size_t *out_len, Diag *diag) {
  if (path == NULL || path[0] == '\0') {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "preprocess_dl: empty path");
  }
  int pipefd[2];
  if (pipe(pipefd) != 0) {
    return diag_set(diag, STIGI_ERR_IO, errno, "pipe() failed for preprocessor");
  }

  size_t plen = strlen(path);
  char *path_copy = malloc(plen + 1U);
  if (path_copy == NULL) {
    (void)close(pipefd[0]);
    (void)close(pipefd[1]);
    return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
  }
  memcpy(path_copy, path, plen + 1U);

  pid_t pid = fork();
  if (pid < 0) {
    int e = errno;
    free(path_copy);
    (void)close(pipefd[0]);
    (void)close(pipefd[1]);
    return diag_set(diag, STIGI_ERR_IO, e, "fork() failed for preprocessor");
  }
  if (pid == 0) {
    if (dup2(pipefd[1], STDOUT_FILENO) < 0) _exit(127);
    (void)close(pipefd[0]);
    (void)close(pipefd[1]);
    char arg0[] = "gcc";
    char arg1[] = "-E";
    char arg2[] = "-P";
    char arg3[] = "-x";
    char arg4[] = "c";
    char *const argv[] = {arg0, arg1, arg2, arg3, arg4, path_copy, NULL};
    execvp(arg0, argv);
    _exit(127);
  }

  free(path_copy);
  (void)close(pipefd[1]);

  size_t cap = 65536U;
  size_t len = 0;
  char *buf = malloc(cap);
  if (buf == NULL) {
    (void)close(pipefd[0]);
    int wst = 0;
    while (waitpid(pid, &wst, 0) < 0 && errno == EINTR) {}
    return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
  }
  for (;;) {
    if (len + 4096U + 1U > cap) {
      if (cap > SIZE_MAX / 2U) {
        free(buf);
        (void)close(pipefd[0]);
        int wst = 0;
        while (waitpid(pid, &wst, 0) < 0 && errno == EINTR) {}
        return diag_set(diag, STIGI_ERR_OVERFLOW, -1, "preprocessor output overflow");
      }
      cap *= 2U;
      char *nb = realloc(buf, cap);
      if (nb == NULL) {
        free(buf);
        (void)close(pipefd[0]);
        int wst = 0;
        while (waitpid(pid, &wst, 0) < 0 && errno == EINTR) {}
        return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
      }
      buf = nb;
    }
    ssize_t n = read(pipefd[0], buf + len, 4096U);
    if (n < 0) {
      if (errno == EINTR) continue;
      int e = errno;
      free(buf);
      (void)close(pipefd[0]);
      int wst = 0;
      while (waitpid(pid, &wst, 0) < 0 && errno == EINTR) {}
      return diag_set(diag, STIGI_ERR_IO, e, "read from preprocessor failed on %s", path);
    }
    if (n == 0) break;
    len += (size_t)n;
  }
  (void)close(pipefd[0]);

  int wstatus = 0;
  while (waitpid(pid, &wstatus, 0) < 0) {
    if (errno == EINTR) continue;
    int e = errno;
    free(buf);
    return diag_set(diag, STIGI_ERR_IO, e, "waitpid failed for preprocessor on %s", path);
  }
  if (!WIFEXITED(wstatus) || WEXITSTATUS(wstatus) != 0) {
    int code = WIFEXITED(wstatus) ? WEXITSTATUS(wstatus) : -1;
    free(buf);
    return diag_set(diag, STIGI_ERR_IO, code, "preprocessor failed on %s", path);
  }
  buf[len] = '\0';
  *out_buf = buf;
  *out_len = len;
  return STIGI_OK;
}

/* ---- Dynamic string builder for in-image Datalog rule/relation compilation ---- */

typedef struct {
  char *buf;
  size_t len;
  size_t cap;
  bool oom;
} DlBuf;

static void db_mem(DlBuf *b, const char *s, size_t n) {
  if (b->oom || n == 0) return;
  if (n > SIZE_MAX - b->len - 1U) { b->oom = true; return; }
  if (b->len + n + 1U > b->cap) {
    size_t ncap = b->cap == 0 ? 4096U : b->cap;
    while (ncap < b->len + n + 1U) {
      if (ncap > SIZE_MAX / 2U) { b->oom = true; return; }
      ncap *= 2U;
    }
    char *nb = realloc(b->buf, ncap);
    if (nb == NULL) { b->oom = true; return; }
    b->buf = nb;
    b->cap = ncap;
  }
  memcpy(b->buf + b->len, s, n);
  b->len += n;
  b->buf[b->len] = '\0';
}

static void db_cstr(DlBuf *b, const char *s) {
  if (s != NULL) db_mem(b, s, strlen(s));
}

static bool str_eq_c(StigiStr s, const char *lit) {
  size_t n = strlen(lit);
  return s.len == n && memcmp(s.ptr, lit, n) == 0;
}

static bool is_dl_num_type(StigiStr name) {
  return str_eq_c(name, "Stigi.Integer") || str_eq_c(name, "U8") || str_eq_c(name, "I8") ||
         str_eq_c(name, "U32") || str_eq_c(name, "I32") || str_eq_c(name, "U64") ||
         str_eq_c(name, "I64");
}

static bool dl_decl_exists(const char *dl_src, const char *rel_name) {
  if (dl_src == NULL || rel_name == NULL) return false;
  char pat[160];
  int n = snprintf(pat, sizeof pat, ".decl %s(", rel_name);
  if (n <= 0 || (size_t)n >= sizeof pat) return false;
  return strstr(dl_src, pat) != NULL;
}

static void resolve_rel_dl_name(const StigiImage *img, uint32_t from_idx, const char *base_dl,
                                StigiStr raw_name, char out[160]) {
  /* Resolve aliases/in-image relation first */
  int32_t ridx = stigi_image_resolve_name(img, from_idx, raw_name.ptr, raw_name.len);
  const char *src = raw_name.ptr;
  size_t len = raw_name.len;
  if (ridx >= 0) {
    src = img->units[ridx].name;
    len = img->units[ridx].name_len;
  }
  /* Check if plain suffix (after Stigi. / Stigi.Facts.) already exists in schema.dl */
  const char *last_dot = NULL;
  for (size_t i = 0; i < len; i++) {
    if (src[i] == '.') last_dot = src + i;
  }
  if (last_dot != NULL && len >= 6 && memcmp(src, "Stigi.", 6) == 0) {
    const char *suf = last_dot + 1;
    size_t slen = (size_t)((src + len) - suf);
    if (slen < 150) {
      char tmp[160];
      memcpy(tmp, suf, slen);
      tmp[slen] = '\0';
      if (dl_decl_exists(base_dl, tmp)) {
        memcpy(out, tmp, slen + 1U);
        return;
      }
    }
  }
  if (last_dot == NULL && len < 150) {
    char tmp[160];
    memcpy(tmp, src, len);
    tmp[len] = '\0';
    if (dl_decl_exists(base_dl, tmp)) {
      memcpy(out, tmp, len + 1U);
      return;
    }
  }
  /* Mangle dotted Stigi relation name into valid Datalog identifier */
  size_t pos = 0;
  for (size_t i = 0; i < len && pos + 3 < 160; i++) {
    if (src[i] == '.') {
      out[pos++] = '_';
      out[pos++] = '_';
    } else {
      out[pos++] = src[i];
    }
  }
  out[pos] = '\0';
}

static void emit_rule_expr_dl(DlBuf *b, const StigiExpr *e) {
  if (e == NULL) { db_cstr(b, "_"); return; }
  if (e->kind == STIGI_EXPR_LOCAL || e->kind == STIGI_EXPR_GLOBAL) {
    db_mem(b, e->u.ident.name.ptr, e->u.ident.name.len);
  } else if (e->kind == STIGI_EXPR_LIT_INT) {
    char nb[32];
    int nn = snprintf(nb, sizeof nb, "%llu", (unsigned long long)e->u.lit_int.value);
    if (nn > 0) db_mem(b, nb, (size_t)nn);
  } else if (e->kind == STIGI_EXPR_LIT_STR) {
    db_mem(b, e->u.lit_str.raw.ptr, e->u.lit_str.raw.len);
  } else if (e->kind == STIGI_EXPR_UNOP && e->u.unop.op == STIGI_OP_NEG) {
    db_cstr(b, "-");
    emit_rule_expr_dl(b, e->u.unop.operand);
  } else if (e->kind == STIGI_EXPR_BINOP) {
    db_cstr(b, "(");
    emit_rule_expr_dl(b, e->u.binop.lhs);
    switch (e->u.binop.op) {
      case STIGI_OP_ADD: db_cstr(b, " + "); break;
      case STIGI_OP_SUB: db_cstr(b, " - "); break;
      case STIGI_OP_MUL: db_cstr(b, " * "); break;
      case STIGI_OP_DIV: db_cstr(b, " / "); break;
      case STIGI_OP_MOD: db_cstr(b, " % "); break;
      default: db_cstr(b, " + "); break;
    }
    emit_rule_expr_dl(b, e->u.binop.rhs);
    db_cstr(b, ")");
  } else {
    db_cstr(b, "_");
  }
}

static void emit_rule_item_dl(DlBuf *b, const StigiImage *img, uint32_t from_idx,
                              const char *base_dl, const StigiFactItem *it) {
  if (it->kind == STIGI_FACT_ATOM) {
    if (it->is_negated) db_cstr(b, "!");
    char rname[160];
    resolve_rel_dl_name(img, from_idx, base_dl, it->atom_name, rname);
    db_cstr(b, rname);
    db_cstr(b, "(");
    for (uint32_t i = 0; i < it->arg_count; i++) {
      if (i > 0) db_cstr(b, ", ");
      emit_rule_expr_dl(b, it->args[i]);
    }
    db_cstr(b, ")");
  } else if (it->kind == STIGI_FACT_CMP) {
    emit_rule_expr_dl(b, it->lhs);
    switch (it->op) {
      case STIGI_OP_EQ: db_cstr(b, " = "); break;
      case STIGI_OP_NE: db_cstr(b, " != "); break;
      case STIGI_OP_LT: db_cstr(b, " < "); break;
      case STIGI_OP_LE: db_cstr(b, " <= "); break;
      case STIGI_OP_GT: db_cstr(b, " > "); break;
      case STIGI_OP_GE: db_cstr(b, " >= "); break;
      default: db_cstr(b, " = "); break;
    }
    emit_rule_expr_dl(b, it->rhs);
  }
}

static Status compile_image_rules(const StigiDoor *door, const StigiImage *cand,
                                  char **out_dl, size_t *out_len, bool *has_custom, Diag *diag) {
  *out_dl = NULL;
  *out_len = 0;
  *has_custom = false;
  for (uint32_t i = 0; i < cand->unit_count; i++) {
    StigiDeclKind k = cand->units[i].ast->decl->kind;
    if (k == STIGI_DECL_RELATION || k == STIGI_DECL_RULE) {
      *has_custom = true;
      break;
    }
  }
  if (!*has_custom) return STIGI_OK;

  DlBuf db = {NULL, 0, 0, false};
  db_mem(&db, door->base_rules_dl, door->base_rules_dl_len);
  db_cstr(&db, "\n// ---- in-image relations and rules ----\n");

  /* 1. Emit .decl + .input for in-image relations not already in schema.dl */
  for (uint32_t i = 0; i < cand->unit_count && !db.oom; i++) {
    const StigiDecl *d = cand->units[i].ast->decl;
    if (d->kind != STIGI_DECL_RELATION) continue;
    StigiStr un = {cand->units[i].name, cand->units[i].name_len};
    char rname[160];
    resolve_rel_dl_name(cand, i, door->base_rules_dl, un, rname);
    if (dl_decl_exists(door->base_rules_dl, rname)) continue;
    db_cstr(&db, ".decl ");
    db_cstr(&db, rname);
    db_cstr(&db, "(");
    for (uint32_t c = 0; c < d->u.relation_decl.column_count; c++) {
      if (c > 0) db_cstr(&db, ", ");
      const StigiParam *cp = &d->u.relation_decl.columns[c];
      if (cp->name.len > 0) db_mem(&db, cp->name.ptr, cp->name.len);
      else {
        char cn[16];
        int nn = snprintf(cn, sizeof cn, "c%u", c);
        if (nn > 0) db_mem(&db, cn, (size_t)nn);
      }
      db_cstr(&db, ": ");
      if (cp->type != NULL && is_dl_num_type(cp->type->name)) db_cstr(&db, "number");
      else db_cstr(&db, "symbol");
    }
    db_cstr(&db, ")\n.input ");
    db_cstr(&db, rname);
    db_cstr(&db, "\n");
  }

  /* 2. Emit rules */
  for (uint32_t i = 0; i < cand->unit_count && !db.oom; i++) {
    const StigiDecl *d = cand->units[i].ast->decl;
    if (d->kind != STIGI_DECL_RULE) continue;
    emit_rule_item_dl(&db, cand, i, door->base_rules_dl, &d->u.rule_decl.head);
    if (d->u.rule_decl.body_count > 0) {
      db_cstr(&db, " :- ");
      for (uint32_t b = 0; b < d->u.rule_decl.body_count; b++) {
        if (b > 0) db_cstr(&db, ", ");
        emit_rule_item_dl(&db, cand, i, door->base_rules_dl, &d->u.rule_decl.body[b]);
      }
    }
    db_cstr(&db, ".\n");
  }

  if (db.oom) {
    free(db.buf);
    return diag_set(diag, STIGI_ERR_OOM, -1, "compile_image_rules OOM");
  }
  *out_dl = db.buf;
  *out_len = db.len;
  return STIGI_OK;
}

/* ---- Door lifecycle ---- */

Status stigi_door_init(StigiDoor *door, const char *all_dl_path,
                       uint64_t row_budget, bool kernel_mode, Diag *diag) {
  if (door == NULL || all_dl_path == NULL) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_door_init: invalid arguments");
  }
  memset(door, 0, sizeof *door);
  stigi_image_init(&door->image);
  door->row_budget = row_budget > 0 ? row_budget : STIGI_DEFAULT_ROW_BUDGET;
  door->kernel_mode = kernel_mode;

  Status st = preprocess_dl(all_dl_path, &door->base_rules_dl, &door->base_rules_dl_len, diag);
  if (st != STIGI_OK) {
    stigi_door_free(door);
    return st;
  }
  door->rules_dl = malloc(door->base_rules_dl_len + 1U);
  if (door->rules_dl == NULL) {
    stigi_door_free(door);
    return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
  }
  memcpy(door->rules_dl, door->base_rules_dl, door->base_rules_dl_len + 1U);
  door->rules_dl_len = door->base_rules_dl_len;

  blake3_hash((const uint8_t *)door->base_rules_dl, door->base_rules_dl_len,
              door->base_rules_hash);
  st = stigi_dl_open(&door->dl, door->rules_dl, door->rules_dl_len, "all.dl",
                     door->row_budget, 1, diag);
  if (st != STIGI_OK) {
    stigi_door_free(door);
    return st;
  }
  return STIGI_OK;
}

void stigi_door_free(StigiDoor *door) {
  if (door == NULL) return;
  stigi_image_free(&door->image);
  for (uint32_t i = 0; i < door->src_count; i++) {
    free(door->unit_srcs[i]);
  }
  free(door->unit_srcs);
  free(door->unit_lens);
  stigi_dl_close(&door->dl);
  free(door->base_rules_dl);
  free(door->rules_dl);
  keyset_free(&door->verdict_cache);
  keyset_free(&door->test_cache);
  memset(door, 0, sizeof *door);
}

static void code_to_kebab(StigiStr code_ident, char *out, size_t cap) {
  const char *last_dot = NULL;
  for (size_t i = 0; i < code_ident.len; i++) {
    if (code_ident.ptr[i] == '.') last_dot = code_ident.ptr + i;
  }
  const char *s = last_dot != NULL ? last_dot + 1 : code_ident.ptr;
  size_t len = last_dot != NULL ? (size_t)((code_ident.ptr + code_ident.len) - s) : code_ident.len;
  size_t pos = 0;
  for (size_t i = 0; i < len && pos + 2 < cap; i++) {
    unsigned char c = (unsigned char)s[i];
    if (c >= 'A' && c <= 'Z') {
      if (i > 0) out[pos++] = '-';
      out[pos++] = (char)(c - 'A' + 'a');
    } else {
      out[pos++] = (char)c;
    }
  }
  out[pos] = '\0';
}

/* Strict parser checking if a real `cleared(..., "code")` rule head exists in rules_dl */
static bool code_has_clearing_rule_in(const char *rules_dl, const char *code) {
  if (rules_dl == NULL || code == NULL || code[0] == '\0') return false;
  size_t clen = strlen(code);

  /* Spec variable-cleared codes (rules.md 10): cleared(o, c) :- belowOwed/leqOwed / kernelOnly */
  static const char *const k_order_codes[] = {
      "precondition-unproved", "exit-unproved", "assertion-unproved",
      "invariant-unproved", "refinement-unproved"};
  for (size_t i = 0; i < sizeof(k_order_codes) / sizeof(k_order_codes[0]); i++) {
    if (strcmp(code, k_order_codes[i]) == 0) {
      if (strstr(rules_dl, "cleared(o, c) :- leqOwed(") != NULL ||
          strstr(rules_dl, "cleared(o, c) :- belowOwed(") != NULL) {
        return true;
      }
    }
  }
  static const char *const k_kernel_codes[] = {"reserved-name", "extern-denied", "top-level-name"};
  for (size_t i = 0; i < sizeof(k_kernel_codes) / sizeof(k_kernel_codes[0]); i++) {
    if (strcmp(code, k_kernel_codes[i]) == 0) {
      if (strstr(rules_dl, "cleared(u, c) :- kernelOnly(") != NULL) return true;
    }
  }

  /* Scan rule heads: line/statement starts with `cleared(` followed by `arg0, "<code>")` then `:-` or `.` */
  const char *p = rules_dl;
  while (*p != '\0') {
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    if (p[0] == '/' && p[1] == '/') {
      while (*p != '\0' && *p != '\n') p++;
      continue;
    }
    if (strncmp(p, "cleared(", 8) == 0) {
      const char *q = p + 8;
      while (*q != '\0' && *q != ',' && *q != ')' && *q != '\n') q++;
      if (*q == ',') {
        q++;
        while (*q == ' ' || *q == '\t') q++;
        if (*q == '"' && strncmp(q + 1, code, clen) == 0 && q[1 + clen] == '"') {
          const char *r = q + 2 + clen;
          while (*r == ' ' || *r == '\t') r++;
          if (*r == ')') {
            r++;
            while (*r == ' ' || *r == '\t') r++;
            if ((r[0] == ':' && r[1] == '-') || r[0] == '.') {
              return true;
            }
          }
        }
      }
    }
    while (*p != '\0' && *p != '\n') p++;
  }
  return false;
}

/* ---- Boundary checks: kernel/user mode, extern-outside-handler, safety-reads-user ---- */

static bool is_kernel_head_name(const StigiImage *img, uint32_t from_idx, StigiStr raw_name) {
  if (raw_name.len >= 6 && memcmp(raw_name.ptr, "Stigi.", 6) == 0) return true;
  /* Check unit aliases */
  const StigiUnit *u = img->units[from_idx].ast;
  for (uint32_t a = 0; a < u->alias_count; a++) {
    if (u->aliases[a].name.len == raw_name.len &&
        memcmp(u->aliases[a].name.ptr, raw_name.ptr, raw_name.len) == 0) {
      if (u->aliases[a].target.len >= 6 && memcmp(u->aliases[a].target.ptr, "Stigi.", 6) == 0) {
        return true;
      }
    }
  }
  int32_t idx = stigi_image_resolve_name(img, from_idx, raw_name.ptr, raw_name.len);
  if (idx >= 0 && img->units[idx].name_len >= 6 && memcmp(img->units[idx].name, "Stigi.", 6) == 0) {
    return true;
  }
  /* Direct schema.dl safety heads */
  if (str_eq_c(raw_name, "cleared") || str_eq_c(raw_name, "owes") ||
      str_eq_c(raw_name, "reject") || str_eq_c(raw_name, "below") ||
      str_eq_c(raw_name, "leq")) {
    return true;
  }
  return false;
}

static Status check_boundary_rules(const StigiImage *cand, bool kernel_mode,
                                   StigiDoorResult *out, Diag *diag) {
  Status st = STIGI_OK;
  for (uint32_t i = 0; i < cand->unit_count && st == STIGI_OK; i++) {
    const StigiImageUnit *u = &cand->units[i];
    const StigiDecl *d = u->ast->decl;
    bool under_stigi = (u->name_len >= 6 && memcmp(u->name, "Stigi.", 6) == 0);
    bool has_dot = (memchr(u->name, '.', u->name_len) != NULL);

    if (!kernel_mode) {
      if (under_stigi) {
        st = add_reject(out, u->name, "reserved-name",
                        "Stigi.* units are only allowed in kernel mode", diag);
        if (st != STIGI_OK) break;
      } else if (d->kind == STIGI_DECL_EXTERN) {
        st = add_reject(out, u->name, "extern-denied",
                        "extern units are only allowed in kernel mode", diag);
        if (st != STIGI_OK) break;
      } else if (!has_dot && strcmp(u->name, "main") != 0) {
        st = add_reject(out, u->name, "top-level-name",
                        "top-level names other than main require kernel mode", diag);
        if (st != STIGI_OK) break;
      }
    }

    if (d->kind == STIGI_DECL_RULE) {
      if (d->u.rule_decl.head.kind == STIGI_FACT_ATOM &&
          is_kernel_head_name(cand, i, d->u.rule_decl.head.atom_name) && !under_stigi) {
        st = add_reject(out, u->name, "rule-outside-kernel",
                        "rules targeting Stigi.* relations must live under Stigi.*", diag);
        if (st != STIGI_OK) break;
      }
      for (uint32_t b = 0; b < d->u.rule_decl.body_count && st == STIGI_OK; b++) {
        const StigiFactItem *bi = &d->u.rule_decl.body[b];
        if (bi->kind != STIGI_FACT_ATOM) continue;
        if (str_eq_c(bi->atom_name, "owes") || str_eq_c(bi->atom_name, "cleared") ||
            str_eq_c(bi->atom_name, "reject") || str_eq_c(bi->atom_name, "Stigi.Gate.owes") ||
            str_eq_c(bi->atom_name, "Stigi.Gate.cleared") ||
            str_eq_c(bi->atom_name, "Stigi.Gate.reject")) {
          st = add_reject(out, u->name, "negation-cycle",
                          "rule body may not read owes, cleared, or reject", diag);
          break;
        }
        if (under_stigi) {
          int32_t ridx = stigi_image_resolve_name(cand, i, bi->atom_name.ptr, bi->atom_name.len);
          if (ridx >= 0 && cand->units[ridx].ast->decl->kind == STIGI_DECL_RELATION &&
              !(cand->units[ridx].name_len >= 6 &&
                memcmp(cand->units[ridx].name, "Stigi.", 6) == 0)) {
            st = add_reject(out, u->name, "safety-reads-user",
                            "kernel rule cannot read user relation", diag);
            break;
          }
        }
      }
    }
  }

  /* extern-outside-handler (names.dl 46-52): only Handler units or Stigi.* may reference body-less externs */
  for (uint32_t e = 0; e < cand->edge_count && st == STIGI_OK; e++) {
    uint32_t f = cand->edges[e].from;
    uint32_t t = cand->edges[e].to;
    if (f >= cand->unit_count || t >= cand->unit_count) continue;
    const StigiDecl *td = cand->units[t].ast->decl;
    if (td->kind == STIGI_DECL_EXTERN && !td->u.extern_decl.is_val &&
        td->u.extern_decl.body == NULL) {
      const StigiImageUnit *fu = &cand->units[f];
      bool from_stigi = (fu->name_len >= 6 && memcmp(fu->name, "Stigi.", 6) == 0);
      bool from_handler = (fu->ast->decl->kind == STIGI_DECL_HANDLER);
      if (!from_stigi && !from_handler) {
        st = add_reject(out, fu->name, "extern-outside-handler",
                        "body-less extern may only be called from a handler or Stigi.*", diag);
      }
    }
  }
  return st;
}

/* ---- Dependency-closed Datalog evaluation for one unit ---- */

static Status eval_unit_datalog(StigiDl *dl, bool kernel_mode, const StigiImage *img,
                                uint32_t uidx, StigiDoorResult *out, Diag *diag) {
  Status st = stigi_dl_reset(dl, diag);
  if (st != STIGI_OK) return st;

  StigiFactSet fs;
  stigi_facts_init(&fs);
  st = stigi_extract_unit_closed(img, uidx, kernel_mode, &fs, diag);
  if (st != STIGI_OK) {
    stigi_facts_free(&fs);
    return st;
  }
  /* For Test units, seed testGot/testWant so tests.dl checks test-leaks-capability cleanly */
  if (img->units[uidx].ast->decl->kind == STIGI_DECL_TEST) {
    const char *got[2] = {img->units[uidx].name, "ok"};
    st = stigi_facts_add_sym(&fs, "testGot", 2, got, diag);
    if (st == STIGI_OK) st = stigi_facts_add_sym(&fs, "testWant", 2, got, diag);
    if (st != STIGI_OK) {
      stigi_facts_free(&fs);
      return st;
    }
  }

  Diag ld;
  diag_clear(&ld);
  st = stigi_facts_load_into_dl(&fs, dl, &ld);
  stigi_facts_free(&fs);
  if (st != STIGI_OK) {
    if (st == STIGI_ERR_DATALOG) {
      return add_reject(out, img->units[uidx].name, tupu_code_to_stigi(ld.dl_code), ld.msg, diag);
    }
    if (diag != NULL) *diag = ld;
    return st;
  }

  Diag ed;
  diag_clear(&ed);
  st = stigi_dl_eval(dl, &ed);
  if (st != STIGI_OK) {
    return add_reject(out, img->units[uidx].name, tupu_code_to_stigi(ed.dl_code), ed.msg, diag);
  }

  int rel_reject = stigi_dl_rel_id(dl, "reject");
  if (rel_reject >= 0) {
    uint32_t nrej = stigi_dl_count(dl, rel_reject);
    for (uint32_t r = 0; r < nrej; r++) {
      int64_t row[2] = {0, 0};
      st = stigi_dl_read(dl, rel_reject, r, row, 2, diag);
      if (st != STIGI_OK) return st;
      size_t slen = 0, clen = 0;
      const char *subj = stigi_dl_sym_str(dl, (uint32_t)row[0], &slen);
      const char *code = stigi_dl_sym_str(dl, (uint32_t)row[1], &clen);
      char sbuf[128], cbuf[64];
      (void)snprintf(sbuf, sizeof sbuf, "%.*s", (int)slen, subj != NULL ? subj : "");
      (void)snprintf(cbuf, sizeof cbuf, "%.*s", (int)clen, code != NULL ? code : "");
      st = add_reject(out, sbuf, cbuf, img->units[uidx].name, diag);
      if (st != STIGI_OK) return st;
    }
  }
  return STIGI_OK;
}

/* ---- NegTest verifier (dynamic expect codes, known Stigi.Code.*, full pipeline) ---- */

static Status check_negtest_unit(StigiDoor *door, StigiDl *dl, const char *active_rules_dl,
                                 const StigiImage *cand, uint32_t nt_idx,
                                 StigiDoorResult *out, Diag *diag) {
  const StigiImageUnit *nt_u = &cand->units[nt_idx];
  const StigiDecl *nt_d = nt_u->ast->decl;
  const StigiExpr *body = nt_d->u.val_decl.body;
  if (body == NULL || body->kind != STIGI_EXPR_BLOCK) {
    return add_reject(out, nt_u->name, "negtest-form", "NegTest body must be a block", diag);
  }

  char (*expected_codes)[64] = NULL;
  uint32_t exp_count = 0, exp_cap = 0;

  StigiImage priv;
  stigi_image_init(&priv);
  StigiNameDiags pnd;
  stigi_name_diags_init(&pnd);

  Status st = STIGI_OK;
  for (uint32_t i = 0; i < cand->unit_count && st == STIGI_OK; i++) {
    if (cand->units[i].ast->decl->kind == STIGI_DECL_NEG_TEST) continue;
    Arena ta;
    arena_init(&ta);
    char *pr = NULL;
    size_t plen = 0;
    st = stigi_print_unit(cand->units[i].ast, &pr, &plen, &ta, diag);
    if (st == STIGI_OK) {
      st = stigi_image_add_source(&priv, pr, plen, &pnd, diag);
    }
    arena_free(&ta);
  }

  uint32_t local_start = priv.unit_count;
  for (uint32_t i = 0; i < body->u.block.count && st == STIGI_OK; i++) {
    const StigiExpr *s = body->u.block.stmts[i];
    if (s->kind == STIGI_EXPR_LOCAL_DECL) {
      StigiUnit tmp_u;
      memset(&tmp_u, 0, sizeof tmp_u);
      tmp_u.has_prompt = true;
      tmp_u.prompt.ptr = "negtest local.";
      tmp_u.prompt.len = 14;
      tmp_u.decl = s->u.local_decl.decl;
      Arena ta;
      arena_init(&ta);
      char *pr = NULL;
      size_t plen = 0;
      st = stigi_print_unit(&tmp_u, &pr, &plen, &ta, diag);
      if (st == STIGI_OK) {
        st = stigi_image_add_source(&priv, pr, plen, &pnd, diag);
      }
      arena_free(&ta);
    } else if (s->kind == STIGI_EXPR_CALL &&
               s->u.call.callee->kind == STIGI_EXPR_GLOBAL &&
               s->u.call.callee->u.ident.name.len == 20 &&
               memcmp(s->u.call.callee->u.ident.name.ptr, "Stigi.NegTest.expect", 20) == 0) {
      if (s->u.call.arg_count != 2 ||
          (s->u.call.args[0]->kind != STIGI_EXPR_GLOBAL &&
           s->u.call.args[0]->kind != STIGI_EXPR_LOCAL)) {
        free(expected_codes);
        stigi_name_diags_free(&pnd);
        stigi_image_free(&priv);
        return add_reject(out, nt_u->name, "negtest-form", "invalid Stigi.NegTest.expect form", diag);
      }
      StigiStr code_sym = s->u.call.args[0]->u.ident.name;
      if (code_sym.len < 11 || memcmp(code_sym.ptr, "Stigi.Code.", 11) != 0 ||
          !stigi_is_known_code_case(code_sym.ptr + 11, code_sym.len - 11)) {
        free(expected_codes);
        stigi_name_diags_free(&pnd);
        stigi_image_free(&priv);
        return add_reject(out, nt_u->name, "negtest-form", "unknown Stigi.Code case", diag);
      }
      char kebab[64];
      code_to_kebab(code_sym, kebab, sizeof kebab);
      for (uint32_t e = 0; e < exp_count; e++) {
        if (strcmp(expected_codes[e], kebab) == 0) {
          free(expected_codes);
          stigi_name_diags_free(&pnd);
          stigi_image_free(&priv);
          return add_reject(out, nt_u->name, "negtest-form", "duplicate expected code in NegTest", diag);
        }
      }
      if (exp_count == exp_cap) {
        uint32_t ncap = exp_cap == 0 ? 4U : exp_cap * 2U;
        char (*nc)[64] = realloc(expected_codes, (size_t)ncap * sizeof *nc);
        if (nc == NULL) {
          free(expected_codes);
          stigi_name_diags_free(&pnd);
          stigi_image_free(&priv);
          return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
        }
        expected_codes = nc;
        exp_cap = ncap;
      }
      memcpy(expected_codes[exp_count++], kebab, sizeof kebab);
    }
  }

  if (st != STIGI_OK) {
    free(expected_codes);
    stigi_name_diags_free(&pnd);
    stigi_image_free(&priv);
    return st;
  }

  if (exp_count == 0) {
    free(expected_codes);
    stigi_name_diags_free(&pnd);
    stigi_image_free(&priv);
    return add_reject(out, nt_u->name, "negtest-form", "NegTest missing expect()", diag);
  }

  /* Verify every expected code has a real clearing rule (T2: negtest-without-rule) */
  for (uint32_t e = 0; e < exp_count; e++) {
    if (!code_has_clearing_rule_in(active_rules_dl, expected_codes[e])) {
      char bad_code[64];
      memcpy(bad_code, expected_codes[e], sizeof bad_code);
      free(expected_codes);
      stigi_name_diags_free(&pnd);
      stigi_image_free(&priv);
      return add_reject(out, nt_u->name, "negtest-without-rule", bad_code, diag);
    }
  }

  for (uint32_t i = local_start; i < priv.unit_count; i++) {
    priv.units[i].is_root = true;
  }

  StigiDoorResult priv_res;
  stigi_door_result_init(&priv_res);

  st = stigi_image_resolve(&priv, &pnd, diag);
  if (st != STIGI_OK) {
    stigi_door_result_free(&priv_res);
    free(expected_codes);
    stigi_name_diags_free(&pnd);
    stigi_image_free(&priv);
    return st;
  }
  for (uint32_t i = 0; i < pnd.count && st == STIGI_OK; i++) {
    if (pnd.items[i].code == STIGI_NAME_NO_READER || pnd.items[i].code == STIGI_NAME_PUB_UNUSED) {
      continue;
    }
    st = add_reject(&priv_res, nt_u->name, stigi_name_code_name(pnd.items[i].code),
                    pnd.items[i].msg, diag);
  }

  if (st == STIGI_OK) {
    st = check_boundary_rules(&priv, door->kernel_mode, &priv_res, diag);
  }

  if (st == STIGI_OK) {
    StigiTypeDiags ptd;
    stigi_type_diags_init(&ptd);
    st = stigi_typecheck_image(&priv, &ptd, diag);
    for (uint32_t i = 0; i < ptd.count && st == STIGI_OK; i++) {
      st = add_reject(&priv_res, nt_u->name, stigi_type_code_name(ptd.items[i].code),
                      ptd.items[i].msg, diag);
    }
    stigi_type_diags_free(&ptd);
  }

  /* Compile any custom rules declared inside the NegTest body */
  StigiDl *priv_dl = dl;
  StigiDl local_dl;
  memset(&local_dl, 0, sizeof local_dl);
  char *priv_dl_src = NULL;
  size_t priv_dl_len = 0;
  bool priv_has_custom = false;
  if (st == STIGI_OK && priv_res.reject_count == 0) {
    st = compile_image_rules(door, &priv, &priv_dl_src, &priv_dl_len, &priv_has_custom, diag);
    if (st == STIGI_OK && priv_has_custom) {
      Diag od;
      diag_clear(&od);
      if (stigi_dl_open(&local_dl, priv_dl_src, priv_dl_len, "negtest.dl",
                        door->row_budget, 1, &od) != STIGI_OK) {
        st = add_reject(&priv_res, nt_u->name, tupu_code_to_stigi(od.dl_code), od.msg, diag);
      } else {
        priv_dl = &local_dl;
      }
    }
  }

  if (st == STIGI_OK && priv_res.reject_count == 0) {
    for (uint32_t i = local_start; i < priv.unit_count && st == STIGI_OK; i++) {
      st = eval_unit_datalog(priv_dl, door->kernel_mode, &priv, i, &priv_res, diag);
    }
  }

  stigi_dl_close(&local_dl);
  free(priv_dl_src);
  stigi_name_diags_free(&pnd);
  stigi_image_free(&priv);

  if (st != STIGI_OK) {
    stigi_door_result_free(&priv_res);
    free(expected_codes);
    return st;
  }

  if (priv_res.reject_count == 0) {
    stigi_door_result_free(&priv_res);
    free(expected_codes);
    return add_reject(out, nt_u->name, "negtest-not-rejected",
                      "NegTest body was accepted instead of rejected", diag);
  }

  /* Exact set equality expected == actual AND exactly 1 subject per expected code */
  for (uint32_t e = 0; e < exp_count; e++) {
    uint32_t hits = 0;
    for (uint32_t r = 0; r < priv_res.reject_count; r++) {
      if (strcmp(priv_res.rejects[r].code, expected_codes[e]) == 0) hits++;
    }
    if (hits == 0) {
      char msg[256];
      (void)snprintf(msg, sizeof msg, "NegTest expected '%.60s', got '%.60s'",
                     expected_codes[e], priv_res.rejects[0].code);
      stigi_door_result_free(&priv_res);
      free(expected_codes);
      return add_reject(out, nt_u->name, "negtest-wrong-code", msg, diag);
    }
    if (hits > 1) {
      stigi_door_result_free(&priv_res);
      free(expected_codes);
      return add_reject(out, nt_u->name, "negtest-form",
                        "NegTest expected code matched multiple subjects", diag);
    }
  }
  for (uint32_t r = 0; r < priv_res.reject_count; r++) {
    bool in_exp = false;
    for (uint32_t e = 0; e < exp_count; e++) {
      if (strcmp(priv_res.rejects[r].code, expected_codes[e]) == 0) {
        in_exp = true;
        break;
      }
    }
    if (!in_exp) {
      char msg[256];
      (void)snprintf(msg, sizeof msg, "NegTest produced unexpected code '%.60s'",
                     priv_res.rejects[r].code);
      stigi_door_result_free(&priv_res);
      free(expected_codes);
      return add_reject(out, nt_u->name, "negtest-wrong-code", msg, diag);
    }
  }

  stigi_door_result_free(&priv_res);
  free(expected_codes);
  return STIGI_OK;
}

/* ---- Atomic transaction pipeline ---- */

static Status run_transaction(StigiDoor *door, const char *const *add_srcs,
                              const size_t *add_lens, uint32_t add_count,
                              const char *const *drop_names, uint32_t drop_count,
                              bool replace_all, bool commit, StigiDoorResult *out, Diag *diag) {
  if (door == NULL || out == NULL) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "run_transaction: invalid arguments");
  }
  out->accepted = false;

  /* Step 1: Single-pass Lex -> Tenet Gate + Parse on all incoming units */
  char **new_names = NULL;
  if (add_count > 0) {
    new_names = calloc(add_count, sizeof *new_names);
    if (new_names == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
  }

  Status tx_st = STIGI_OK;
  for (uint32_t i = 0; i < add_count && tx_st == STIGI_OK; i++) {
    Arena pa;
    arena_init(&pa);
    StigiTok *toks = NULL;
    uint32_t tok_count = 0;
    tx_st = stigi_lex_all(add_srcs[i], add_lens[i], &toks, &tok_count, &pa, diag);
    if (tx_st != STIGI_OK) {
      arena_free(&pa);
      break;
    }

    StigiGateFindings gf;
    stigi_gate_findings_init(&gf);
    tx_st = stigi_gate_check_tokens(toks, tok_count, &gf, diag);
    if (tx_st == STIGI_OK) {
      for (uint32_t k = 0; k < gf.count && tx_st == STIGI_OK; k++) {
        tx_st = add_reject(out, "<input>", stigi_gate_code_name(gf.items[k].code),
                           gf.items[k].msg, diag);
      }
    }
    stigi_gate_findings_free(&gf);
    if (tx_st != STIGI_OK) {
      arena_free(&pa);
      break;
    }

    StigiParseResult pr = stigi_parse_tokens(toks, tok_count, &pa);
    if (pr.status != STIGI_OK || pr.unit == NULL || pr.unit->decl == NULL) {
      tx_st = add_reject(out, "<input>", pr.diag_code != NULL ? pr.diag_code : "parse",
                         pr.msg, diag);
    } else {
      StigiStr nm = pr.unit->decl->name;
      new_names[i] = malloc(nm.len + 1U);
      if (new_names[i] == NULL) {
        tx_st = diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
      } else {
        memcpy(new_names[i], nm.ptr, nm.len);
        new_names[i][nm.len] = '\0';
      }
    }
    arena_free(&pa);
  }

  /* Verify all dropped names exist in current image */
  for (uint32_t d = 0; !replace_all && d < drop_count && tx_st == STIGI_OK; d++) {
    if (drop_names[d] == NULL ||
        stigi_image_find(&door->image, drop_names[d], strlen(drop_names[d])) < 0) {
      tx_st = add_reject(out, drop_names[d] != NULL ? drop_names[d] : "<drop>",
                         "unknown-unit", "cannot drop unit not in image", diag);
    }
  }

  if (tx_st != STIGI_OK || out->reject_count > 0) {
    for (uint32_t i = 0; i < add_count; i++) free(new_names[i]);
    free(new_names);
    return tx_st;
  }

  /* Build candidate source list */
  uint32_t max_srcs = door->src_count + add_count;
  char **cand_srcs = calloc(max_srcs > 0 ? max_srcs : 1U, sizeof *cand_srcs);
  size_t *cand_lens = calloc(max_srcs > 0 ? max_srcs : 1U, sizeof *cand_lens);
  if (cand_srcs == NULL || cand_lens == NULL) {
    free(cand_srcs);
    free(cand_lens);
    for (uint32_t i = 0; i < add_count; i++) free(new_names[i]);
    free(new_names);
    return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
  }
  uint32_t cand_count = 0;

  for (uint32_t i = 0; !replace_all && i < door->src_count && tx_st == STIGI_OK; i++) {
    const char *uname = i < door->image.unit_count ? door->image.units[i].name : "";
    bool skip = false;
    for (uint32_t d = 0; d < drop_count; d++) {
      if (strcmp(uname, drop_names[d]) == 0) { skip = true; break; }
    }
    for (uint32_t a = 0; !skip && a < add_count; a++) {
      if (new_names[a] != NULL && strcmp(uname, new_names[a]) == 0) { skip = true; break; }
    }
    if (!skip) {
      cand_srcs[cand_count] = malloc(door->unit_lens[i] + 1U);
      if (cand_srcs[cand_count] == NULL) {
        tx_st = diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
        break;
      }
      memcpy(cand_srcs[cand_count], door->unit_srcs[i], door->unit_lens[i] + 1U);
      cand_lens[cand_count] = door->unit_lens[i];
      cand_count++;
    }
  }
  for (uint32_t i = 0; i < add_count; i++) {
    if (tx_st == STIGI_OK) {
      cand_srcs[cand_count] = malloc(add_lens[i] + 1U);
      if (cand_srcs[cand_count] == NULL) {
        tx_st = diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
      } else {
        memcpy(cand_srcs[cand_count], add_srcs[i], add_lens[i]);
        cand_srcs[cand_count][add_lens[i]] = '\0';
        cand_lens[cand_count] = add_lens[i];
        cand_count++;
      }
    }
    free(new_names[i]);
  }
  free(new_names);

  /* Door.md 106: on pure drop, tree-shake units no remaining root reaches */
  if (tx_st == STIGI_OK && drop_count > 0 && add_count == 0 && !replace_all && cand_count > 0) {
    StigiImage tmp_img;
    stigi_image_init(&tmp_img);
    StigiNameDiags tmp_nd;
    stigi_name_diags_init(&tmp_nd);
    bool ok = true;
    for (uint32_t i = 0; i < cand_count && ok; i++) {
      if (stigi_image_add_source(&tmp_img, cand_srcs[i], cand_lens[i], &tmp_nd, diag) != STIGI_OK) {
        ok = false;
      }
    }
    if (ok && stigi_image_resolve(&tmp_img, &tmp_nd, diag) == STIGI_OK) {
      /* Only prune if there were no unresolved references from remaining roots */
      bool has_unres = false;
      for (uint32_t i = 0; i < tmp_nd.count; i++) {
        if (tmp_nd.items[i].code == STIGI_NAME_UNRESOLVED) { has_unres = true; break; }
      }
      if (!has_unres && tmp_img.unit_count == cand_count) {
        uint32_t w = 0;
        for (uint32_t i = 0; i < cand_count; i++) {
          if (tmp_img.units[i].reachable) {
            cand_srcs[w] = cand_srcs[i];
            cand_lens[w] = cand_lens[i];
            w++;
          } else {
            free(cand_srcs[i]);
          }
        }
        cand_count = w;
      }
    }
    stigi_name_diags_free(&tmp_nd);
    stigi_image_free(&tmp_img);
  }

  /* Stage cache updates locally; merge ONLY on commit */
  StigiKeySet staged_vkeys = {NULL, 0, 0};
  StigiKeySet staged_tkeys = {NULL, 0, 0};

  /* Step 2: Names + unit_id / verdict_key (C3, S119) + boundary checks */
  StigiImage cand;
  stigi_image_init(&cand);
  StigiNameDiags nd;
  stigi_name_diags_init(&nd);
  for (uint32_t i = 0; i < cand_count && tx_st == STIGI_OK; i++) {
    Status st = stigi_image_add_source(&cand, cand_srcs[i], cand_lens[i], &nd, diag);
    if (st != STIGI_OK) {
      if (st == STIGI_ERR_OOM) tx_st = st;
      else tx_st = add_reject(out, "<input>", "parse", diag->msg, diag);
    }
  }
  if (tx_st == STIGI_OK && out->reject_count == 0) {
    tx_st = stigi_image_resolve(&cand, &nd, diag);
  }
  for (uint32_t i = 0; i < nd.count && tx_st == STIGI_OK; i++) {
    tx_st = add_reject(out, "<names>", stigi_name_code_name(nd.items[i].code),
                       nd.items[i].msg, diag);
  }
  stigi_name_diags_free(&nd);

  if (tx_st == STIGI_OK) {
    tx_st = check_boundary_rules(&cand, door->kernel_mode, out, diag);
  }

  /* Step 3: Types (C4) */
  if (tx_st == STIGI_OK && out->reject_count == 0) {
    StigiTypeDiags td;
    stigi_type_diags_init(&td);
    tx_st = stigi_typecheck_image(&cand, &td, diag);
    for (uint32_t i = 0; i < td.count && tx_st == STIGI_OK; i++) {
      tx_st = add_reject(out, "<types>", stigi_type_code_name(td.items[i].code),
                         td.items[i].msg, diag);
    }
    stigi_type_diags_free(&td);
  }

  /* Step 3b: Compile in-image rules & relations into active candidate StigiDl */
  StigiDl *eval_dl = &door->dl;
  StigiDl cand_dl;
  memset(&cand_dl, 0, sizeof cand_dl);
  char *cand_rules_dl = NULL;
  size_t cand_rules_len = 0;
  bool has_custom_rules = false;
  bool need_dl_swap = false;

  if (tx_st == STIGI_OK && out->reject_count == 0) {
    tx_st = compile_image_rules(door, &cand, &cand_rules_dl, &cand_rules_len,
                                &has_custom_rules, diag);
    if (tx_st == STIGI_OK && has_custom_rules) {
      Diag od;
      diag_clear(&od);
      if (stigi_dl_open(&cand_dl, cand_rules_dl, cand_rules_len, "image.dl",
                        door->row_budget, 1, &od) != STIGI_OK) {
        tx_st = add_reject(out, "<rules>", tupu_code_to_stigi(od.dl_code), od.msg, diag);
      } else {
        eval_dl = &cand_dl;
        need_dl_swap = true;
      }
    } else if (tx_st == STIGI_OK && !has_custom_rules &&
               door->rules_dl_len != door->base_rules_dl_len) {
      /* All custom rules were dropped; rebuild clean base_rules_dl session */
      cand_rules_dl = malloc(door->base_rules_dl_len + 1U);
      if (cand_rules_dl == NULL) {
        tx_st = diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
      } else {
        memcpy(cand_rules_dl, door->base_rules_dl, door->base_rules_dl_len + 1U);
        cand_rules_len = door->base_rules_dl_len;
        tx_st = stigi_dl_open(&cand_dl, cand_rules_dl, cand_rules_len, "all.dl",
                              door->row_budget, 1, diag);
        if (tx_st == STIGI_OK) {
          eval_dl = &cand_dl;
          need_dl_swap = true;
        }
      }
    }
  }

  const char *active_rules_src = cand_rules_dl != NULL ? cand_rules_dl : door->rules_dl;
  uint8_t rules_hash[32];
  memset(rules_hash, 0, sizeof rules_hash);
  if (tx_st == STIGI_OK && out->reject_count == 0) {
    tx_st = compute_active_rules_hash(door, &cand, rules_hash, diag);
  }

  /* Steps 4 & 5: Extract dependency-closed facts + run Datalog per unit (including Test rows) */
  if (tx_st == STIGI_OK && out->reject_count == 0) {
    for (uint32_t i = 0; i < cand.unit_count && tx_st == STIGI_OK; i++) {
      StigiDeclKind k = cand.units[i].ast->decl->kind;
      if (k == STIGI_DECL_NEG_TEST) continue;
      uint8_t vkey[32];
      effective_verdict_key(cand.units[i].verdict_key, rules_hash, door->row_budget,
                            door->kernel_mode, vkey);
      if (keyset_contains(&door->verdict_cache, vkey)) {
        out->verdicts_cached++;
        continue;
      }
      out->verdicts_computed++;
      uint32_t before = out->reject_count;
      tx_st = eval_unit_datalog(eval_dl, door->kernel_mode, &cand, i, out, diag);
      if (tx_st == STIGI_OK && out->reject_count == before) {
        tx_st = keyset_insert(&staged_vkeys, vkey, diag);
      }
    }
  }

  /* Step 6: NegTests */
  if (tx_st == STIGI_OK && out->reject_count == 0) {
    for (uint32_t i = 0; i < cand.unit_count && tx_st == STIGI_OK; i++) {
      if (cand.units[i].ast->decl->kind != STIGI_DECL_NEG_TEST) continue;
      uint8_t ntkey[32];
      effective_verdict_key(cand.units[i].verdict_key, rules_hash, door->row_budget,
                            door->kernel_mode, ntkey);
      if (keyset_contains(&door->test_cache, ntkey)) {
        out->tests_cached++;
        continue;
      }
      out->tests_computed++;
      uint32_t before = out->reject_count;
      tx_st = check_negtest_unit(door, eval_dl, active_rules_src, &cand, i, out, diag);
      if (tx_st == STIGI_OK && out->reject_count == before) {
        tx_st = keyset_insert(&staged_tkeys, ntkey, diag);
      }
    }
  }

  /* Step 7: Tests & compile-time vals */
  if (tx_st == STIGI_OK && out->reject_count == 0) {
    Arena eval_arena;
    arena_init(&eval_arena);
    for (uint32_t i = 0; i < cand.unit_count && tx_st == STIGI_OK; i++) {
      StigiDeclKind k = cand.units[i].ast->decl->kind;
      if (k == STIGI_DECL_TEST) {
        uint8_t tkey[32];
        effective_verdict_key(cand.units[i].verdict_key, rules_hash, door->row_budget,
                              door->kernel_mode, tkey);
        if (keyset_contains(&door->test_cache, tkey)) {
          out->tests_cached++;
          continue;
        }
        out->tests_computed++;
        StigiEvalResult er = stigi_eval_test(&cand, i, STIGI_DEFAULT_FUEL, &eval_arena);
        if (er.status != STIGI_OK || er.value.kind != STIGI_VAL_TEST_OK) {
          tx_st = add_reject(out, cand.units[i].name,
                             er.diag_code != NULL ? er.diag_code : "test-failed",
                             er.msg[0] != '\0' ? er.msg : "Test did not return Stigi.Test.Result",
                             diag);
        } else {
          tx_st = keyset_insert(&staged_tkeys, tkey, diag);
        }
      } else if (k == STIGI_DECL_VAL) {
        StigiEvalResult er = stigi_eval_unit(&cand, i, STIGI_DEFAULT_FUEL, &eval_arena);
        if (er.status != STIGI_OK) {
          tx_st = add_reject(out, cand.units[i].name,
                             er.diag_code != NULL ? er.diag_code : "eval-error", er.msg, diag);
        }
      }
    }
    arena_free(&eval_arena);
  }

  /* Step 8: Atomic commit or rollback (pre-reserve cache capacity so commit cannot half-fail) */
  if (tx_st == STIGI_OK && out->reject_count == 0) {
    if (commit) {
      tx_st = keyset_reserve(&door->verdict_cache, staged_vkeys.count, diag);
      if (tx_st == STIGI_OK) {
        tx_st = keyset_reserve(&door->test_cache, staged_tkeys.count, diag);
      }
    }
    if (tx_st == STIGI_OK) {
      if (commit) {
        for (uint32_t i = 0; i < staged_vkeys.cap; i++) {
          if (staged_vkeys.slots[i].used) {
            (void)keyset_insert(&door->verdict_cache, staged_vkeys.slots[i].key, diag);
          }
        }
        for (uint32_t i = 0; i < staged_tkeys.cap; i++) {
          if (staged_tkeys.slots[i].used) {
            (void)keyset_insert(&door->test_cache, staged_tkeys.slots[i].key, diag);
          }
        }
        if (need_dl_swap) {
          stigi_dl_close(&door->dl);
          door->dl = cand_dl;
          free(door->rules_dl);
          door->rules_dl = cand_rules_dl;
          door->rules_dl_len = cand_rules_len;
          cand_dl.sess = NULL;
          cand_rules_dl = NULL;
        }
        stigi_image_free(&door->image);
        door->image = cand;
        for (uint32_t i = 0; i < door->src_count; i++) free(door->unit_srcs[i]);
        free(door->unit_srcs);
        free(door->unit_lens);
        door->unit_srcs = cand_srcs;
        door->unit_lens = cand_lens;
        door->src_count = cand_count;
        door->src_cap = max_srcs;
        out->accepted = true;
        keyset_free(&staged_vkeys);
        keyset_free(&staged_tkeys);
        return STIGI_OK;
      }
      out->accepted = true;
      keyset_free(&staged_vkeys);
      keyset_free(&staged_tkeys);
      stigi_dl_close(&cand_dl);
      free(cand_rules_dl);
      stigi_image_free(&cand);
      for (uint32_t i = 0; i < cand_count; i++) free(cand_srcs[i]);
      free(cand_srcs);
      free(cand_lens);
      return STIGI_OK;
    }
  }

  /* Rollback: live door->image, door->dl, and door caches are 100% untouched */
  keyset_free(&staged_vkeys);
  keyset_free(&staged_tkeys);
  stigi_dl_close(&cand_dl);
  free(cand_rules_dl);
  stigi_image_free(&cand);
  for (uint32_t i = 0; i < cand_count; i++) free(cand_srcs[i]);
  free(cand_srcs);
  free(cand_lens);
  return tx_st;
}

Status stigi_door_submit(StigiDoor *door, const char *const *srcs, const size_t *lens,
                         uint32_t count, StigiDoorResult *out, Diag *diag) {
  return run_transaction(door, srcs, lens, count, NULL, 0, false, true, out, diag);
}

Status stigi_door_check(StigiDoor *door, const char *const *srcs, const size_t *lens,
                        uint32_t count, StigiDoorResult *out, Diag *diag) {
  return run_transaction(door, srcs, lens, count, NULL, 0, false, false, out, diag);
}

Status stigi_door_drop(StigiDoor *door, const char *const *names, uint32_t count,
                       StigiDoorResult *out, Diag *diag) {
  return run_transaction(door, NULL, NULL, 0, names, count, false, true, out, diag);
}

Status stigi_door_tx(StigiDoor *door, const char *const *srcs, const size_t *lens,
                     uint32_t src_count, const char *const *drop_names, uint32_t drop_count,
                     bool commit, StigiDoorResult *out, Diag *diag) {
  return run_transaction(door, srcs, lens, src_count, drop_names, drop_count, false, commit,
                         out, diag);
}

Status stigi_door_replace_all(StigiDoor *door, const char *const *srcs, const size_t *lens,
                              uint32_t count, StigiDoorResult *out, Diag *diag) {
  return run_transaction(door, srcs, lens, count, NULL, 0, true, true, out, diag);
}

Status stigi_door_cache_add_verdict(StigiDoor *door, const uint8_t key[32], Diag *diag) {
  if (door == NULL || key == NULL) return diag_set(diag, STIGI_ERR_INVALID, -1, "invalid args");
  return keyset_insert(&door->verdict_cache, key, diag);
}

Status stigi_door_cache_add_test(StigiDoor *door, const uint8_t key[32], Diag *diag) {
  if (door == NULL || key == NULL) return diag_set(diag, STIGI_ERR_INVALID, -1, "invalid args");
  return keyset_insert(&door->test_cache, key, diag);
}

Status stigi_door_set_budget(StigiDoor *door, uint64_t new_budget,
                             StigiDoorResult *out, Diag *diag) {
  if (door == NULL || out == NULL) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_door_set_budget: invalid arguments");
  }
  out->accepted = false;
  if (new_budget == 0) {
    return add_reject(out, "Stigi.Datalog.rowBudget", "datalog-budget",
                      "rowBudget must be > 0", diag);
  }

  StigiDl tmp_dl;
  Diag od;
  diag_clear(&od);
  Status st = stigi_dl_open(&tmp_dl, door->rules_dl, door->rules_dl_len, "all.dl",
                            new_budget, 1, &od);
  if (st != STIGI_OK) {
    return add_reject(out, "Stigi.Datalog.rowBudget", tupu_code_to_stigi(od.dl_code),
                      od.msg, diag);
  }

  for (uint32_t i = 0; i < door->image.unit_count && st == STIGI_OK; i++) {
    StigiDeclKind k = door->image.units[i].ast->decl->kind;
    out->verdicts_computed++;
    if (k == STIGI_DECL_NEG_TEST) {
      st = check_negtest_unit(door, &tmp_dl, door->rules_dl, &door->image, i, out, diag);
    } else {
      st = eval_unit_datalog(&tmp_dl, door->kernel_mode, &door->image, i, out, diag);
    }
  }

  if (st == STIGI_OK && out->reject_count == 0) {
    stigi_dl_close(&door->dl);
    door->dl = tmp_dl;
    door->row_budget = new_budget;
    out->accepted = true;
    return STIGI_OK;
  }

  stigi_dl_close(&tmp_dl);
  return st;
}

/* Full image fact extraction including door metadata for true C7 tupu parity */
Status stigi_door_extract_image(const StigiDoor *door, StigiFactSet *fs, Diag *diag) {
  if (door == NULL || fs == NULL) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_door_extract_image: invalid arguments");
  }
  Status st = stigi_extract_image(&door->image, fs, diag);
  if (st != STIGI_OK) return st;

  const char *mode_s[1] = {door->kernel_mode ? "Kernel" : "User"};
  st = stigi_facts_add_sym(fs, "imageMode", 1, mode_s, diag);
  if (st != STIGI_OK) return st;

  for (uint32_t i = 0; i < door->image.unit_count && st == STIGI_OK; i++) {
    const StigiImageUnit *u = &door->image.units[i];
    const StigiDecl *d = u->ast->decl;
    if (d->kind == STIGI_DECL_TEST) {
      const char *tv[2] = {u->name, "ok"};
      st = stigi_facts_add_sym(fs, "testGot", 2, tv, diag);
      if (st == STIGI_OK) st = stigi_facts_add_sym(fs, "testWant", 2, tv, diag);
    } else if (d->kind == STIGI_DECL_NEG_TEST) {
      const StigiExpr *body = d->u.val_decl.body;
      if (body != NULL && body->kind == STIGI_EXPR_BLOCK) {
        for (uint32_t k = 0; k < body->u.block.count && st == STIGI_OK; k++) {
          const StigiExpr *s = body->u.block.stmts[k];
          if (s->kind == STIGI_EXPR_CALL &&
              s->u.call.callee->kind == STIGI_EXPR_GLOBAL &&
              s->u.call.callee->u.ident.name.len == 20 &&
              memcmp(s->u.call.callee->u.ident.name.ptr, "Stigi.NegTest.expect", 20) == 0 &&
              s->u.call.arg_count == 2 &&
              (s->u.call.args[0]->kind == STIGI_EXPR_GLOBAL ||
               s->u.call.args[0]->kind == STIGI_EXPR_LOCAL)) {
            char kebab[64];
            code_to_kebab(s->u.call.args[0]->u.ident.name, kebab, sizeof kebab);
            const char *ne[2] = {u->name, kebab};
            const char *nr[3] = {u->name, "s0", kebab};
            const char *cr[2] = {"kernel", kebab};
            st = stigi_facts_add_sym(fs, "negExpect", 2, ne, diag);
            if (st == STIGI_OK) st = stigi_facts_add_sym(fs, "negRejects", 3, nr, diag);
            if (st == STIGI_OK) st = stigi_facts_add_sym(fs, "clearsRule", 2, cr, diag);
          }
        }
      }
    }
  }
  return st;
}

Status stigi_door_export(const StigiDoor *door, const char *out_dl_path,
                         const char *out_facts_dir, Diag *diag) {
  if (door == NULL || out_dl_path == NULL || out_facts_dir == NULL) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_door_export: invalid arguments");
  }
  FILE *f = fopen(out_dl_path, "w");
  if (f == NULL) {
    return diag_set(diag, STIGI_ERR_IO, -1, "cannot write %s", out_dl_path);
  }
  if (fwrite(door->rules_dl, 1, door->rules_dl_len, f) != door->rules_dl_len) {
    (void)fclose(f);
    return diag_set(diag, STIGI_ERR_IO, -1, "write error on %s", out_dl_path);
  }
  if (fclose(f) != 0) {
    return diag_set(diag, STIGI_ERR_IO, -1, "close failed on %s", out_dl_path);
  }

  StigiFactSet fs;
  stigi_facts_init(&fs);
  Status st = stigi_door_extract_image(door, &fs, diag);
  if (st == STIGI_OK) {
    st = stigi_facts_write_dir(&fs, out_facts_dir, door->rules_dl, diag);
  }
  stigi_facts_free(&fs);
  return st;
}
