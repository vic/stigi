#define _GNU_SOURCE

#include "server.h"
#include "datalog.h"
#include "emit.h"
#include "extract.h"
#include "json_util.h"
#include "parse.h"
#include "print.h"
#include "store.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

typedef struct {
  char *buf;
  size_t len;
  size_t cap;
  bool oom;
} ReplyBuf;

static void rb_grow(ReplyBuf *b, size_t add) {
  if (b->oom) return;
  if (add > SIZE_MAX - b->len - 1U) { b->oom = true; return; }
  if (b->len + add + 1U <= b->cap) return;
  size_t ncap = b->cap == 0 ? 512U : b->cap;
  while (ncap < b->len + add + 1U) {
    if (ncap > SIZE_MAX / 2U) { b->oom = true; return; }
    ncap *= 2U;
  }
  char *nb = realloc(b->buf, ncap);
  if (nb == NULL) { b->oom = true; return; }
  b->buf = nb;
  b->cap = ncap;
}

static void rb_mem(ReplyBuf *b, const char *s, size_t n) {
  if (n == 0 || b->oom) return;
  rb_grow(b, n);
  if (b->oom) return;
  memcpy(b->buf + b->len, s, n);
  b->len += n;
  b->buf[b->len] = '\0';
}

static void rb_cstr(ReplyBuf *b, const char *s) {
  if (s != NULL) rb_mem(b, s, strlen(s));
}

static void rb_fmt(ReplyBuf *b, const char *fmt, ...) {
  if (b->oom) return;
  va_list ap, ap2;
  va_start(ap, fmt);
  va_copy(ap2, ap);
  int n = vsnprintf(NULL, 0, fmt, ap2);
  va_end(ap2);
  if (n <= 0) {
    va_end(ap);
    return;
  }
  size_t need = (size_t)n;
  rb_grow(b, need);
  if (!b->oom) {
    (void)vsnprintf(b->buf + b->len, need + 1U, fmt, ap);
    b->len += need;
  }
  va_end(ap);
}

static void rb_json_escaped(ReplyBuf *b, const char *s, size_t len) {
  rb_mem(b, "\"", 1);
  for (size_t i = 0; i < len; i++) {
    unsigned char c = (unsigned char)s[i];
    if (c == '"') rb_cstr(b, "\\\"");
    else if (c == '\\') rb_cstr(b, "\\\\");
    else if (c == '\n') rb_cstr(b, "\\n");
    else if (c == '\r') rb_cstr(b, "\\r");
    else if (c == '\t') rb_cstr(b, "\\t");
    else if (c < 0x20U) rb_fmt(b, "\\u%04x", (unsigned)c);
    else rb_mem(b, &s[i], 1);
  }
  rb_mem(b, "\"", 1);
}

static void hex64(const uint8_t id[32], char out[65]) {
  static const char H[] = "0123456789abcdef";
  for (int i = 0; i < 32; i++) {
    out[i * 2] = H[(id[i] >> 4) & 0x0f];
    out[i * 2 + 1] = H[id[i] & 0x0f];
  }
  out[64] = '\0';
}

static void clear_fixes(StigiServerCtx *ctx) {
  for (uint32_t i = 0; i < ctx->fix_count; i++) {
    free(ctx->fixes[i].fixed_src);
  }
  ctx->fix_count = 0;
}

void stigi_server_init(StigiServerCtx *ctx, StigiDoor *door) {
  if (ctx == NULL) return;
  memset(ctx, 0, sizeof *ctx);
  ctx->door = door;
  arena_init(&ctx->scratch);
}

void stigi_server_free(StigiServerCtx *ctx) {
  if (ctx == NULL) return;
  clear_fixes(ctx);
  free(ctx->fixes);
  arena_free(&ctx->scratch);
  memset(ctx, 0, sizeof *ctx);
}

