#define _POSIX_C_SOURCE 200809L

#include "door.h"
#include "blake3.h"
#include "eval.h"
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

/* ---- O(1) open-addressed key set cache (Fix #9) ---- */

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

static Status keyset_insert(StigiKeySet *ks, const uint8_t key[32], Diag *diag) {
  if (keyset_contains(ks, key)) return STIGI_OK;
  if (ks->cap == 0) {
    Status st = keyset_rehash(ks, 64U, diag);
    if (st != STIGI_OK) return st;
  } else if ((uint64_t)(ks->count + 1U) * 4ULL >= (uint64_t)ks->cap * 3ULL) {
    Status st = keyset_rehash(ks, ks->cap * 2U, diag);
    if (st != STIGI_OK) return st;
  }
  uint32_t mask = ks->cap - 1U;
  uint32_t idx = (uint32_t)(key_hash64(key) & mask);
  while (ks->slots[idx].used) idx = (idx + 1U) & mask;
  memcpy(ks->slots[idx].key, key, 32);
  ks->slots[idx].used = true;
  ks->count++;
  return STIGI_OK;
}

/* Fix #8: include active rule unit IDs + base rules hash + row_budget in verdict key */
static void compute_active_rules_hash(const StigiDoor *door, const StigiImage *img,
                                      uint8_t out[32]) {
  size_t rule_cnt = 0;
  for (uint32_t i = 0; i < img->unit_count; i++) {
    if (img->units[i].ast->decl->kind == STIGI_DECL_RULE) rule_cnt++;
  }
  size_t buf_len = 32U + rule_cnt * 32U;
  uint8_t *buf = malloc(buf_len);
  if (buf == NULL) {
    memcpy(out, door->base_rules_hash, 32);
    return;
  }
  memcpy(buf, door->base_rules_hash, 32);
  size_t off = 32U;
  for (uint32_t i = 0; i < img->unit_count; i++) {
    if (img->units[i].ast->decl->kind == STIGI_DECL_RULE) {
      memcpy(buf + off, img->units[i].verdict_key, 32);
      off += 32U;
    }
  }
  blake3_hash(buf, buf_len, out);
  free(buf);
}

static void effective_verdict_key(const uint8_t base_key[32], const uint8_t rules_hash[32],
                                  uint64_t budget, uint8_t out[32]) {
  uint8_t buf[72];
  memcpy(buf, base_key, 32);
  memcpy(buf + 32, rules_hash, 32);
  for (int i = 0; i < 8; i++) {
    buf[64 + i] = (uint8_t)((budget >> (i * 8)) & 0xFFU);
  }
  blake3_hash(buf, sizeof buf, out);
}

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

Status stigi_door_init(StigiDoor *door, const char *all_dl_path,
                       uint64_t row_budget, bool kernel_mode, Diag *diag) {
  if (door == NULL || all_dl_path == NULL) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_door_init: invalid arguments");
  }
  memset(door, 0, sizeof *door);
  stigi_image_init(&door->image);
  door->row_budget = row_budget > 0 ? row_budget : STIGI_DEFAULT_ROW_BUDGET;
  door->kernel_mode = kernel_mode;

  Status st = preprocess_dl(all_dl_path, &door->rules_dl, &door->rules_dl_len, diag);
  if (st != STIGI_OK) {
    stigi_door_free(door);
    return st;
  }
  blake3_hash((const uint8_t *)door->rules_dl, door->rules_dl_len, door->base_rules_hash);
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

static bool code_has_clearing_rule(const StigiDoor *door, const char *code) {
  if (door == NULL || door->rules_dl == NULL || code == NULL) return false;
  char pat[128];
  (void)snprintf(pat, sizeof pat, "\"%.60s\")", code);
  if (strstr(door->rules_dl, pat) != NULL) return true;
  (void)snprintf(pat, sizeof pat, "\"%.60s\"", code);
  return strstr(door->rules_dl, pat) != NULL;
}

