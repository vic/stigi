#include "mcp.h"
#include "json_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char *buf;
  size_t len;
  size_t cap;
  bool oom;
} JsonBuf;

static void jb_grow(JsonBuf *b, size_t add) {
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

static void jb_mem(JsonBuf *b, const char *s, size_t n) {
  if (n == 0 || b->oom) return;
  jb_grow(b, n);
  if (b->oom) return;
  memcpy(b->buf + b->len, s, n);
  b->len += n;
  b->buf[b->len] = '\0';
}

static void jb_cstr(JsonBuf *b, const char *s) {
  if (s != NULL) jb_mem(b, s, strlen(s));
}

static void jb_escaped(JsonBuf *b, const char *s, size_t len) {
  jb_mem(b, "\"", 1);
  for (size_t i = 0; i < len; i++) {
    unsigned char c = (unsigned char)s[i];
    if (c == '"') jb_cstr(b, "\\\"");
    else if (c == '\\') jb_cstr(b, "\\\\");
    else if (c == '\b') jb_cstr(b, "\\b");
    else if (c == '\f') jb_cstr(b, "\\f");
    else if (c == '\n') jb_cstr(b, "\\n");
    else if (c == '\r') jb_cstr(b, "\\r");
    else if (c == '\t') jb_cstr(b, "\\t");
    else if (c < 0x20U) {
      char u[8];
      int n = snprintf(u, sizeof u, "\\u%04x", (unsigned)c);
      if (n > 0) jb_mem(b, u, (size_t)n);
    } else {
      jb_mem(b, &s[i], 1);
    }
  }
  jb_mem(b, "\"", 1);
}

static Status finalize_jsonbuf(JsonBuf *jb, char **out_json, size_t *out_len, Diag *diag) {
  if (jb->oom || jb->buf == NULL) {
    free(jb->buf);
    return diag_set(diag, STIGI_ERR_OOM, -1, "mcp OOM");
  }
  *out_json = jb->buf;
  if (out_len != NULL) *out_len = jb->len;
  return STIGI_OK;
}

static Status make_error_reply(const JsonSpan *id_span, int code, const char *msg,
                               char **out_json, size_t *out_len, Diag *diag) {
  JsonBuf eb = {NULL, 0, 0, false};
  jb_cstr(&eb, "{\"jsonrpc\":\"2.0\",\"id\":");
  if (id_span != NULL && id_span->len > 0) jb_mem(&eb, id_span->ptr, id_span->len);
  else jb_cstr(&eb, "null");
  char cbuf[64];
  int cn = snprintf(cbuf, sizeof cbuf, ",\"error\":{\"code\":%d,\"message\":", code);
  if (cn > 0) jb_mem(&eb, cbuf, (size_t)cn);
  jb_escaped(&eb, msg, strlen(msg));
  jb_cstr(&eb, "}}\n");
  return finalize_jsonbuf(&eb, out_json, out_len, diag);
}

static int cmp_unit_name_mcp(const void *pa, const void *pb) {
  const StigiImageUnit *a = *(const StigiImageUnit *const *)pa;
  const StigiImageUnit *b = *(const StigiImageUnit *const *)pb;
  return strcmp(a->name, b->name);
}

static Status build_resource_frame(const char *uri, size_t ulen, char **out_frame, size_t *out_len) {
  JsonBuf fb = {NULL, 0, 0, false};
  if (ulen >= 16U && memcmp(uri, "stigi://summary/", 16) == 0) {
    if (ulen > 16U) {
      jb_cstr(&fb, "summary ");
      jb_mem(&fb, uri + 16, ulen - 16U);
    } else {
      jb_cstr(&fb, "summary");
    }
  } else if (ulen >= 13U && memcmp(uri, "stigi://laws/", 13) == 0) {
    if (ulen > 13U) {
      jb_cstr(&fb, "laws ");
      jb_mem(&fb, uri + 13, ulen - 13U);
    } else {
      jb_cstr(&fb, "laws");
    }
  } else if (ulen > 13U && memcmp(uri, "stigi://unit/", 13) == 0) {
    jb_cstr(&fb, "show ");
    jb_mem(&fb, uri + 13, ulen - 13U);
  } else if (ulen == 13U && memcmp(uri, "stigi://tests", 13) == 0) {
    jb_cstr(&fb, "tests");
  } else {
    return STIGI_ERR_INVALID;
  }
  if (fb.oom || fb.buf == NULL) {
    free(fb.buf);
    return STIGI_ERR_OOM;
  }
  *out_frame = fb.buf;
  *out_len = fb.len;
  return STIGI_OK;
}

static Status handle_mcp_internal(StigiServerCtx *ctx, const char *bridge_sock,
                                  const char *req_json, size_t req_len,
                                  char **out_json, size_t *out_len, Diag *diag) {
  if ((ctx == NULL && bridge_sock == NULL) || req_json == NULL || out_json == NULL) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_mcp_handle_jsonrpc: invalid args");
  }
  *out_json = NULL;
  if (out_len != NULL) *out_len = 0;

  /* Strictly validate JSON object syntax (fail closed -32700 on malformed/unterminated JSON) */
  if (stigi_json_validate_object(req_json, req_len) != JSON_OK) {
    return make_error_reply(NULL, -32700, "parse error", out_json, out_len, diag);
  }

  /* Detect presence of top-level "id" (Defect #1: notifications have no "id" and get NO reply) */
  JsonSpan id_span = {NULL, 0};
  JsonStatus id_st = stigi_json_obj_get_raw(req_json, req_len, "id", &id_span);
  if (id_st == JSON_ERR_SYNTAX) {
    return make_error_reply(NULL, -32700, "parse error", out_json, out_len, diag);
  }
  if (id_st == JSON_ERR_OOM) {
    return diag_set(diag, STIGI_ERR_OOM, -1, "mcp OOM");
  }
  bool is_notification = (id_st == JSON_NOT_FOUND);
  if (!is_notification && id_span.len > 0 &&
      (id_span.ptr[0] == '{' || id_span.ptr[0] == '[' ||
       id_span.ptr[0] == 't' || id_span.ptr[0] == 'f')) {
    return make_error_reply(NULL, -32600, "invalid JSON-RPC id (must be string, number, or null)",
                            out_json, out_len, diag);
  }

  char *method = NULL;
  size_t mlen = 0;
  JsonStatus ms = stigi_json_obj_get_string(req_json, req_len, "method", &method, &mlen);
  if (ms == JSON_ERR_OOM) {
    return diag_set(diag, STIGI_ERR_OOM, -1, "mcp OOM");
  }
  if (ms != JSON_OK) {
    if (is_notification) return STIGI_OK;
    return make_error_reply(&id_span, -32600, "missing method", out_json, out_len, diag);
  }

  /* JSON-RPC 2.0 notifications MUST NOT produce a reply */
  if (is_notification) {
    free(method);
    *out_json = NULL;
    if (out_len != NULL) *out_len = 0;
    return STIGI_OK;
  }

  JsonBuf jb = {NULL, 0, 0, false};
  jb_cstr(&jb, "{\"jsonrpc\":\"2.0\",\"id\":");
  jb_mem(&jb, id_span.ptr, id_span.len);

  if (strcmp(method, "initialize") == 0) {
    jb_cstr(&jb,
            ",\"result\":{\"protocolVersion\":\"2025-03-26\","
            "\"capabilities\":{\"tools\":{},\"resources\":{}},"
            "\"serverInfo\":{\"name\":\"stigi-cc\",\"version\":\"0.1.0\"}}}\n");
  } else if (strcmp(method, "ping") == 0) {
    jb_cstr(&jb, ",\"result\":{}}\n");
  } else if (strcmp(method, "tools/list") == 0) {
    jb_cstr(&jb,
            ",\"result\":{\"tools\":[{\"name\":\"stigi_frame\","
            "\"description\":\"Send any stigi-cc frame verbatim and return the byte-stable reply.\","
            "\"inputSchema\":{\"type\":\"object\","
            "\"properties\":{\"frame\":{\"type\":\"string\"},\"json\":{\"type\":\"boolean\"}},"
            "\"required\":[\"frame\"]}}]}}\n");
  } else if (strcmp(method, "tools/call") == 0) {
    JsonSpan params = {NULL, 0};
    JsonSpan args = {NULL, 0};
    char *tname = NULL;
    size_t tnlen = 0;
    if (stigi_json_obj_get_object(req_json, req_len, "params", &params) != JSON_OK ||
        stigi_json_obj_get_string(params.ptr, params.len, "name", &tname, &tnlen) != JSON_OK) {
      free(tname);
      jb_cstr(&jb, ",\"error\":{\"code\":-32602,\"message\":\"missing params.name\"}}\n");
    } else if (strcmp(tname, "stigi_frame") != 0) {
      free(tname);
      jb_cstr(&jb, ",\"error\":{\"code\":-32602,\"message\":\"unknown tool name\"}}\n");
    } else {
      free(tname);
      char *frame = NULL;
      size_t flen = 0;
      if (stigi_json_obj_get_object(params.ptr, params.len, "arguments", &args) != JSON_OK ||
          stigi_json_obj_get_string(args.ptr, args.len, "frame", &frame, &flen) != JSON_OK) {
        free(frame);
        jb_cstr(&jb, ",\"error\":{\"code\":-32602,\"message\":\"missing arguments.frame\"}}\n");
      } else {
        bool json_param = false;
        bool has_json_param = (stigi_json_obj_get_bool(args.ptr, args.len, "json", &json_param) == JSON_OK);
        char *rep = NULL;
        size_t rlen = 0;
        Status st = STIGI_OK;
        if (bridge_sock != NULL) {
          if (has_json_param && json_param) {
            /* Wrap plain reply into JSON envelope without mutating shared server state */
            char *raw_rep = NULL;
            size_t raw_len = 0;
            st = stigi_send_socket_frame(bridge_sock, frame, flen, &raw_rep, &raw_len, diag);
            if (st == STIGI_OK && raw_rep != NULL) {
              if (raw_len > 0 && raw_rep[0] == '{') {
                rep = raw_rep;
                rlen = raw_len;
              } else {
                const char *stag = "accepted";
                if (raw_len >= 8 && memcmp(raw_rep, "rejected", 8) == 0) stag = "rejected";
                else if (raw_len >= 5 && memcmp(raw_rep, "error", 5) == 0) stag = "error";
                JsonBuf ejb = {NULL, 0, 0, false};
                jb_cstr(&ejb, "{\"status\":");
                jb_escaped(&ejb, stag, strlen(stag));
                jb_cstr(&ejb, ",\"reply\":");
                jb_escaped(&ejb, raw_rep, raw_len);
                jb_cstr(&ejb, "}\n");
                free(raw_rep);
                st = finalize_jsonbuf(&ejb, &rep, &rlen, diag);
              }
            } else {
              free(raw_rep);
            }
          } else {
            st = stigi_send_socket_frame(bridge_sock, frame, flen, &rep, &rlen, diag);
          }
        } else {
          bool saved_json = ctx->json_mode;
          if (has_json_param) ctx->json_mode = json_param;
          st = stigi_handle_frame(ctx, frame, flen, &rep, &rlen, diag);
          if (has_json_param) ctx->json_mode = saved_json;
        }
        free(frame);
        if (st != STIGI_OK || rep == NULL) {
          free(rep);
          jb_cstr(&jb, ",\"error\":{\"code\":-32603,\"message\":\"frame execution failed\"}}\n");
        } else {
          jb_cstr(&jb, ",\"result\":{\"content\":[{\"type\":\"text\",\"text\":");
          jb_escaped(&jb, rep, rlen);
          jb_cstr(&jb, "}]}}\n");
          free(rep);
        }
      }
    }
  } else if (strcmp(method, "resources/list") == 0) {
    jb_cstr(&jb,
            ",\"result\":{\"resources\":["
            "{\"uri\":\"stigi://summary/\",\"name\":\"summary\",\"mimeType\":\"text/plain\"},"
            "{\"uri\":\"stigi://laws/\",\"name\":\"laws\",\"mimeType\":\"text/plain\"},"
            "{\"uri\":\"stigi://tests\",\"name\":\"tests\",\"mimeType\":\"text/plain\"}");
    if (bridge_sock != NULL) {
      char *lrep = NULL;
      size_t lcnt = 0;
      if (stigi_send_socket_frame(bridge_sock, "list\n", 5, &lrep, &lcnt, diag) == STIGI_OK && lrep != NULL) {
        const char *p = lrep;
        const char *end = lrep + lcnt;
        while (p < end) {
          const char *nl = memchr(p, '\n', (size_t)(end - p));
          const char *lend = nl != NULL ? nl : end;
          const char *sp = memchr(p, ' ', (size_t)(lend - p));
          if (sp != NULL && sp > p) {
            size_t nlen = (size_t)(sp - p);
            char *uri_buf = malloc(13U + nlen + 1U);
            if (uri_buf != NULL) {
              memcpy(uri_buf, "stigi://unit/", 13U);
              memcpy(uri_buf + 13U, p, nlen);
              uri_buf[13U + nlen] = '\0';
              jb_cstr(&jb, ",{\"uri\":");
              jb_escaped(&jb, uri_buf, 13U + nlen);
              jb_cstr(&jb, ",\"name\":");
              jb_escaped(&jb, p, nlen);
              jb_cstr(&jb, ",\"mimeType\":\"text/plain\"}");
              free(uri_buf);
            }
          }
          if (nl == NULL) break;
          p = nl + 1;
        }
      }
      free(lrep);
    } else if (ctx->door != NULL && ctx->door->image.unit_count > 0) {
      const StigiImage *img = &ctx->door->image;
      const StigiImageUnit **sorted = malloc((size_t)img->unit_count * sizeof *sorted);
      if (sorted == NULL) {
        free(method);
        free(jb.buf);
        return diag_set(diag, STIGI_ERR_OOM, -1, "mcp OOM");
      }
      for (uint32_t i = 0; i < img->unit_count; i++) sorted[i] = &img->units[i];
      qsort(sorted, img->unit_count, sizeof *sorted, cmp_unit_name_mcp);
      for (uint32_t i = 0; i < img->unit_count; i++) {
        char *uri_buf = malloc(13U + sorted[i]->name_len + 1U);
        if (uri_buf != NULL) {
          memcpy(uri_buf, "stigi://unit/", 13U);
          memcpy(uri_buf + 13U, sorted[i]->name, sorted[i]->name_len);
          uri_buf[13U + sorted[i]->name_len] = '\0';
          jb_cstr(&jb, ",{\"uri\":");
          jb_escaped(&jb, uri_buf, 13U + sorted[i]->name_len);
          jb_cstr(&jb, ",\"name\":");
          jb_escaped(&jb, sorted[i]->name, sorted[i]->name_len);
          jb_cstr(&jb, ",\"mimeType\":\"text/plain\"}");
          free(uri_buf);
        }
      }
      free(sorted);
    }
    jb_cstr(&jb, "]}}\n");
  } else if (strcmp(method, "resources/read") == 0) {
    JsonSpan params = {NULL, 0};
    char *uri = NULL;
    size_t ulen = 0;
    if (stigi_json_obj_get_object(req_json, req_len, "params", &params) != JSON_OK ||
        stigi_json_obj_get_string(params.ptr, params.len, "uri", &uri, &ulen) != JSON_OK) {
      free(uri);
      jb_cstr(&jb, ",\"error\":{\"code\":-32602,\"message\":\"missing params.uri\"}}\n");
    } else {
      char *fbuf = NULL;
      size_t flen = 0;
      Status bst = build_resource_frame(uri, ulen, &fbuf, &flen);
      if (bst == STIGI_ERR_OOM) {
        free(uri);
        free(method);
        free(jb.buf);
        return diag_set(diag, STIGI_ERR_OOM, -1, "mcp OOM");
      }
      if (bst != STIGI_OK) {
        jb_cstr(&jb, ",\"error\":{\"code\":-32602,\"message\":\"unknown resource uri\"}}\n");
      } else {
        char *rep = NULL;
        size_t rlen = 0;
        Status st = STIGI_OK;
        if (bridge_sock != NULL) {
          st = stigi_send_socket_frame(bridge_sock, fbuf, flen, &rep, &rlen, diag);
        } else {
          /* Force plain text (json_mode = false) for resources/read */
          bool saved_json = ctx->json_mode;
          ctx->json_mode = false;
          st = stigi_handle_frame(ctx, fbuf, flen, &rep, &rlen, diag);
          ctx->json_mode = saved_json;
        }
        free(fbuf);
        if (st != STIGI_OK || rep == NULL) {
          free(rep);
          jb_cstr(&jb, ",\"error\":{\"code\":-32603,\"message\":\"resource read failed\"}}\n");
        } else {
          jb_cstr(&jb, ",\"result\":{\"contents\":[{\"uri\":");
          jb_escaped(&jb, uri, ulen);
          jb_cstr(&jb, ",\"mimeType\":\"text/plain\",\"text\":");
          jb_escaped(&jb, rep, rlen);
          jb_cstr(&jb, "}]}}\n");
          free(rep);
        }
      }
      free(uri);
    }
  } else {
    jb_cstr(&jb, ",\"error\":{\"code\":-32601,\"message\":\"method not found\"}}\n");
  }

  free(method);
  return finalize_jsonbuf(&jb, out_json, out_len, diag);
}