static Status add_checked_fix(StigiServerCtx *ctx, const char *unit_name,
                              const char *desc, const char *src, size_t len,
                              char out_id[16], Diag *diag) {
  if (ctx->fix_count == ctx->fix_cap) {
    uint32_t ncap = ctx->fix_cap == 0 ? 8U : ctx->fix_cap * 2U;
    if (ncap <= ctx->fix_cap) return diag_set(diag, STIGI_ERR_OVERFLOW, -1, "fix list overflow");
    StigiCheckedFix *nf = realloc(ctx->fixes, (size_t)ncap * sizeof *nf);
    if (nf == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
    ctx->fixes = nf;
    ctx->fix_cap = ncap;
  }
  StigiCheckedFix *f = &ctx->fixes[ctx->fix_count++];
  memset(f, 0, sizeof *f);
  (void)snprintf(f->id, sizeof f->id, "fix%u", ctx->fix_count);
  (void)snprintf(f->unit_name, sizeof f->unit_name, "%s", unit_name != NULL ? unit_name : "");
  (void)snprintf(f->desc, sizeof f->desc, "%s", desc != NULL ? desc : "");
  f->fixed_src = malloc(len + 1U);
  if (f->fixed_src == NULL) {
    ctx->fix_count--;
    return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
  }
  memcpy(f->fixed_src, src, len);
  f->fixed_src[len] = '\0';
  f->fixed_len = len;
  if (out_id != NULL) (void)snprintf(out_id, 16, "%s", f->id);
  return STIGI_OK;
}

static int cmp_img_unit_name(const void *pa, const void *pb) {
  const StigiImageUnit *a = *(const StigiImageUnit *const *)pa;
  const StigiImageUnit *b = *(const StigiImageUnit *const *)pb;
  return strcmp(a->name, b->name);
}

static int cmp_cstr_ptr(const void *pa, const void *pb) {
  return strcmp(*(const char *const *)pa, *(const char *const *)pb);
}

static const char *decl_kind_label(StigiDeclKind k) {
  switch (k) {
    case STIGI_DECL_ENUM: return "enum";
    case STIGI_DECL_TYPE: return "type";
    case STIGI_DECL_DEF: return "def";
    case STIGI_DECL_LEMMA_DEF: return "lemma";
    case STIGI_DECL_VAL: return "val";
    case STIGI_DECL_CAPABILITY: return "capability";
    case STIGI_DECL_HANDLER: return "handler";
    case STIGI_DECL_EXTERN: return "extern";
    case STIGI_DECL_RELATION: return "relation";
    case STIGI_DECL_RULE: return "rule";
    case STIGI_DECL_TEST: return "test";
    case STIGI_DECL_NEG_TEST: return "negtest";
  }
  return "unit";
}

/* Extract full header signature (multi-line safe, zero body leak) for `summary` and `laws` */
static void append_unit_signature(StigiServerCtx *ctx, ReplyBuf *rb, const StigiImageUnit *u) {
  arena_reset(&ctx->scratch);
  char *printed = NULL;
  size_t plen = 0;
  Diag d;
  diag_clear(&d);
  if (stigi_print_unit(u->ast, &printed, &plen, &ctx->scratch, &d) != STIGI_OK || printed == NULL) {
    return;
  }
  const char *p = printed;
  const char *end = printed + plen;
  /* Skip leading prompt comments and local alias lines */
  while (p < end) {
    const char *nl = memchr(p, '\n', (size_t)(end - p));
    size_t llen = nl != NULL ? (size_t)(nl - p) : (size_t)(end - p);
    if (llen == 0 || strncmp(p, "//", 2) == 0 || strncmp(p, "alias ", 6) == 0) {
      p = nl != NULL ? nl + 1 : end;
      continue;
    }
    break;
  }
  /* Emit header lines up to top-level '=' (for def/val/type) or ':' (for enum/cap/handler) */
  int paren = 0, brace = 0, angle = 0;
  bool in_str = false;
  bool emitted = false;
  while (p < end) {
    const char *nl = memchr(p, '\n', (size_t)(end - p));
    const char *lend = nl != NULL ? nl : end;
    const char *cut = lend;
    bool stop = false;
    for (const char *c = p; c < lend; c++) {
      if (in_str) {
        if (*c == '\\' && c + 1 < lend) { c++; continue; }
        if (*c == '"') in_str = false;
        continue;
      }
      if (*c == '"') { in_str = true; continue; }
      if (*c == '(') paren++;
      else if (*c == ')' && paren > 0) paren--;
      else if (*c == '{') brace++;
      else if (*c == '}' && brace > 0) brace--;
      else if (*c == '<') angle++;
      else if (*c == '>' && angle > 0) angle--;
      else if (paren == 0 && brace == 0 && angle == 0) {
        if (*c == '=' && !(c + 1 < lend && c[1] == '=') && !(c > p && (c[-1] == '=' || c[-1] == '!' || c[-1] == '<' || c[-1] == '>' || c[-1] == ':'))) {
          cut = c;
          while (cut > p && cut[-1] == ' ') cut--;
          stop = true;
          break;
        }
        if (*c == ':' && c + 1 == lend) {
          cut = lend;
          stop = true;
          break;
        }
      }
    }
    rb_mem(rb, p, (size_t)(cut - p));
    rb_cstr(rb, "\n");
    emitted = true;
    if (stop || nl == NULL) break;
    p = nl + 1;
  }
  if (!emitted && u->name != NULL) {
    rb_fmt(rb, "%s\n", u->name);
  }
}

static bool has_prefix_span(const char *name, const char *prefix, size_t prefix_len) {
  if (prefix == NULL || prefix_len == 0) return true;
  return strncmp(name, prefix, prefix_len) == 0;
}

static char *dup_span_cstr(const char *s, size_t len) {
  char *out = malloc(len + 1U);
  if (out == NULL) return NULL;
  if (len > 0) memcpy(out, s, len);
  out[len] = '\0';
  return out;
}

/* Try to synthesize & verify a checked fix for a rejected unit (S59/S114).
 * Uses AST spans (after prompt comments) and appends to existing entry facts.
 * ONLY offers a fix if stigi_door_check() accepts it! */
static bool try_synthesize_fix(StigiServerCtx *ctx, const char *src, size_t len,
                               const StigiDoorResult *rej, char out_fix_id[16],
                               char out_desc[256], Diag *diag) {
  if (rej->reject_count == 0) return false;
  bool is_idx_unproved = false;
  for (uint32_t i = 0; i < rej->reject_count; i++) {
    if (strcmp(rej->rejects[i].code, "index-unproved") == 0) {
      is_idx_unproved = true;
      break;
    }
  }
  if (!is_idx_unproved) return false;

  arena_reset(&ctx->scratch);
  StigiParseResult pr = stigi_parse_unit(src, len, &ctx->scratch);
  if (pr.status != STIGI_OK || pr.unit == NULL || pr.unit->decl == NULL ||
      pr.unit->decl->kind != STIGI_DECL_DEF) {
    return false;
  }

  const StigiDefHead *h = &pr.unit->decl->u.def_decl.head;
  StigiStr len_name = {NULL, 0};
  StigiStr idx_name = {NULL, 0};
  for (uint32_t i = 0; i < h->param_count; i++) {
    if (h->params[i].type->kind == STIGI_TYPE_ARRAY && h->params[i].type->len_arg_count > 0 &&
        h->params[i].type->len_args[0]->kind == STIGI_EXPR_LOCAL) {
      len_name = h->params[i].type->len_args[0]->u.ident.name;
    }
  }
  for (uint32_t i = 0; i < h->param_count; i++) {
    if (h->params[i].type->kind == STIGI_TYPE_NAMED &&
        h->params[i].type->name.len == 3 &&
        memcmp(h->params[i].type->name.ptr, "U64", 3) == 0 &&
        !(h->params[i].name.len == len_name.len &&
          memcmp(h->params[i].name.ptr, len_name.ptr, len_name.len) == 0)) {
      idx_name = h->params[i].name;
    }
  }
  if (len_name.len == 0 || idx_name.len == 0) return false;

  size_t decl_off = pr.unit->decl->span.offset;
  if (decl_off >= len) return false;

  ReplyBuf fb = {NULL, 0, 0, false};
  if (h->post_val_fact_count > 0) {
    /* Append `, below(idx, len)` right before the closing `>` of the existing entry fact group */
    StigiSpan fsp = h->post_val_facts[0].span;
    if (fsp.offset >= decl_off && fsp.offset < len && src[fsp.offset] == '<') {
      const char *gt = memchr(src + fsp.offset + 1U, '>', len - (size_t)fsp.offset - 1U);
      if (gt != NULL) {
        size_t ins = (size_t)(gt - src);
        rb_mem(&fb, src, ins);
        rb_fmt(&fb, ", below(%.*s, %.*s)", (int)idx_name.len, idx_name.ptr,
               (int)len_name.len, len_name.ptr);
        rb_mem(&fb, src + ins, len - ins);
      }
    }
  } else {
    /* Insert ` <below(idx, len)>` before `: ` after parameter list (starting after decl_off) */
    size_t scan_start = decl_off;
    if (h->param_count > 0) {
      size_t pend = (size_t)h->params[h->param_count - 1U].span.offset +
                    (size_t)h->params[h->param_count - 1U].span.len;
      if (pend <= len) scan_start = pend;
    }
    const char *p = src + scan_start;
    const char *end = src + len;
    const char *colon = NULL;
    while (p < end) {
      if (*p == ':') { colon = p; break; }
      if (*p == '=' || *p == '\n') break;
      p++;
    }
    if (colon != NULL) {
      size_t ins = (size_t)(colon - src);
      rb_mem(&fb, src, ins);
      rb_fmt(&fb, " <below(%.*s, %.*s)>", (int)idx_name.len, idx_name.ptr,
             (int)len_name.len, len_name.ptr);
      rb_mem(&fb, src + ins, len - ins);
    }
  }

  if (fb.oom || fb.buf == NULL || fb.len == 0) {
    free(fb.buf);
    return false;
  }

  const char *csrcs[1] = {fb.buf};
  size_t clens[1] = {fb.len};
  StigiDoorResult chk;
  stigi_door_result_init(&chk);
  Status st = stigi_door_check(ctx->door, csrcs, clens, 1, &chk, diag);
  bool ok = (st == STIGI_OK && chk.accepted);
  stigi_door_result_free(&chk);

  if (ok) {
    char uname[128];
    (void)snprintf(uname, sizeof uname, "%.*s",
                   (int)pr.unit->decl->name.len, pr.unit->decl->name.ptr);
    (void)snprintf(out_desc, 256, "add <below(%.*s, %.*s)> to %s",
                   (int)idx_name.len, idx_name.ptr,
                   (int)len_name.len, len_name.ptr, uname);
    if (add_checked_fix(ctx, uname, out_desc, fb.buf, fb.len, out_fix_id, diag) == STIGI_OK) {
      free(fb.buf);
      return true;
    }
  }
  free(fb.buf);
  return false;
}

typedef struct {
  bool is_drop;
  char *text;
  size_t len;
  char *unit_name;
} BatchOp;

static void free_batch_ops(BatchOp *ops, uint32_t cnt) {
  for (uint32_t i = 0; i < cnt; i++) {
    free(ops[i].text);
    free(ops[i].unit_name);
  }
  free(ops);
}

static bool is_batch_op_start(const char *p, const char *end) {
  size_t rem = (size_t)(end - p);
  if (rem >= 4U && memcmp(p, "new ", 4) == 0) return true;
  if (rem >= 5U && memcmp(p, "edit ", 5) == 0) return true;
  if (rem >= 5U && memcmp(p, "drop ", 5) == 0) return true;
  if (rem >= 5U && memcmp(p, "test ", 5) == 0) return true;
  return false;
}

static const char *memmem_bounded(const char *hay, size_t hlen, const char *needle, size_t nlen) {
  if (nlen == 0 || nlen > hlen) return NULL;
  for (size_t i = 0; i + nlen <= hlen; i++) {
    if (memcmp(hay + i, needle, nlen) == 0) return hay + i;
  }
  return NULL;
}

/* Find top-level '==' outside strings and nested delimiters for `test <lhs> == <rhs>` */
static const char *find_top_level_eqeq(const char *s, size_t len) {
  int paren = 0, brack = 0, brace = 0;
  bool in_str = false;
  bool in_chr = false;
  for (size_t i = 0; i + 1U < len; i++) {
    char c = s[i];
    if (in_str) {
      if (c == '\\' && i + 1U < len) { i++; continue; }
      if (c == '"') in_str = false;
      continue;
    }
    if (in_chr) {
      if (c == '\\' && i + 1U < len) { i++; continue; }
      if (c == '\'') in_chr = false;
      continue;
    }
    if (c == '"') { in_str = true; continue; }
    if (c == '\'') { in_chr = true; continue; }
    if (c == '(') paren++;
    else if (c == ')' && paren > 0) paren--;
    else if (c == '[') brack++;
    else if (c == ']' && brack > 0) brack--;
    else if (c == '{') brace++;
    else if (c == '}' && brace > 0) brace--;
    else if (paren == 0 && brack == 0 && brace == 0 && c == '=' && s[i + 1U] == '=') {
      return s + i;
    }
  }
  return NULL;
}

/* Splice `edit <Name>.body[<i>..<j>] := <statements>` using AST body span (skips comments/headers) */
static Status splice_body_edit(StigiServerCtx *ctx, const char *uname, size_t ulen,
                               uint32_t line_lo, uint32_t line_hi,
                               const char *repl, size_t repl_len,
                               char **out_src, size_t *out_len, Diag *diag) {
  int32_t idx = stigi_image_find(&ctx->door->image, uname, ulen);
  if (idx < 0 || (uint32_t)idx >= ctx->door->src_count) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "unknown unit for body edit");
  }
  const char *orig = ctx->door->unit_srcs[idx];
  size_t orig_len = ctx->door->unit_lens[idx];

  const char *body_start = orig;
  const StigiDecl *decl = ctx->door->image.units[idx].ast != NULL
                              ? ctx->door->image.units[idx].ast->decl
                              : NULL;
  const StigiExpr *body_e = NULL;
  if (decl != NULL) {
    if (decl->kind == STIGI_DECL_DEF || decl->kind == STIGI_DECL_LEMMA_DEF) {
      body_e = decl->u.def_decl.body;
    } else if (decl->kind == STIGI_DECL_VAL || decl->kind == STIGI_DECL_TEST ||
               decl->kind == STIGI_DECL_NEG_TEST) {
      body_e = decl->u.val_decl.body;
    }
  }
  if (body_e != NULL && body_e->span.offset < orig_len) {
    /* Rewind to start of body's first line (preserving indentation) or right after '=' */
    size_t off = body_e->span.offset;
    while (off > 0 && orig[off - 1U] != '\n') off--;
    if (off > 0) {
      body_start = orig + off;
    } else {
      body_start = orig + body_e->span.offset;
    }
  } else {
    size_t decl_off = (decl != NULL && decl->span.offset < orig_len) ? decl->span.offset : 0U;
    const char *eq = memchr(orig + decl_off, '=', orig_len - decl_off);
    if (eq != NULL) {
      const char *nl = memchr(eq, '\n', (size_t)((orig + orig_len) - eq));
      body_start = nl != NULL ? nl + 1 : eq + 1;
    }
  }

  ReplyBuf rb = {NULL, 0, 0, false};
  rb_mem(&rb, orig, (size_t)(body_start - orig));

  const char *p = body_start;
  const char *end = orig + orig_len;
  uint32_t cur_line = 0;
  bool inserted = false;
  while (p < end) {
    const char *nl = memchr(p, '\n', (size_t)(end - p));
    const char *next = nl != NULL ? nl + 1 : end;
    if (cur_line < line_lo || cur_line > line_hi) {
      rb_mem(&rb, p, (size_t)(next - p));
    } else if (!inserted) {
      rb_mem(&rb, repl, repl_len);
      if (repl_len == 0 || repl[repl_len - 1U] != '\n') rb_cstr(&rb, "\n");
      inserted = true;
    }
    cur_line++;
    p = next;
  }
  if (!inserted) {
    rb_mem(&rb, repl, repl_len);
    if (repl_len == 0 || repl[repl_len - 1U] != '\n') rb_cstr(&rb, "\n");
  }
  if (rb.oom || rb.buf == NULL) {
    free(rb.buf);
    return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
  }
  *out_src = rb.buf;
  *out_len = rb.len;
  return STIGI_OK;
}