static Status eval_unit_datalog(StigiDl *dl, bool kernel_mode, const StigiImage *img,
                                uint32_t uidx, StigiDoorResult *out, Diag *diag) {
  Status st = stigi_dl_reset(dl, diag);
  if (st != STIGI_OK) return st;

  /* Emit imageMode fact (Fix #3) */
  int rel_mode = stigi_dl_rel_id(dl, "imageMode");
  if (rel_mode >= 0) {
    const char *mstr = kernel_mode ? "Kernel" : "User";
    uint32_t mid = 0;
    st = stigi_dl_intern(dl, mstr, strlen(mstr), &mid, diag);
    if (st != STIGI_OK) return st;
    int64_t row[1] = {(int64_t)mid};
    st = stigi_dl_add(dl, rel_mode, row, 1, diag);
    if (st != STIGI_OK) return st;
  }

  StigiFactSet fs;
  stigi_facts_init(&fs);
  st = stigi_extract_unit(img, uidx, &fs, diag);
  if (st != STIGI_OK) {
    stigi_facts_free(&fs);
    return st;
  }
  st = stigi_facts_load_into_dl(&fs, dl, diag);
  stigi_facts_free(&fs);
  if (st != STIGI_OK) {
    if (st == STIGI_ERR_DATALOG) {
      return add_reject(out, img->units[uidx].name, "datalog-budget", diag->msg, diag);
    }
    return st;
  }

  Diag ed;
  diag_clear(&ed);
  st = stigi_dl_eval(dl, &ed);
  if (st != STIGI_OK) {
    return add_reject(out, img->units[uidx].name, "datalog-budget", ed.msg, diag);
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

/* Fix #5: resolve priv image, exact set equality, 1 subject per code, clearing-rule check */
static Status check_negtest_unit(StigiDoor *door, StigiDl *dl, const StigiImage *cand,
                                 uint32_t nt_idx, StigiDoorResult *out, Diag *diag) {
  const StigiImageUnit *nt_u = &cand->units[nt_idx];
  const StigiDecl *nt_d = nt_u->ast->decl;
  const StigiExpr *body = nt_d->u.val_decl.body;
  if (body == NULL || body->kind != STIGI_EXPR_BLOCK) {
    return add_reject(out, nt_u->name, "negtest-form", "NegTest body must be a block", diag);
  }

  char expected_codes[16][64];
  uint32_t exp_count = 0;

  StigiImage priv;
  stigi_image_init(&priv);
  StigiNameDiags pnd;
  stigi_name_diags_init(&pnd);

  for (uint32_t i = 0; i < cand->unit_count; i++) {
    if (cand->units[i].ast->decl->kind == STIGI_DECL_NEG_TEST) continue;
    Arena ta;
    arena_init(&ta);
    char *pr = NULL;
    size_t plen = 0;
    if (stigi_print_unit(cand->units[i].ast, &pr, &plen, &ta, diag) == STIGI_OK) {
      (void)stigi_image_add_source(&priv, pr, plen, &pnd, diag);
    }
    arena_free(&ta);
  }

  uint32_t local_start = priv.unit_count;
  for (uint32_t i = 0; i < body->u.block.count; i++) {
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
      if (stigi_print_unit(&tmp_u, &pr, &plen, &ta, diag) == STIGI_OK) {
        (void)stigi_image_add_source(&priv, pr, plen, &pnd, diag);
      }
      arena_free(&ta);
    } else if (s->kind == STIGI_EXPR_CALL &&
               s->u.call.callee->kind == STIGI_EXPR_GLOBAL &&
               s->u.call.callee->u.ident.name.len == 20 &&
               memcmp(s->u.call.callee->u.ident.name.ptr, "Stigi.NegTest.expect", 20) == 0 &&
               s->u.call.arg_count == 2 &&
               (s->u.call.args[0]->kind == STIGI_EXPR_GLOBAL ||
                s->u.call.args[0]->kind == STIGI_EXPR_LOCAL)) {
      if (exp_count < 16) {
        code_to_kebab(s->u.call.args[0]->u.ident.name, expected_codes[exp_count++], 64);
      }
    }
  }

  if (exp_count == 0) {
    stigi_name_diags_free(&pnd);
    stigi_image_free(&priv);
    return add_reject(out, nt_u->name, "negtest-form", "NegTest missing expect()", diag);
  }

  /* Verify every expected code has a clearing rule (T2: negtest-without-rule) */
  for (uint32_t e = 0; e < exp_count; e++) {
    if (!code_has_clearing_rule(door, expected_codes[e])) {
      stigi_name_diags_free(&pnd);
      stigi_image_free(&priv);
      return add_reject(out, nt_u->name, "negtest-without-rule", expected_codes[e], diag);
    }
  }

  for (uint32_t i = local_start; i < priv.unit_count; i++) {
    priv.units[i].is_root = true;
  }
  Status rst = stigi_image_resolve(&priv, &pnd, diag);
  StigiDoorResult priv_res;
  stigi_door_result_init(&priv_res);
  if (rst != STIGI_OK) {
    stigi_door_result_free(&priv_res);
    stigi_name_diags_free(&pnd);
    stigi_image_free(&priv);
    return rst;
  }
  for (uint32_t i = 0; i < pnd.count; i++) {
    if (pnd.items[i].code == STIGI_NAME_NO_READER || pnd.items[i].code == STIGI_NAME_PUB_UNUSED) {
      continue;
    }
    (void)add_reject(&priv_res, nt_u->name, stigi_name_code_name(pnd.items[i].code),
                     pnd.items[i].msg, diag);
  }

  StigiTypeDiags ptd;
  stigi_type_diags_init(&ptd);
  (void)stigi_typecheck_image(&priv, &ptd, diag);
  for (uint32_t i = 0; i < ptd.count; i++) {
    (void)add_reject(&priv_res, nt_u->name, stigi_type_code_name(ptd.items[i].code),
                     ptd.items[i].msg, diag);
  }
  stigi_type_diags_free(&ptd);

  for (uint32_t i = local_start; i < priv.unit_count; i++) {
    Status dst = eval_unit_datalog(dl, door->kernel_mode, &priv, i, &priv_res, diag);
    if (dst != STIGI_OK) {
      stigi_door_result_free(&priv_res);
      stigi_name_diags_free(&pnd);
      stigi_image_free(&priv);
      return dst;
    }
  }

  stigi_name_diags_free(&pnd);
  stigi_image_free(&priv);

  if (priv_res.reject_count == 0) {
    stigi_door_result_free(&priv_res);
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
      return add_reject(out, nt_u->name, "negtest-wrong-code", msg, diag);
    }
    if (hits > 1) {
      stigi_door_result_free(&priv_res);
      return add_reject(out, nt_u->name, "negtest-form",
                        "NegTest expected code matched multiple subjects", diag);
    }
  }
  for (uint32_t r = 0; r < priv_res.reject_count; r++) {
    bool in_exp = false;
    for (uint32_t e = 0; e < exp_count; e++) {
      if (strcmp(priv_res.rejects[r].code, expected_codes[e]) == 0) { in_exp = true; break; }
    }
    if (!in_exp) {
      char msg[256];
      (void)snprintf(msg, sizeof msg, "NegTest produced unexpected code '%.60s'",
                     priv_res.rejects[r].code);
      stigi_door_result_free(&priv_res);
      return add_reject(out, nt_u->name, "negtest-wrong-code", msg, diag);
    }
  }

  stigi_door_result_free(&priv_res);
  return STIGI_OK;
}