Status stigi_mcp_handle_jsonrpc(StigiServerCtx *ctx, const char *req_json, size_t req_len,
                                char **out_json, size_t *out_len, Diag *diag) {
  return handle_mcp_internal(ctx, NULL, req_json, req_len, out_json, out_len, diag);
}

static bool is_blank_line(const char *s, size_t len) {
  for (size_t i = 0; i < len; i++) {
    if (s[i] != ' ' && s[i] != '\t' && s[i] != '\r' && s[i] != '\n') return false;
  }
  return true;
}

static Status read_stdin_line(char **buf_p, size_t *cap_p, size_t *out_len, bool *out_eof,
                              Diag *diag) {
  size_t len = 0;
  *out_eof = false;
  if (*buf_p == NULL) {
    *cap_p = 4096U;
    *buf_p = malloc(*cap_p);
    if (*buf_p == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
  }
  for (;;) {
    int ch = fgetc(stdin);
    if (ch == EOF) {
      if (ferror(stdin)) return diag_set(diag, STIGI_ERR_IO, -1, "stdin read error");
      if (len == 0) {
        *out_eof = true;
        *out_len = 0;
        return STIGI_OK;
      }
      break;
    }
    if (len >= STIGI_MAX_FRAME_BYTES) {
      /* Drain rest of overlong line */
      while (ch != '\n' && ch != EOF) ch = fgetc(stdin);
      return diag_set(diag, STIGI_ERR_OVERFLOW, -1, "MCP line exceeds STIGI_MAX_FRAME_BYTES");
    }
    if (len + 2U > *cap_p) {
      if (*cap_p > SIZE_MAX / 2U) return diag_set(diag, STIGI_ERR_OVERFLOW, -1, "line buffer overflow");
      size_t ncap = *cap_p * 2U;
      char *nb = realloc(*buf_p, ncap);
      if (nb == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
      *buf_p = nb;
      *cap_p = ncap;
    }
    (*buf_p)[len++] = (char)ch;
    if (ch == '\n') break;
  }
  (*buf_p)[len] = '\0';
  *out_len = len;
  return STIGI_OK;
}

static Status serve_mcp_loop(StigiServerCtx *ctx, const char *bridge_sock, Diag *diag) {
  char *line = NULL;
  size_t cap = 0;
  Status st = STIGI_OK;

  while (ctx == NULL || !ctx->stopped) {
    size_t len = 0;
    bool eof = false;
    Status rst = read_stdin_line(&line, &cap, &len, &eof, diag);
    if (rst == STIGI_ERR_OVERFLOW) {
      char *err_rep = NULL;
      size_t err_len = 0;
      if (make_error_reply(NULL, -32700, "request exceeds max frame size",
                           &err_rep, &err_len, diag) == STIGI_OK && err_rep != NULL) {
        if (fwrite(err_rep, 1, err_len, stdout) != err_len || fflush(stdout) != 0) {
          free(err_rep);
          st = diag_set(diag, STIGI_ERR_IO, -1, "stdout write failed");
          break;
        }
        free(err_rep);
      }
      continue;
    }
    if (rst != STIGI_OK) { st = rst; break; }
    if (eof) break;
    if (len == 0 || is_blank_line(line, len)) continue;

    char *resp = NULL;
    size_t rlen = 0;
    st = handle_mcp_internal(ctx, bridge_sock, line, len, &resp, &rlen, diag);
    if (st != STIGI_OK) break;
    if (resp != NULL && rlen > 0) {
      if (fwrite(resp, 1, rlen, stdout) != rlen || fflush(stdout) != 0) {
        free(resp);
        st = diag_set(diag, STIGI_ERR_IO, -1, "stdout write failed");
        break;
      }
    }
    free(resp);
  }

  free(line);
  return st;
}

Status stigi_serve_mcp_stdio(StigiServerCtx *ctx, Diag *diag) {
  if (ctx == NULL) return diag_set(diag, STIGI_ERR_INVALID, -1, "invalid ctx");
  return serve_mcp_loop(ctx, NULL, diag);
}

Status stigi_serve_mcp_stdio_bridge(const char *sock_path, Diag *diag) {
  if (sock_path == NULL || sock_path[0] == '\0') {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "invalid socket path");
  }
  return serve_mcp_loop(NULL, sock_path, diag);
}