static Status parse_batch_ops(StigiServerCtx *ctx, const char *body, size_t len,
                              BatchOp **out_ops, uint32_t *out_cnt, Diag *diag) {
  BatchOp *ops = NULL;
  uint32_t cnt = 0, cap = 0;
  uint32_t synth_test_idx = 0;
  const char *p = body;
  const char *end = body + len;

  while (p < end) {
    while (p < end && (*p == '\n' || *p == '\r')) p++;
    if (p >= end) break;

    if (cnt >= STIGI_MAX_BATCH_OPS) {
      free_batch_ops(ops, cnt);
      return diag_set(diag, STIGI_ERR_OVERFLOW, -1, "batch exceeds max ops (%u)", STIGI_MAX_BATCH_OPS);
    }

    const char *line1_nl = memchr(p, '\n', (size_t)(end - p));
    const char *line1_end = line1_nl != NULL ? line1_nl : end;
    if (line1_end > p && line1_end[-1] == '\r') line1_end--;

    bool is_drop = false;
    bool is_test = false;
    bool is_body_edit = false;
    const char *be_name = NULL;
    size_t be_nlen = 0;
    uint32_t be_lo = 0, be_hi = 0;

    const char *start = p;
    size_t rem = (size_t)(end - p);
    if (rem >= 4U && memcmp(p, "new ", 4) == 0) {
      start = p + 4;
    } else if (rem >= 5U && memcmp(p, "edit ", 5) == 0) {
      /* Restrict `:=` to line 1 of edit */
      const char *eq = memmem_bounded(p + 5, (size_t)(line1_end - (p + 5)), ":=", 2U);
      if (eq != NULL) {
        /* Check for `<Name>.body[<i>..<j>]` before `:=` */
        const char *hdr = p + 5;
        size_t hlen = (size_t)(eq - hdr);
        while (hlen > 0 && hdr[hlen - 1U] == ' ') hlen--;
        const char *btag = memmem_bounded(hdr, hlen, ".body[", 6U);
        if (btag != NULL && hlen > 0 && hdr[hlen - 1U] == ']') {
          const char *dots = memmem_bounded(btag + 6, (size_t)((hdr + hlen - 1U) - (btag + 6)), "..", 2U);
          if (dots != NULL) {
            be_name = hdr;
            be_nlen = (size_t)(btag - hdr);
            char lo_buf[32], hi_buf[32];
            size_t lo_l = (size_t)(dots - (btag + 6));
            size_t hi_l = (size_t)((hdr + hlen - 1U) - (dots + 2));
            if (lo_l > 0 && lo_l < sizeof lo_buf && hi_l > 0 && hi_l < sizeof hi_buf) {
              memcpy(lo_buf, btag + 6, lo_l); lo_buf[lo_l] = '\0';
              memcpy(hi_buf, dots + 2, hi_l); hi_buf[hi_l] = '\0';
              char *e1 = NULL, *e2 = NULL;
              unsigned long v1 = strtoul(lo_buf, &e1, 10);
              unsigned long v2 = strtoul(hi_buf, &e2, 10);
              if (e1 != NULL && *e1 == '\0' && e2 != NULL && *e2 == '\0' && v1 <= v2) {
                be_lo = (uint32_t)v1;
                be_hi = (uint32_t)v2;
                is_body_edit = true;
              }
            }
          }
        }
        start = eq + 2;
        while (start < end && (*start == ' ' || *start == '\r' || *start == '\n')) start++;
      } else {
        start = p + 5;
      }
    } else if (rem >= 5U && memcmp(p, "drop ", 5) == 0) {
      is_drop = true;
      start = p + 5;
    } else if (rem >= 5U && memcmp(p, "test ", 5) == 0) {
      is_test = true;
      start = p + 5;
    }

    const char *q = start;
    while (q < end) {
      const char *nl = memchr(q, '\n', (size_t)(end - q));
      if (nl == NULL) { q = end; break; }
      const char *nxt = nl + 1;
      if (is_drop || is_test) { q = nl; break; }
      if (is_batch_op_start(nxt, end)) { q = nxt; break; }
      q = nxt;
    }

    size_t olen = (size_t)(q - start);
    while (olen > 0 && (start[olen - 1U] == '\r' || ((is_drop || is_test) && start[olen - 1U] == ' '))) {
      olen--;
    }

    if (olen > 0) {
      if (cnt == cap) {
        uint32_t ncap = cap == 0 ? 8U : cap * 2U;
        BatchOp *no = realloc(ops, (size_t)ncap * sizeof *no);
        if (no == NULL) {
          free_batch_ops(ops, cnt);
          return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
        }
        ops = no;
        cap = ncap;
      }
      ops[cnt].is_drop = is_drop;
      ops[cnt].text = NULL;
      ops[cnt].len = 0;
      ops[cnt].unit_name = NULL;

      if (is_test) {
        /* Synthesize a tenet-gate-clean Stigi Test unit for `test <lhs> == <rhs>` */
        const char *eqeq = find_top_level_eqeq(start, olen);
        if (eqeq == NULL) {
          ops[cnt].text = dup_span_cstr(start, olen);
          ops[cnt].len = olen;
        } else {
          const char *lhs = start;
          size_t lhsl = (size_t)(eqeq - start);
          while (lhsl > 0 && lhs[lhsl - 1U] == ' ') lhsl--;
          const char *rhs = eqeq + 2;
          size_t rhsl = (size_t)((start + olen) - rhs);
          while (rhsl > 0 && *rhs == ' ') { rhs++; rhsl--; }
          uint32_t sidx = synth_test_idx++;
          char c0 = (char)('A' + ((sidx / 676U) % 26U));
          char c1 = (char)('a' + ((sidx / 26U) % 26U));
          char c2 = (char)('a' + (sidx % 26U));
          ReplyBuf tb = {NULL, 0, 0, false};
          rb_fmt(&tb,
                 "// prompt: batch inline test %c%c%c.\n"
                 "val Batch.Tests.inlineTest%c%c%c {Stigi.Test}: Stigi.Test.Result =\n"
                 "  Stigi.Test.expect(%.*s, %.*s, \"batch test %c%c%c\")\n",
                 c0, c1, c2, c0, c1, c2, (int)lhsl, lhs, (int)rhsl, rhs, c0, c1, c2);
          if (tb.oom || tb.buf == NULL) {
            free(tb.buf);
            free_batch_ops(ops, cnt);
            return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
          }
          ops[cnt].text = tb.buf;
          ops[cnt].len = tb.len;
        }
      } else if (is_body_edit) {
        Status sp_st = splice_body_edit(ctx, be_name, be_nlen, be_lo, be_hi,
                                        start, olen, &ops[cnt].text, &ops[cnt].len, diag);
        if (sp_st != STIGI_OK) {
          /* Keep raw text so component reports error cleanly */
          ops[cnt].text = dup_span_cstr(start, olen);
          ops[cnt].len = olen;
        }
      } else {
        ops[cnt].text = dup_span_cstr(start, olen);
        ops[cnt].len = olen;
      }

      if (ops[cnt].text == NULL) {
        free_batch_ops(ops, cnt);
        return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
      }
      cnt++;
    }
    p = q;
  }

  *out_ops = ops;
  *out_cnt = cnt;
  return STIGI_OK;
}

/* ---- DSU for S114 dependency-closed component partitioning ---- */

static uint32_t dsu_find(uint32_t *parent, uint32_t x) {
  while (parent[x] != x) {
    parent[x] = parent[parent[x]];
    x = parent[x];
  }
  return x;
}

static void dsu_union(uint32_t *parent, uint32_t a, uint32_t b) {
  uint32_t ra = dsu_find(parent, a);
  uint32_t rb = dsu_find(parent, b);
  if (ra != rb) {
    if (ra < rb) parent[rb] = ra;
    else parent[ra] = rb;
  }
}

static bool is_ident_char(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9') || c == '_' || c == '.';
}

static bool text_mentions_name(const char *text, size_t tlen, const char *name) {
  if (name == NULL || name[0] == '\0') return false;
  size_t nlen = strlen(name);
  const char *p = text;
  const char *end = text + tlen;
  while (p < end) {
    const char *hit = memmem_bounded(p, (size_t)(end - p), name, nlen);
    if (hit == NULL) break;
    bool left_ok = (hit == text || !is_ident_char(hit[-1]));
    bool right_ok = (hit + nlen == end || !is_ident_char(hit[nlen]));
    if (left_ok && right_ok) return true;
    p = hit + 1;
  }
  return false;
}

static bool same_or_ancestor_namespace(const char *child, const char *cand) {
  if (child == NULL || cand == NULL) return false;
  const char *dc = strrchr(child, '.');
  const char *da = strrchr(cand, '.');
  if (da == NULL) return true; /* top-level cand is visible in all namespaces */
  if (dc == NULL) return false;
  size_t lc = (size_t)(dc - child);
  size_t la = (size_t)(da - cand);
  if (la == lc && memcmp(child, cand, la) == 0) return true;
  if (la < lc && memcmp(child, cand, la) == 0 && child[la] == '.') return true;
  return false;
}