static Status run_transaction(StigiDoor *door, const char *const *add_srcs,
                              const size_t *add_lens, uint32_t add_count,
                              const char *const *drop_names, uint32_t drop_count,
                              bool replace_all, bool commit, StigiDoorResult *out, Diag *diag) {
  if (door == NULL || out == NULL) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "run_transaction: invalid arguments");
  }
  out->accepted = false;

  /* Step 1: Parse + Tenet gate on all incoming units */
  char **new_names = NULL;
  if (add_count > 0) {
    new_names = calloc(add_count, sizeof *new_names);
    if (new_names == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
  }

  Status tx_st = STIGI_OK;
  for (uint32_t i = 0; i < add_count && tx_st == STIGI_OK; i++) {
    StigiGateFindings gf;
    stigi_gate_findings_init(&gf);
    tx_st = stigi_gate_check_source(add_srcs[i], add_lens[i], &gf, diag);
    if (tx_st == STIGI_OK) {
      for (uint32_t k = 0; k < gf.count && tx_st == STIGI_OK; k++) {
        tx_st = add_reject(out, "<input>", stigi_gate_code_name(gf.items[k].code),
                           gf.items[k].msg, diag);
      }
    }
    stigi_gate_findings_free(&gf);
    if (tx_st != STIGI_OK) break;

    Arena pa;
    arena_init(&pa);
    StigiParseResult pr = stigi_parse_unit(add_srcs[i], add_lens[i], &pa);
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
    free(cand_srcs); free(cand_lens);
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
      if (cand_srcs[cand_count] == NULL) { tx_st = diag_set(diag, STIGI_ERR_OOM, -1, "OOM"); break; }
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

  /* Stage cache updates locally; merge ONLY on commit (Fix #1) */
  StigiKeySet staged_vkeys = {NULL, 0, 0};
  StigiKeySet staged_tkeys = {NULL, 0, 0};

  /* Step 2: Names + unit_id / verdict_key (C3, S119) + Kernel mode checks (Fix #3) */
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
  /* Fix #4: enforce ALL name diagnostics including STIGI_NAME_PUB_UNUSED */
  for (uint32_t i = 0; i < nd.count && tx_st == STIGI_OK; i++) {
    tx_st = add_reject(out, "<names>", stigi_name_code_name(nd.items[i].code),
                       nd.items[i].msg, diag);
  }
  stigi_name_diags_free(&nd);

  /* Fix #3: kernel_mode enforcement in user mode */
  if (tx_st == STIGI_OK && !door->kernel_mode) {
    for (uint32_t i = 0; i < cand.unit_count && tx_st == STIGI_OK; i++) {
      const StigiImageUnit *u = &cand.units[i];
      const StigiDecl *d = u->ast->decl;
      if (u->name_len >= 6 && memcmp(u->name, "Stigi.", 6) == 0) {
        tx_st = add_reject(out, u->name, "reserved-name",
                           "Stigi.* units are only allowed in kernel mode", diag);
      } else if (d->kind == STIGI_DECL_EXTERN) {
        tx_st = add_reject(out, u->name, "extern-denied",
                           "extern units are only allowed in kernel mode", diag);
      } else if (d->kind == STIGI_DECL_RULE && d->u.rule_decl.head.kind == STIGI_FACT_ATOM &&
                 d->u.rule_decl.head.atom_name.len >= 6 &&
                 memcmp(d->u.rule_decl.head.atom_name.ptr, "Stigi.", 6) == 0) {
        tx_st = add_reject(out, u->name, "rule-outside-kernel",
                           "rules targeting Stigi.* relations require kernel mode", diag);
      }
    }
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

  uint8_t rules_hash[32];
  compute_active_rules_hash(door, &cand, rules_hash);

  /* Steps 4 & 5: Extract facts + run Datalog per unit (including Tests, Fix #6) */
  if (tx_st == STIGI_OK && out->reject_count == 0) {
    for (uint32_t i = 0; i < cand.unit_count && tx_st == STIGI_OK; i++) {
      StigiDeclKind k = cand.units[i].ast->decl->kind;
      if (k == STIGI_DECL_NEG_TEST || k == STIGI_DECL_TEST) continue;
      uint8_t vkey[32];
      effective_verdict_key(cand.units[i].verdict_key, rules_hash, door->row_budget, vkey);
      if (keyset_contains(&door->verdict_cache, vkey)) {
        out->verdicts_cached++;
        continue;
      }
      out->verdicts_computed++;
      uint32_t before = out->reject_count;
      tx_st = eval_unit_datalog(&door->dl, door->kernel_mode, &cand, i, out, diag);
      if (tx_st == STIGI_OK && out->reject_count == before) {
        tx_st = keyset_insert(&staged_vkeys, vkey, diag);
      }
    }
  }

  /* Step 6: NegTests (cached by effective_verdict_key, Fix #5) */
  if (tx_st == STIGI_OK && out->reject_count == 0) {
    for (uint32_t i = 0; i < cand.unit_count && tx_st == STIGI_OK; i++) {
      if (cand.units[i].ast->decl->kind != STIGI_DECL_NEG_TEST) continue;
      uint8_t ntkey[32];
      effective_verdict_key(cand.units[i].verdict_key, rules_hash, door->row_budget, ntkey);
      if (keyset_contains(&door->test_cache, ntkey)) {
        out->tests_cached++;
        continue;
      }
      out->tests_computed++;
      uint32_t before = out->reject_count;
      tx_st = check_negtest_unit(door, &door->dl, &cand, i, out, diag);
      if (tx_st == STIGI_OK && out->reject_count == before) {
        tx_st = keyset_insert(&staged_tkeys, ntkey, diag);
      }
    }
  }

  /* Step 7: Tests & compile-time vals (Fix #6) */
  if (tx_st == STIGI_OK && out->reject_count == 0) {
    Arena eval_arena;
    arena_init(&eval_arena);
    for (uint32_t i = 0; i < cand.unit_count && tx_st == STIGI_OK; i++) {
      StigiDeclKind k = cand.units[i].ast->decl->kind;
      if (k == STIGI_DECL_TEST) {
        uint8_t tkey[32];
        effective_verdict_key(cand.units[i].verdict_key, rules_hash, door->row_budget, tkey);
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

  /* Step 8: Atomic commit or rollback (T0, Fixes #1 & #2) */
  if (tx_st == STIGI_OK && out->reject_count == 0) {
    if (commit) {
      for (uint32_t i = 0; i < staged_vkeys.cap && tx_st == STIGI_OK; i++) {
        if (staged_vkeys.slots[i].used) {
          tx_st = keyset_insert(&door->verdict_cache, staged_vkeys.slots[i].key, diag);
        }
      }
      for (uint32_t i = 0; i < staged_tkeys.cap && tx_st == STIGI_OK; i++) {
        if (staged_tkeys.slots[i].used) {
          tx_st = keyset_insert(&door->test_cache, staged_tkeys.slots[i].key, diag);
        }
      }
    }
    if (tx_st == STIGI_OK) {
      out->accepted = true;
      keyset_free(&staged_vkeys);
      keyset_free(&staged_tkeys);
      if (commit) {
        stigi_image_free(&door->image);
        door->image = cand;
        for (uint32_t i = 0; i < door->src_count; i++) free(door->unit_srcs[i]);
        free(door->unit_srcs);
        free(door->unit_lens);
        door->unit_srcs = cand_srcs;
        door->unit_lens = cand_lens;
        door->src_count = cand_count;
        door->src_cap = max_srcs;
        return STIGI_OK;
      }
      stigi_image_free(&cand);
      for (uint32_t i = 0; i < cand_count; i++) free(cand_srcs[i]);
      free(cand_srcs);
      free(cand_lens);
      return STIGI_OK;
    }
  }

  /* Rollback: live door->image and door caches are 100% untouched */
  keyset_free(&staged_vkeys);
  keyset_free(&staged_tkeys);
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
  return run_transaction(door, srcs, lens, src_count, drop_names, drop_count, false, commit, out, diag);
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

/* Fix #7 & #2: reject new_budget == 0, re-check NegTests too, propagate Status */
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
  Status st = stigi_dl_open(&tmp_dl, door->rules_dl, door->rules_dl_len, "all.dl",
                            new_budget, 1, diag);
  if (st != STIGI_OK) {
    return add_reject(out, "Stigi.Datalog.rowBudget", "datalog-budget", diag->msg, diag);
  }

  for (uint32_t i = 0; i < door->image.unit_count && st == STIGI_OK; i++) {
    StigiDeclKind k = door->image.units[i].ast->decl->kind;
    out->verdicts_computed++;
    if (k == STIGI_DECL_NEG_TEST) {
      st = check_negtest_unit(door, &tmp_dl, &door->image, i, out, diag);
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
  (void)fclose(f);

  StigiFactSet fs;
  stigi_facts_init(&fs);
  Status st = stigi_extract_image(&door->image, &fs, diag);
  if (st == STIGI_OK) {
    st = stigi_facts_write_dir(&fs, out_facts_dir, door->rules_dl, diag);
  }
  stigi_facts_free(&fs);
  return st;
}