static Status finalize_reply(const StigiServerCtx *ctx, ReplyBuf *rb, const char *status_tag,
                             const StigiDoorResult *diag_res,
                             char **out_reply, size_t *out_len, Diag *diag) {
  if (rb->oom) {
    free(rb->buf);
    return diag_set(diag, STIGI_ERR_OOM, -1, "reply OOM");
  }
  if (rb->buf == NULL) {
    rb->buf = malloc(1U);
    if (rb->buf == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
    rb->buf[0] = '\0';
    rb->len = 0;
  }

  /* Budget truncation at line boundary without * 4U overflow (frames.md 3) */
  if (ctx->budget_tokens > 0 && ctx->budget_tokens <= SIZE_MAX / 4U) {
    size_t max_bytes = ctx->budget_tokens * 4U;
    if (rb->len > max_bytes) {
      size_t cut = max_bytes;
      while (cut > 0 && rb->buf[cut - 1U] != '\n') cut--;
      rb->len = cut;
      rb->buf[rb->len] = '\0';
      rb_cstr(rb, "... (budget truncated)\n");
      if (rb->oom) {
        free(rb->buf);
        return diag_set(diag, STIGI_ERR_OOM, -1, "reply OOM");
      }
    }
  }

  if (ctx->json_mode) {
    ReplyBuf jb = {NULL, 0, 0, false};
    rb_cstr(&jb, "{\"status\":");
    rb_json_escaped(&jb, status_tag, strlen(status_tag));
    rb_cstr(&jb, ",\"reply\":");
    rb_json_escaped(&jb, rb->buf, rb->len);
    rb_cstr(&jb, ",\"diagnostics\":[");
    if (diag_res != NULL) {
      for (uint32_t i = 0; i < diag_res->reject_count; i++) {
        if (i > 0) rb_cstr(&jb, ",");
        const char *code = diag_res->rejects[i].code;
        const char *subj = diag_res->rejects[i].subject;
        const char *msg = diag_res->rejects[i].msg;
        rb_cstr(&jb, "{\"code\":");
        rb_json_escaped(&jb, code, strlen(code));
        rb_cstr(&jb, ",\"subject\":");
        rb_json_escaped(&jb, subj, strlen(subj));
        rb_cstr(&jb, ",\"unit\":");
        rb_json_escaped(&jb, subj, strlen(subj));
        rb_cstr(&jb, ",\"line\":1,\"col\":1,\"expected\":");
        rb_json_escaped(&jb, code, strlen(code));
        rb_cstr(&jb, ",\"found\":");
        rb_json_escaped(&jb, msg, strlen(msg));
        rb_cstr(&jb, ",\"help\":\"see Stigi.Code and vic/spec/obligations.md\"");
        rb_cstr(&jb, ",\"message\":");
        rb_json_escaped(&jb, msg, strlen(msg));
        rb_cstr(&jb, "}");
      }
    }
    rb_cstr(&jb, "]}\n");
    free(rb->buf);
    if (jb.oom || jb.buf == NULL) {
      free(jb.buf);
      return diag_set(diag, STIGI_ERR_OOM, -1, "json OOM");
    }
    *out_reply = jb.buf;
    if (out_len != NULL) *out_len = jb.len;
    return STIGI_OK;
  }

  *out_reply = rb->buf;
  if (out_len != NULL) *out_len = rb->len;
  return STIGI_OK;
}

static Status append_rejects_to_res(StigiDoorResult *dst, const StigiDoorResult *src, Diag *diag) {
  for (uint32_t i = 0; i < src->reject_count; i++) {
    if (dst->reject_count == dst->reject_cap) {
      uint32_t ncap = dst->reject_cap == 0 ? 8U : dst->reject_cap * 2U;
      StigiDoorReject *nr = realloc(dst->rejects, (size_t)ncap * sizeof *nr);
      if (nr == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
      dst->rejects = nr;
      dst->reject_cap = ncap;
    }
    dst->rejects[dst->reject_count++] = src->rejects[i];
  }
  return STIGI_OK;
}

static Status handle_query_frame(StigiServerCtx *ctx, const char *arg, size_t arg_len,
                                 const char *rest, size_t rest_len,
                                 ReplyBuf *rb, const char **status_tag, Diag *diag) {
  /* Extract optional relation name from arg or first token of rest */
  const char *qptr = arg_len > 0 ? arg : rest;
  size_t qlen = arg_len > 0 ? arg_len : rest_len;
  while (qlen > 0 && (*qptr == ' ' || *qptr == '\t' || *qptr == '\n' || *qptr == '\r')) {
    qptr++;
    qlen--;
  }
  size_t rname_len = 0;
  while (rname_len < qlen && is_ident_char(qptr[rname_len])) rname_len++;

  StigiFactSet fs;
  stigi_facts_init(&fs);
  Status st = stigi_door_extract_image(ctx->door, &fs, diag);
  if (st != STIGI_OK) {
    stigi_facts_free(&fs);
    return st;
  }

  if (rname_len == 0) {
    arena_reset(&ctx->scratch);
    char *printed = NULL;
    size_t plen = 0;
    st = stigi_facts_print(&fs, &printed, &plen, &ctx->scratch, diag);
    stigi_facts_free(&fs);
    if (st != STIGI_OK) return st;
    rb_fmt(rb, "accepted query (%u facts)\n", fs.count);
    if (printed != NULL && plen > 0) rb_mem(rb, printed, plen);
    return STIGI_OK;
  }

  char *rname = dup_span_cstr(qptr, rname_len);
  if (rname == NULL) {
    stigi_facts_free(&fs);
    return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
  }

  /* Query derived or EDB relation via Datalog session */
  int rel_id = stigi_dl_rel_id(&ctx->door->dl, rname);
  if (rel_id >= 0) {
    st = stigi_dl_reset(&ctx->door->dl, diag);
    if (st == STIGI_OK) st = stigi_facts_load_into_dl(&fs, &ctx->door->dl, diag);
    if (st == STIGI_OK) st = stigi_dl_eval(&ctx->door->dl, diag);
    if (st == STIGI_OK) {
      int ncols = stigi_dl_rel_ncols(&ctx->door->dl, rel_id);
      uint32_t nrows = stigi_dl_count(&ctx->door->dl, rel_id);
      uint64_t limit = ctx->door->row_budget;
      rb_fmt(rb, "accepted query %s (%u rows)\n", rname, nrows);
      int64_t vals[STIGI_MAX_FACT_COLS];
      if (ncols > 0 && ncols <= STIGI_MAX_FACT_COLS) {
        for (uint32_t r = 0; r < nrows && (uint64_t)r < limit; r++) {
          if (stigi_dl_read(&ctx->door->dl, rel_id, r, vals, ncols, diag) != STIGI_OK) break;
          rb_fmt(rb, "%s(", rname);
          for (int c = 0; c < ncols; c++) {
            if (c > 0) rb_cstr(rb, ", ");
            size_t slen = 0;
            const char *sym = (vals[c] >= 0 && vals[c] <= (int64_t)UINT32_MAX)
                                  ? stigi_dl_sym_str(&ctx->door->dl, (uint32_t)vals[c], &slen)
                                  : NULL;
            if (sym != NULL) rb_mem(rb, sym, slen);
            else rb_fmt(rb, "%lld", (long long)vals[c]);
          }
          rb_cstr(rb, ")\n");
        }
      }
    }
    free(rname);
    stigi_facts_free(&fs);
    return st;
  }

  /* Fallback: check extracted EDB rows directly */
  uint32_t matched = 0;
  for (uint32_t i = 0; i < fs.count; i++) {
    if (strcmp(fs.rows[i].rel, rname) == 0) matched++;
  }
  if (matched > 0) {
    rb_fmt(rb, "accepted query %s (%u rows)\n", rname, matched);
    for (uint32_t i = 0; i < fs.count; i++) {
      if (strcmp(fs.rows[i].rel, rname) != 0) continue;
      rb_fmt(rb, "%s(", rname);
      for (int c = 0; c < fs.rows[i].ncols; c++) {
        if (c > 0) rb_cstr(rb, ", ");
        if (fs.rows[i].cols[c].is_num) {
          char nbuf[48];
          (void)stigi_i128_format(fs.rows[i].cols[c].num, nbuf);
          rb_cstr(rb, nbuf);
        } else {
          rb_cstr(rb, fs.rows[i].cols[c].sym);
        }
      }
      rb_cstr(rb, ")\n");
    }
  } else {
    *status_tag = "error";
    rb_fmt(rb, "error unknown relation '%s'\n", rname);
  }
  free(rname);
  stigi_facts_free(&fs);
  return STIGI_OK;
}

Status stigi_handle_frame(StigiServerCtx *ctx, const char *frame, size_t len,
                          char **out_reply, size_t *out_len, Diag *diag) {
  if (ctx == NULL || ctx->door == NULL || frame == NULL || out_reply == NULL) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_handle_frame: invalid arguments");
  }

  ReplyBuf rb = {NULL, 0, 0, false};

  /* Reject embedded NUL bytes in text frames */
  if (memchr(frame, '\0', len) != NULL) {
    rb_cstr(&rb, "error frame contains embedded NUL byte\n");
    return finalize_reply(ctx, &rb, "error", NULL, out_reply, out_len, diag);
  }

  /* Trim leading/trailing whitespace */
  while (len > 0 && (*frame == ' ' || *frame == '\n' || *frame == '\r')) {
    frame++;
    len--;
  }
  while (len > 0 && (frame[len - 1U] == '\n' || frame[len - 1U] == '\r' || frame[len - 1U] == ' ')) {
    len--;
  }

  /* Extract first line (stripping trailing \r) & command word */
  const char *nl = memchr(frame, '\n', len);
  size_t raw_flen = nl != NULL ? (size_t)(nl - frame) : len;
  size_t flen = raw_flen;
  if (flen > 0 && frame[flen - 1U] == '\r') flen--;

  const char *sp = memchr(frame, ' ', flen);
  size_t cmd_len = sp != NULL ? (size_t)(sp - frame) : flen;
  const char *arg = sp != NULL ? sp + 1 : "";
  size_t arg_len = sp != NULL ? flen - cmd_len - 1U : 0;
  while (arg_len > 0 && *arg == ' ') { arg++; arg_len--; }
  while (arg_len > 0 && arg[arg_len - 1U] == ' ') arg_len--;

  const char *rest = nl != NULL ? nl + 1 : "";
  size_t rest_len = nl != NULL ? len - raw_flen - 1U : 0;

  const char *status_tag = "accepted";
  const StigiImage *img = &ctx->door->image;

  if (cmd_len == 4 && memcmp(frame, "stop", 4) == 0) {
    ctx->stopped = true;
    rb_cstr(&rb, "accepted stop\n");
    return finalize_reply(ctx, &rb, "accepted", NULL, out_reply, out_len, diag);
  }

  if (cmd_len == 4 && memcmp(frame, "json", 4) == 0) {
    if (arg_len == 2 && memcmp(arg, "on", 2) == 0) {
      ctx->json_mode = true;
      rb_cstr(&rb, "accepted json on\n");
    } else if (arg_len == 3 && memcmp(arg, "off", 3) == 0) {
      ctx->json_mode = false;
      rb_cstr(&rb, "accepted json off\n");
    } else {
      status_tag = "error";
      rb_cstr(&rb, "error expected 'json on' or 'json off'\n");
    }
    return finalize_reply(ctx, &rb, status_tag, NULL, out_reply, out_len, diag);
  }

  if (cmd_len == 6 && memcmp(frame, "budget", 6) == 0) {
    bool valid = (arg_len > 0);
    for (size_t i = 0; i < arg_len && valid; i++) {
      if (arg[i] < '0' || arg[i] > '9') valid = false;
    }
    if (!valid) {
      rb_fmt(&rb, "error invalid budget '%.*s'\n", (int)arg_len, arg);
      return finalize_reply(ctx, &rb, "error", NULL, out_reply, out_len, diag);
    }
    char *ab = dup_span_cstr(arg, arg_len);
    if (ab == NULL) {
      free(rb.buf);
      return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
    }
    errno = 0;
    char *ep = NULL;
    unsigned long long val = strtoull(ab, &ep, 10);
    bool ok = (errno == 0 && ep != NULL && *ep == '\0' && val <= (unsigned long long)SIZE_MAX);
    free(ab);
    if (!ok) {
      rb_fmt(&rb, "error budget overflow '%.*s'\n", (int)arg_len, arg);
      return finalize_reply(ctx, &rb, "error", NULL, out_reply, out_len, diag);
    }
    ctx->budget_tokens = (size_t)val;
    rb_fmt(&rb, "accepted budget %zu\n", ctx->budget_tokens);
    return finalize_reply(ctx, &rb, "accepted", NULL, out_reply, out_len, diag);
  }

  if (cmd_len == 5 && memcmp(frame, "tests", 5) == 0) {
    uint32_t n_test = 0, n_neg = 0;
    for (uint32_t i = 0; i < img->unit_count; i++) {
      StigiDeclKind k = img->units[i].ast->decl->kind;
      if (k == STIGI_DECL_TEST) n_test++;
      else if (k == STIGI_DECL_NEG_TEST) n_neg++;
    }
    rb_fmt(&rb, "accepted tests %u/%u   negtests %u/%u   verdicts_cached %u   tests_cached %u\n",
           n_test, n_test, n_neg, n_neg,
           ctx->door->verdict_cache.count, ctx->door->test_cache.count);
    return finalize_reply(ctx, &rb, "accepted", NULL, out_reply, out_len, diag);
  }

  /* Sorted unit views for summary / laws / list / find */
  if ((cmd_len == 7 && memcmp(frame, "summary", 7) == 0) ||
      (cmd_len == 4 && memcmp(frame, "laws", 4) == 0) ||
      (cmd_len == 4 && memcmp(frame, "list", 4) == 0) ||
      (cmd_len == 4 && memcmp(frame, "find", 4) == 0)) {
    const StigiImageUnit **sorted = NULL;
    if (img->unit_count > 0) {
      sorted = malloc((size_t)img->unit_count * sizeof *sorted);
      if (sorted == NULL) {
        free(rb.buf);
        return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
      }
      for (uint32_t i = 0; i < img->unit_count; i++) sorted[i] = &img->units[i];
      qsort(sorted, img->unit_count, sizeof *sorted, cmp_img_unit_name);
    }

    if (cmd_len == 7) { /* summary [PREFIX] */
      for (uint32_t i = 0; i < img->unit_count; i++) {
        if (!has_prefix_span(sorted[i]->name, arg, arg_len)) continue;
        append_unit_signature(ctx, &rb, sorted[i]);
      }
    } else if (memcmp(frame, "laws", 4) == 0) { /* laws [PREFIX] */
      for (uint32_t i = 0; i < img->unit_count; i++) {
        if (sorted[i]->ast->decl->kind != STIGI_DECL_LEMMA_DEF) continue;
        if (!has_prefix_span(sorted[i]->name, arg, arg_len)) continue;
        char h[65];
        hex64(sorted[i]->unit_id, h);
        rb_fmt(&rb, "law %s %s\n  ", sorted[i]->name, h);
        append_unit_signature(ctx, &rb, sorted[i]);
        /* Include sorted dependency units (door.md 9) */
        uint32_t u_idx = sorted[i]->idx;
        if (img->adj_head != NULL && u_idx < img->adj_cap && img->adj_head[u_idx] != UINT32_MAX) {
          uint32_t dcnt = 0;
          for (uint32_t e = img->adj_head[u_idx]; e != UINT32_MAX; e = img->edge_next[e]) dcnt++;
          const char **dnames = malloc((size_t)dcnt * sizeof *dnames);
          if (dnames != NULL) {
            uint32_t k = 0;
            for (uint32_t e = img->adj_head[u_idx]; e != UINT32_MAX; e = img->edge_next[e]) {
              dnames[k++] = img->units[img->edges[e].to].name;
            }
            if (dcnt > 1U) qsort(dnames, dcnt, sizeof *dnames, cmp_cstr_ptr);
            rb_cstr(&rb, "  depends:");
            for (uint32_t j = 0; j < dcnt; j++) {
              if (j > 0 && strcmp(dnames[j], dnames[j - 1U]) == 0) continue;
              rb_fmt(&rb, " %s", dnames[j]);
            }
            rb_cstr(&rb, "\n");
            free(dnames);
          }
        }
      }
    } else if (memcmp(frame, "list", 4) == 0) { /* list [PREFIX] */
      for (uint32_t i = 0; i < img->unit_count; i++) {
        if (!has_prefix_span(sorted[i]->name, arg, arg_len)) continue;
        char h[65];
        hex64(sorted[i]->unit_id, h);
        rb_fmt(&rb, "%s %s %s\n", sorted[i]->name,
               decl_kind_label(sorted[i]->ast->decl->kind), h);
      }
    } else { /* find TEXT */
      for (uint32_t i = 0; i < img->unit_count; i++) {
        arena_reset(&ctx->scratch);
        char *pr = NULL;
        size_t plen = 0;
        if (stigi_print_unit(sorted[i]->ast, &pr, &plen, &ctx->scratch, diag) == STIGI_OK &&
            pr != NULL &&
            (arg_len == 0 || memmem_bounded(pr, plen, arg, arg_len) != NULL)) {
          rb_fmt(&rb, "%s\n", sorted[i]->name);
        }
      }
    }
    free(sorted);
    return finalize_reply(ctx, &rb, "accepted", NULL, out_reply, out_len, diag);
  }

  if (cmd_len == 4 && memcmp(frame, "show", 4) == 0) {
    const char *p = arg;
    const char *end = arg + arg_len;
    while (p < end) {
      while (p < end && *p == ' ') p++;
      if (p >= end) break;
      const char *start = p;
      while (p < end && *p != ' ') p++;
      int32_t idx = stigi_image_find(img, start, (size_t)(p - start));
      if (idx < 0) {
        status_tag = "error";
        rb_fmt(&rb, "error unknown unit '%.*s'\n", (int)(p - start), start);
      } else {
        arena_reset(&ctx->scratch);
        char *pr = NULL;
        size_t plen = 0;
        Status pst = stigi_print_unit(img->units[idx].ast, &pr, &plen, &ctx->scratch, diag);
        if (pst != STIGI_OK || pr == NULL) {
          status_tag = "error";
          rb_fmt(&rb, "error show '%.*s': %s\n", (int)(p - start), start,
                 diag != NULL && diag->msg[0] ? diag->msg : "print failed");
        } else {
          rb_mem(&rb, pr, plen);
        }
      }
    }
    return finalize_reply(ctx, &rb, status_tag, NULL, out_reply, out_len, diag);
  }

  if (cmd_len == 5 && memcmp(frame, "store", 5) == 0) {
    /* door.md:113: `store DIR [NAME...]` */
    const char *p = arg;
    const char *end = arg + arg_len;
    while (p < end && *p == ' ') p++;
    const char *dstart = p;
    while (p < end && *p != ' ') p++;
    char *dir_cstr = dup_span_cstr(dstart, (size_t)(p - dstart));
    if (dir_cstr == NULL) {
      free(rb.buf);
      return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
    }
    char **snames = NULL;
    uint32_t sncnt = 0, sncap = 0;
    while (p < end) {
      while (p < end && *p == ' ') p++;
      if (p >= end) break;
      const char *ns = p;
      while (p < end && *p != ' ') p++;
      if (sncnt == sncap) {
        uint32_t nn = sncap == 0 ? 8U : sncap * 2U;
        char **na = realloc(snames, (size_t)nn * sizeof *na);
        if (na == NULL) {
          for (uint32_t k = 0; k < sncnt; k++) free(snames[k]);
          free(snames);
          free(dir_cstr);
          free(rb.buf);
          return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
        }
        snames = na;
        sncap = nn;
      }
      snames[sncnt] = dup_span_cstr(ns, (size_t)(p - ns));
      if (snames[sncnt] == NULL) {
        for (uint32_t k = 0; k < sncnt; k++) free(snames[k]);
        free(snames);
        free(dir_cstr);
        free(rb.buf);
        return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
      }
      sncnt++;
    }
    char ebuf[256] = {0};
    Status st = stigi_store_dir_subset(ctx->door, dir_cstr,
                                       (const char *const *)snames, sncnt,
                                       ebuf, sizeof ebuf);
    if (st != STIGI_OK) {
      status_tag = "error";
      rb_fmt(&rb, "error store: %s\n", ebuf);
    } else {
      rb_fmt(&rb, "accepted store %s (%u units)\n", dir_cstr, sncnt > 0 ? sncnt : img->unit_count);
    }
    for (uint32_t k = 0; k < sncnt; k++) free(snames[k]);
    free(snames);
    free(dir_cstr);
    return finalize_reply(ctx, &rb, status_tag, NULL, out_reply, out_len, diag);
  }

  if (cmd_len == 4 && memcmp(frame, "load", 4) == 0) {
    clear_fixes(ctx);
    char *dir_cstr = dup_span_cstr(arg, arg_len);
    if (dir_cstr == NULL) {
      free(rb.buf);
      return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
    }
    StigiDoorResult r;
    stigi_door_result_init(&r);
    Status st = stigi_load_dir(ctx->door, dir_cstr, &r, diag);
    if (st != STIGI_OK || !r.accepted) {
      status_tag = "rejected";
      if (r.reject_count == 0 && diag != NULL && diag->msg[0] != '\0') {
        rb_fmt(&rb, "rejected load %s: %s\n", dir_cstr, diag->msg);
      } else {
        rb_fmt(&rb, "rejected load %s\n", dir_cstr);
      }
      for (uint32_t i = 0; i < r.reject_count; i++) {
        rb_fmt(&rb, "  error[%s] %s: %s\n", r.rejects[i].code, r.rejects[i].subject, r.rejects[i].msg);
      }
    } else {
      rb_fmt(&rb, "accepted load %s (%u units, computed=%u cached=%u)\n",
             dir_cstr, ctx->door->image.unit_count, r.verdicts_computed, r.verdicts_cached);
    }
    free(dir_cstr);
    Status fin = finalize_reply(ctx, &rb, status_tag, &r, out_reply, out_len, diag);
    stigi_door_result_free(&r);
    return fin;
  }

  if (cmd_len == 6 && memcmp(frame, "export", 6) == 0) {
    char *path_cstr = dup_span_cstr(arg, arg_len);
    if (path_cstr == NULL) {
      free(rb.buf);
      return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
    }
    size_t fl = arg_len + 9U;
    char *fdir = malloc(fl);
    if (fdir == NULL) {
      free(path_cstr);
      free(rb.buf);
      return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
    }
    (void)snprintf(fdir, fl, "%s.facts.d", path_cstr);
    (void)mkdir(fdir, 0755);
    Status st = stigi_door_export(ctx->door, path_cstr, fdir, diag);
    if (st != STIGI_OK) {
      status_tag = "error";
      rb_fmt(&rb, "error export: %s\n", diag != NULL ? diag->msg : "failed");
    } else {
      rb_fmt(&rb, "accepted export %s\n", path_cstr);
    }
    free(fdir);
    free(path_cstr);
    return finalize_reply(ctx, &rb, status_tag, NULL, out_reply, out_len, diag);
  }

  if (cmd_len == 4 && memcmp(frame, "emit", 4) == 0) {
    char *path_cstr = dup_span_cstr(arg, arg_len);
    if (path_cstr == NULL) {
      free(rb.buf);
      return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
    }
    char *csrc = NULL;
    size_t clen = 0;
    StigiEmitStats es;
    Status st = stigi_emit_c(ctx->door, &csrc, &clen, &es, diag);
    if (st != STIGI_OK) {
      status_tag = "error";
      rb_fmt(&rb, "error emit: %s\n", diag != NULL ? diag->msg : "failed");
    } else {
      FILE *ef = fopen(path_cstr, "wb");
      if (ef == NULL || fwrite(csrc, 1, clen, ef) != clen) {
        if (ef != NULL) (void)fclose(ef);
        status_tag = "error";
        rb_fmt(&rb, "error emit: cannot write '%s'\n", path_cstr);
      } else if (fclose(ef) != 0) {
        status_tag = "error";
        rb_fmt(&rb, "error emit: close failed for '%s'\n", path_cstr);
      } else {
        rb_fmt(&rb, "accepted emit %s (%zu bytes, masks=%u)\n", path_cstr, clen, es.spectre_masks);
      }
      free(csrc);
    }
    free(path_cstr);
    return finalize_reply(ctx, &rb, status_tag, NULL, out_reply, out_len, diag);
  }

  if (cmd_len == 4 && memcmp(frame, "drop", 4) == 0) {
    clear_fixes(ctx);
    char **names = NULL;
    uint32_t ncnt = 0, ncap = 0;
    const char *p = arg;
    const char *end = arg + arg_len;
    while (p < end) {
      while (p < end && *p == ' ') p++;
      if (p >= end) break;
      const char *start = p;
      while (p < end && *p != ' ') p++;
      if (ncnt == ncap) {
        uint32_t nn = ncap == 0 ? 8U : ncap * 2U;
        char **na = realloc(names, (size_t)nn * sizeof *na);
        if (na == NULL) {
          for (uint32_t i = 0; i < ncnt; i++) free(names[i]);
          free(names);
          free(rb.buf);
          return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
        }
        names = na;
        ncap = nn;
      }
      names[ncnt] = dup_span_cstr(start, (size_t)(p - start));
      if (names[ncnt] == NULL) {
        for (uint32_t i = 0; i < ncnt; i++) free(names[i]);
        free(names);
        free(rb.buf);
        return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
      }
      ncnt++;
    }
    StigiDoorResult r;
    stigi_door_result_init(&r);
    Status st = stigi_door_drop(ctx->door, (const char *const *)names, ncnt, &r, diag);
    for (uint32_t i = 0; i < ncnt; i++) free(names[i]);
    free(names);
    if (st != STIGI_OK) {
      stigi_door_result_free(&r);
      free(rb.buf);
      return st;
    }
    if (!r.accepted) {
      status_tag = "rejected";
      rb_cstr(&rb, "rejected drop\n");
      for (uint32_t i = 0; i < r.reject_count; i++) {
        rb_fmt(&rb, "  error[%s] %s: %s\n", r.rejects[i].code, r.rejects[i].subject, r.rejects[i].msg);
      }
    } else {
      rb_fmt(&rb, "accepted drop (%u units remaining)\n", ctx->door->image.unit_count);
    }
    Status fin = finalize_reply(ctx, &rb, status_tag, &r, out_reply, out_len, diag);
    stigi_door_result_free(&r);
    return fin;
  }

  if (cmd_len == 6 && memcmp(frame, "submit", 6) == 0) {
    clear_fixes(ctx);
    const char *s[1] = {rest};
    size_t l[1] = {rest_len};
    StigiDoorResult r;
    stigi_door_result_init(&r);
    Status st = stigi_door_submit(ctx->door, s, l, 1, &r, diag);
    if (st != STIGI_OK) {
      stigi_door_result_free(&r);
      free(rb.buf);
      return st;
    }
    if (!r.accepted) {
      status_tag = "rejected";
      rb_cstr(&rb, "rejected submit\n");
      for (uint32_t i = 0; i < r.reject_count; i++) {
        rb_fmt(&rb, "  error[%s] %s: %s\n", r.rejects[i].code, r.rejects[i].subject, r.rejects[i].msg);
      }
    } else {
      rb_fmt(&rb, "accepted submit (computed=%u cached=%u)\n",
             r.verdicts_computed, r.verdicts_cached);
    }
    Status fin = finalize_reply(ctx, &rb, status_tag, &r, out_reply, out_len, diag);
    stigi_door_result_free(&r);
    return fin;
  }

  /* S114: batch / check (dependency-closed component admission + checked fixes) */
  if ((cmd_len == 5 && memcmp(frame, "batch", 5) == 0) ||
      (cmd_len == 5 && memcmp(frame, "check", 5) == 0)) {
    bool is_batch = (memcmp(frame, "batch", 5) == 0);
    clear_fixes(ctx);
    BatchOp *ops = NULL;
    uint32_t ocnt = 0;
    Status pst = parse_batch_ops(ctx, rest, rest_len, &ops, &ocnt, diag);
    if (pst != STIGI_OK) {
      free(rb.buf);
      return pst;
    }

    if (ocnt == 0) {
      free_batch_ops(ops, ocnt);
      rb_cstr(&rb, "accepted empty\n");
      return finalize_reply(ctx, &rb, "accepted", NULL, out_reply, out_len, diag);
    }

    /* Extract declared unit names for each op */
    for (uint32_t i = 0; i < ocnt; i++) {
      if (ops[i].is_drop) {
        ops[i].unit_name = dup_span_cstr(ops[i].text, ops[i].len);
      } else {
        arena_reset(&ctx->scratch);
        StigiParseResult pr = stigi_parse_unit(ops[i].text, ops[i].len, &ctx->scratch);
        if (pr.status == STIGI_OK && pr.unit != NULL && pr.unit->decl != NULL) {
          ops[i].unit_name = dup_span_cstr(pr.unit->decl->name.ptr, pr.unit->decl->name.len);
        }
      }
    }

    /* Partition ops into dependency-closed components via Union-Find */
    uint32_t *parent = malloc((size_t)ocnt * sizeof *parent);
    if (parent == NULL) {
      free_batch_ops(ops, ocnt);
      free(rb.buf);
      return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
    }
    for (uint32_t i = 0; i < ocnt; i++) parent[i] = i;

    for (uint32_t i = 0; i < ocnt; i++) {
      for (uint32_t j = 0; j < ocnt; j++) {
        if (i == j) continue;
        if (ops[i].unit_name != NULL && ops[j].unit_name != NULL &&
            strcmp(ops[i].unit_name, ops[j].unit_name) == 0) {
          dsu_union(parent, i, j);
          continue;
        }
        if (!ops[i].is_drop && ops[j].unit_name != NULL) {
          if (text_mentions_name(ops[i].text, ops[i].len, ops[j].unit_name)) {
            dsu_union(parent, i, j);
            continue;
          }
          const char *dot = strrchr(ops[j].unit_name, '.');
          if (dot != NULL && same_or_ancestor_namespace(ops[i].unit_name, ops[j].unit_name) &&
              text_mentions_name(ops[i].text, ops[i].len, dot + 1)) {
            dsu_union(parent, i, j);
            continue;
          }
          /* Rules and relations affect all units in the batch */
          if (memmem_bounded(ops[i].text, ops[i].len, "rule ", 5U) != NULL ||
              memmem_bounded(ops[i].text, ops[i].len, "relation ", 9U) != NULL) {
            dsu_union(parent, i, j);
            continue;
          }
        }
      }
    }

    /* Process each component in deterministic order of its minimum op index */
    bool *visited = calloc(ocnt, sizeof *visited);
    const char **add_srcs = malloc((size_t)ocnt * sizeof *add_srcs);
    size_t *add_lens = malloc((size_t)ocnt * sizeof *add_lens);
    const char **drop_names = malloc((size_t)ocnt * sizeof *drop_names);
    uint32_t *comp_ops = malloc((size_t)ocnt * sizeof *comp_ops);
    if (visited == NULL || add_srcs == NULL || add_lens == NULL ||
        drop_names == NULL || comp_ops == NULL) {
      free(visited); free(add_srcs); free(add_lens); free(drop_names); free(comp_ops);
      free(parent);
      free_batch_ops(ops, ocnt);
      free(rb.buf);
      return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
    }

    StigiDoorResult all_diags;
    stigi_door_result_init(&all_diags);
    uint32_t comp_num = 0;
    Status tx_err = STIGI_OK;

    for (uint32_t i = 0; i < ocnt && tx_err == STIGI_OK; i++) {
      uint32_t root = dsu_find(parent, i);
      if (visited[root]) continue;
      visited[root] = true;
      comp_num++;

      uint32_t acnt = 0, dcnt = 0, mcnt = 0;
      for (uint32_t j = 0; j < ocnt; j++) {
        if (dsu_find(parent, j) != root) continue;
        comp_ops[mcnt++] = j;
        if (ops[j].is_drop) {
          drop_names[dcnt++] = ops[j].text;
        } else {
          add_srcs[acnt] = ops[j].text;
          add_lens[acnt] = ops[j].len;
          acnt++;
        }
      }

      StigiDoorResult r;
      stigi_door_result_init(&r);
      tx_err = stigi_door_tx(ctx->door, add_srcs, add_lens, acnt,
                             drop_names, dcnt, is_batch, &r, diag);
      if (tx_err == STIGI_OK) {
        if (r.accepted) {
          rb_fmt(&rb, "admitted component %u   laws ok\n", comp_num);
        } else {
          status_tag = "rejected";
          (void)append_rejects_to_res(&all_diags, &r, diag);
          for (uint32_t k = 0; k < r.reject_count; k++) {
            rb_fmt(&rb, "rejected: %s at %s: %s\n",
                   r.rejects[k].code, r.rejects[k].subject, r.rejects[k].msg);
          }
          for (uint32_t m = 0; m < mcnt; m++) {
            uint32_t idx = comp_ops[m];
            char fix_id[16] = {0}, fix_desc[256] = {0};
            if (!ops[idx].is_drop &&
                try_synthesize_fix(ctx, ops[idx].text, ops[idx].len, &r, fix_id, fix_desc, diag)) {
              rb_fmt(&rb, "  %s (checked): %s\n", fix_id, fix_desc);
            }
          }
        }
      }
      stigi_door_result_free(&r);
    }

    free(visited);
    free(add_srcs);
    free(add_lens);
    free(drop_names);
    free(comp_ops);
    free(parent);
    free_batch_ops(ops, ocnt);

    if (tx_err != STIGI_OK) {
      stigi_door_result_free(&all_diags);
      free(rb.buf);
      return tx_err;
    }
    Status fin = finalize_reply(ctx, &rb, status_tag, &all_diags, out_reply, out_len, diag);
    stigi_door_result_free(&all_diags);
    return fin;
  }

  /* S114: accept <fix>... (atomic multi-fix admission + clear fixes) */
  if (cmd_len == 6 && memcmp(frame, "accept", 6) == 0) {
    uint32_t *fix_idxs = NULL;
    uint32_t fcnt = 0, fcap = 0;
    const char *p = arg;
    const char *end = arg + arg_len;
    bool missing = false;

    while (p < end) {
      while (p < end && *p == ' ') p++;
      if (p >= end) break;
      const char *start = p;
      while (p < end && *p != ' ') p++;
      size_t idl = (size_t)(p - start);
      int32_t found = -1;
      for (uint32_t i = 0; i < ctx->fix_count; i++) {
        if (strlen(ctx->fixes[i].id) == idl && memcmp(ctx->fixes[i].id, start, idl) == 0) {
          found = (int32_t)i;
          break;
        }
      }
      if (found < 0) {
        missing = true;
        status_tag = "error";
        rb_fmt(&rb, "error unknown fix '%.*s'\n", (int)idl, start);
      } else {
        if (fcnt == fcap) {
          uint32_t nf = fcap == 0 ? 8U : fcap * 2U;
          uint32_t *ni = realloc(fix_idxs, (size_t)nf * sizeof *ni);
          if (ni == NULL) {
            free(fix_idxs);
            free(rb.buf);
            return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
          }
          fix_idxs = ni;
          fcap = nf;
        }
        fix_idxs[fcnt++] = (uint32_t)found;
      }
    }

    if (missing) {
      free(fix_idxs);
      return finalize_reply(ctx, &rb, "error", NULL, out_reply, out_len, diag);
    }
    if (fcnt == 0) {
      free(fix_idxs);
      rb_cstr(&rb, "error accept requires a fix id\n");
      return finalize_reply(ctx, &rb, "error", NULL, out_reply, out_len, diag);
    }

    const char **fsrcs = malloc((size_t)fcnt * sizeof *fsrcs);
    size_t *flens = malloc((size_t)fcnt * sizeof *flens);
    if (fsrcs == NULL || flens == NULL) {
      free(fsrcs); free(flens); free(fix_idxs); free(rb.buf);
      return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
    }
    for (uint32_t i = 0; i < fcnt; i++) {
      fsrcs[i] = ctx->fixes[fix_idxs[i]].fixed_src;
      flens[i] = ctx->fixes[fix_idxs[i]].fixed_len;
    }

    StigiDoorResult r;
    stigi_door_result_init(&r);
    Status st = stigi_door_submit(ctx->door, fsrcs, flens, fcnt, &r, diag);
    free(fsrcs);
    free(flens);
    if (st != STIGI_OK) {
      free(fix_idxs);
      stigi_door_result_free(&r);
      free(rb.buf);
      return st;
    }
    if (r.accepted) {
      for (uint32_t i = 0; i < fcnt; i++) {
        rb_fmt(&rb, "admitted %s   laws ok\n", ctx->fixes[fix_idxs[i]].unit_name);
      }
      clear_fixes(ctx);
    } else {
      status_tag = "rejected";
      for (uint32_t i = 0; i < r.reject_count; i++) {
        rb_fmt(&rb, "rejected: %s at %s: %s\n",
               r.rejects[i].code, r.rejects[i].subject, r.rejects[i].msg);
      }
    }
    free(fix_idxs);
    Status fin = finalize_reply(ctx, &rb, status_tag, &r, out_reply, out_len, diag);
    stigi_door_result_free(&r);
    return fin;
  }

  if (cmd_len == 5 && memcmp(frame, "query", 5) == 0) {
    Status qst = handle_query_frame(ctx, arg, arg_len, rest, rest_len, &rb, &status_tag, diag);
    if (qst != STIGI_OK) {
      free(rb.buf);
      return qst;
    }
    return finalize_reply(ctx, &rb, status_tag, NULL, out_reply, out_len, diag);
  }

  rb_fmt(&rb, "error unknown frame '%.*s'\n", (int)cmd_len, frame);
  return finalize_reply(ctx, &rb, "error", NULL, out_reply, out_len, diag);
}

/* ---- Claude Code --cc-socks envelope unwrapper (frames.md 7.2) ---- */

static const char *rfind_bytes(const char *hay, size_t hlen, const char *needle, size_t nlen) {
  if (nlen == 0 || nlen > hlen) return NULL;
  for (size_t i = hlen - nlen + 1U; i > 0; i--) {
    if (memcmp(hay + (i - 1U), needle, nlen) == 0) return hay + (i - 1U);
  }
  return NULL;
}

Status stigi_cc_socks_handle_envelope(StigiServerCtx *ctx, const char *envelope_json, size_t len,
                                      char **out_reply, size_t *out_len, Diag *diag) {
  if (ctx == NULL || envelope_json == NULL || out_reply == NULL) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_cc_socks_handle_envelope: invalid args");
  }
  if (stigi_json_validate_object(envelope_json, len) != JSON_OK) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "malformed JSON in cc-socks envelope");
  }
  char *body = NULL;
  size_t blen = 0;
  JsonStatus bs = stigi_json_obj_get_string(envelope_json, len, "body", &body, &blen);
  if (bs == JSON_NOT_FOUND) {
    bs = stigi_json_obj_get_string(envelope_json, len, "content", &body, &blen);
  }
  if (bs != JSON_OK) {
    return diag_set(diag, bs == JSON_ERR_OOM ? STIGI_ERR_OOM : STIGI_ERR_INVALID, -1,
                    "missing or invalid body/content in cc-socks envelope");
  }

  const char *frame_ptr = body;
  size_t frame_len = blen;
  const char *open_tag = memmem_bounded(body, blen, "<cross-session-message", 22U);
  if (open_tag != NULL) {
    const char *gt = memchr(open_tag, '>', (size_t)((body + blen) - open_tag));
    if (gt != NULL) {
      const char *after = gt + 1;
      size_t rem = (size_t)((body + blen) - after);
      const char *close_tag = rfind_bytes(after, rem, "</cross-session-message>", 24U);
      if (close_tag != NULL && close_tag >= after) {
        frame_ptr = after;
        frame_len = (size_t)(close_tag - after);
      }
    }
  }
  Status st = stigi_handle_frame(ctx, frame_ptr, frame_len, out_reply, out_len, diag);
  free(body);
  return st;
}

/* ---- Core Unix socket server (mode 0600, SO_PEERCRED same-UID) ---- */

static Status read_all_fd(int fd, char **out_buf, size_t *out_len, Diag *diag) {
  size_t cap = 4096U, len = 0;
  char *buf = malloc(cap);
  if (buf == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
  for (;;) {
    if (len >= STIGI_MAX_FRAME_BYTES) {
      char extra;
      ssize_t probe;
      do {
        probe = read(fd, &extra, 1U);
      } while (probe < 0 && errno == EINTR);
      if (probe > 0) {
        free(buf);
        return diag_set(diag, STIGI_ERR_OVERFLOW, -1, "frame exceeds max bytes");
      }
      break;
    }
    size_t want = STIGI_MAX_FRAME_BYTES - len;
    if (want > 4096U) want = 4096U;
    if (len + want + 1U > cap) {
      size_t ncap = cap;
      while (ncap < len + want + 1U) {
        if (ncap > SIZE_MAX / 2U) {
          free(buf);
          return diag_set(diag, STIGI_ERR_OVERFLOW, -1, "buffer overflow");
        }
        ncap *= 2U;
      }
      char *nb = realloc(buf, ncap);
      if (nb == NULL) { free(buf); return diag_set(diag, STIGI_ERR_OOM, -1, "OOM"); }
      buf = nb;
      cap = ncap;
    }
    ssize_t n = read(fd, buf + len, want);
    if (n < 0) {
      if (errno == EINTR) continue;
      free(buf);
      return diag_set(diag, STIGI_ERR_IO, errno, "socket read failed");
    }
    if (n == 0) break;
    len += (size_t)n;
  }
  buf[len] = '\0';
  *out_buf = buf;
  *out_len = len;
  return STIGI_OK;
}

/* Read one newline-delimited JSON line (no EOF deadlock for --cc-socks) */
static Status read_line_fd(int fd, char **out_buf, size_t *out_len, Diag *diag) {
  size_t cap = 4096U, len = 0;
  char *buf = malloc(cap);
  if (buf == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
  for (;;) {
    char ch = 0;
    ssize_t n = read(fd, &ch, 1U);
    if (n < 0) {
      if (errno == EINTR) continue;
      free(buf);
      return diag_set(diag, STIGI_ERR_IO, errno, "socket read failed");
    }
    if (n == 0) break;
    if (len >= STIGI_MAX_FRAME_BYTES) {
      free(buf);
      return diag_set(diag, STIGI_ERR_OVERFLOW, -1, "cc-socks line exceeds max bytes");
    }
    if (len + 2U > cap) {
      if (cap > SIZE_MAX / 2U) {
        free(buf);
        return diag_set(diag, STIGI_ERR_OVERFLOW, -1, "buffer overflow");
      }
      cap *= 2U;
      char *nb = realloc(buf, cap);
      if (nb == NULL) { free(buf); return diag_set(diag, STIGI_ERR_OOM, -1, "OOM"); }
      buf = nb;
    }
    buf[len++] = ch;
    if (ch == '\n') break;
  }
  buf[len] = '\0';
  *out_buf = buf;
  *out_len = len;
  return STIGI_OK;
}

static Status write_all_fd(int fd, const char *buf, size_t len, Diag *diag) {
  size_t off = 0;
  while (off < len) {
    ssize_t n = send(fd, buf + off, len - off, MSG_NOSIGNAL);
    if (n < 0) {
      if (errno == EINTR) continue;
      return diag_set(diag, STIGI_ERR_IO, errno, "socket send failed");
    }
    off += (size_t)n;
  }
  return STIGI_OK;
}

static Status serve_unix_loop(StigiServerCtx *ctx, const char *sock_path,
                              bool cc_socks_mode, uint32_t max_requests, Diag *diag) {
  if (ctx == NULL || sock_path == NULL) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "serve_unix_loop: invalid args");
  }
  (void)unlink(sock_path);

  int sfd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sfd < 0) return diag_set(diag, STIGI_ERR_IO, errno, "socket() failed");

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof addr);
  addr.sun_family = AF_UNIX;
  if (strlen(sock_path) >= sizeof addr.sun_path) {
    (void)close(sfd);
    return diag_set(diag, STIGI_ERR_INVALID, -1, "socket path too long");
  }
  (void)snprintf(addr.sun_path, sizeof addr.sun_path, "%s", sock_path);

  /* Eliminate TOCTOU permission race before chmod(0600) */
  mode_t old_um = umask(0077);
  int brc = bind(sfd, (struct sockaddr *)&addr, sizeof addr);
  int be = errno;
  (void)umask(old_um);
  if (brc != 0) {
    (void)close(sfd);
    return diag_set(diag, STIGI_ERR_IO, be, "bind(%s) failed", sock_path);
  }
  if (chmod(sock_path, 0600) != 0) {
    int e = errno;
    (void)close(sfd);
    (void)unlink(sock_path);
    return diag_set(diag, STIGI_ERR_IO, e, "chmod(0600) failed");
  }
  if (listen(sfd, 16) != 0) {
    int e = errno;
    (void)close(sfd);
    (void)unlink(sock_path);
    return diag_set(diag, STIGI_ERR_IO, e, "listen() failed");
  }

  uid_t my_uid = geteuid();
  uint32_t served = 0;
  Status st = STIGI_OK;

  while (!ctx->stopped && (max_requests == 0 || served < max_requests)) {
    int cfd = accept(sfd, NULL, NULL);
    if (cfd < 0) {
      if (errno == EINTR) continue;
      st = diag_set(diag, STIGI_ERR_IO, errno, "accept() failed");
      break;
    }

    /* S115: enforce same-UID SO_PEERCRED */
    struct ucred cred;
    socklen_t clen = sizeof cred;
    if (getsockopt(cfd, SOL_SOCKET, SO_PEERCRED, &cred, &clen) != 0 || cred.uid != my_uid) {
      (void)close(cfd);
      continue;
    }

    char *req = NULL;
    size_t rlen = 0;
    Status rst = cc_socks_mode ? read_line_fd(cfd, &req, &rlen, diag)
                               : read_all_fd(cfd, &req, &rlen, diag);
    if (rst == STIGI_OK && rlen > 0) {
      char *rep = NULL;
      size_t rep_len = 0;
      Status hst = cc_socks_mode
                       ? stigi_cc_socks_handle_envelope(ctx, req, rlen, &rep, &rep_len, diag)
                       : stigi_handle_frame(ctx, req, rlen, &rep, &rep_len, diag);
      if (hst == STIGI_OK && rep != NULL) {
        (void)write_all_fd(cfd, rep, rep_len, diag);
      } else {
        const char *emsg = "error frame processing failed\n";
        (void)write_all_fd(cfd, emsg, strlen(emsg), diag);
      }
      free(rep);
    } else if (rst != STIGI_OK) {
      char ebuf[256];
      int en = snprintf(ebuf, sizeof ebuf, "error %s\n",
                        (diag != NULL && diag->msg[0] != '\0') ? diag->msg : "frame read error");
      if (en > 0) {
        size_t el = (size_t)en < sizeof ebuf ? (size_t)en : sizeof ebuf - 1U;
        (void)write_all_fd(cfd, ebuf, el, diag);
      }
    }
    free(req);
    (void)close(cfd);
    served++;
  }

  (void)close(sfd);
  (void)unlink(sock_path);
  return st;
}

Status stigi_serve_socket(StigiServerCtx *ctx, const char *sock_path,
                          uint32_t max_requests, Diag *diag) {
  return serve_unix_loop(ctx, sock_path, false, max_requests, diag);
}

Status stigi_serve_cc_socks(StigiServerCtx *ctx, const char *sock_path,
                            uint32_t max_requests, Diag *diag) {
  return serve_unix_loop(ctx, sock_path, true, max_requests, diag);
}

Status stigi_send_socket_frame(const char *sock_path, const char *frame, size_t len,
                               char **out_reply, size_t *out_len, Diag *diag) {
  if (sock_path == NULL || frame == NULL || out_reply == NULL) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_send_socket_frame: invalid args");
  }
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) return diag_set(diag, STIGI_ERR_IO, errno, "socket() failed");

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof addr);
  addr.sun_family = AF_UNIX;
  if (strlen(sock_path) >= sizeof addr.sun_path) {
    (void)close(fd);
    return diag_set(diag, STIGI_ERR_INVALID, -1, "socket path too long");
  }
  (void)snprintf(addr.sun_path, sizeof addr.sun_path, "%s", sock_path);

  if (connect(fd, (struct sockaddr *)&addr, sizeof addr) != 0) {
    int e = errno;
    (void)close(fd);
    return diag_set(diag, STIGI_ERR_IO, e, "connect(%s) failed", sock_path);
  }
  Status st = write_all_fd(fd, frame, len, diag);
  if (st == STIGI_OK) {
    (void)shutdown(fd, SHUT_WR);
    st = read_all_fd(fd, out_reply, out_len, diag);
  }
  (void)close(fd);
  return st;
}
