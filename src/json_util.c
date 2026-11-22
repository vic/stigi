#include "json_util.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define JSON_MAX_DEPTH 64

static void skip_ws(const char **pp, const char *end) {
  const char *p = *pp;
  while (p < end && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) p++;
  *pp = p;
}

static int hex_val(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
  if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
  return -1;
}

static bool parse_hex4(const char *p, const char *end, uint32_t *out) {
  if ((size_t)(end - p) < 4U) return false;
  uint32_t v = 0;
  for (int i = 0; i < 4; i++) {
    int h = hex_val(p[i]);
    if (h < 0) return false;
    v = (v << 4) | (uint32_t)h;
  }
  *out = v;
  return true;
}

static bool skip_string(const char **pp, const char *end) {
  const char *p = *pp;
  if (p >= end || *p != '"') return false;
  p++;
  while (p < end) {
    unsigned char c = (unsigned char)*p;
    if (c == '"') {
      *pp = p + 1;
      return true;
    }
    if (c < 0x20U) return false; /* unescaped control character */
    if (c == '\\') {
      p++;
      if (p >= end) return false;
      char esc = *p++;
      switch (esc) {
        case '"':
        case '\\':
        case '/':
        case 'b':
        case 'f':
        case 'n':
        case 'r':
        case 't':
          break;
        case 'u': {
          uint32_t cp = 0;
          if (!parse_hex4(p, end, &cp)) return false;
          p += 4;
          break;
        }
        default:
          return false;
      }
    } else {
      p++;
    }
  }
  return false; /* unterminated string */
}

static bool skip_number(const char **pp, const char *end) {
  const char *p = *pp;
  if (p < end && *p == '-') p++;
  if (p >= end) return false;
  if (*p == '0') {
    p++;
  } else if (*p >= '1' && *p <= '9') {
    while (p < end && *p >= '0' && *p <= '9') p++;
  } else {
    return false;
  }
  if (p < end && *p == '.') {
    p++;
    if (p >= end || *p < '0' || *p > '9') return false;
    while (p < end && *p >= '0' && *p <= '9') p++;
  }
  if (p < end && (*p == 'e' || *p == 'E')) {
    p++;
    if (p < end && (*p == '+' || *p == '-')) p++;
    if (p >= end || *p < '0' || *p > '9') return false;
    while (p < end && *p >= '0' && *p <= '9') p++;
  }
  *pp = p;
  return true;
}

static bool skip_value(const char **pp, const char *end, int depth) {
  if (depth > JSON_MAX_DEPTH) return false;
  skip_ws(pp, end);
  const char *p = *pp;
  if (p >= end) return false;

  if (*p == '"') {
    return skip_string(pp, end);
  }
  if (*p == '{') {
    p++;
    skip_ws(&p, end);
    if (p < end && *p == '}') {
      *pp = p + 1;
      return true;
    }
    for (;;) {
      skip_ws(&p, end);
      if (!skip_string(&p, end)) return false;
      skip_ws(&p, end);
      if (p >= end || *p != ':') return false;
      p++;
      if (!skip_value(&p, end, depth + 1)) return false;
      skip_ws(&p, end);
      if (p >= end) return false;
      if (*p == '}') {
        *pp = p + 1;
        return true;
      }
      if (*p != ',') return false;
      p++;
    }
  }
  if (*p == '[') {
    p++;
    skip_ws(&p, end);
    if (p < end && *p == ']') {
      *pp = p + 1;
      return true;
    }
    for (;;) {
      if (!skip_value(&p, end, depth + 1)) return false;
      skip_ws(&p, end);
      if (p >= end) return false;
      if (*p == ']') {
        *pp = p + 1;
        return true;
      }
      if (*p != ',') return false;
      p++;
    }
  }
  if ((size_t)(end - p) >= 4U && memcmp(p, "true", 4) == 0) {
    *pp = p + 4;
    return true;
  }
  if ((size_t)(end - p) >= 5U && memcmp(p, "false", 5) == 0) {
    *pp = p + 5;
    return true;
  }
  if ((size_t)(end - p) >= 4U && memcmp(p, "null", 4) == 0) {
    *pp = p + 4;
    return true;
  }
  return skip_number(pp, end);
}

JsonStatus stigi_json_validate_object(const char *buf, size_t len) {
  if (buf == NULL) return JSON_ERR_SYNTAX;
  const char *p = buf;
  const char *end = buf + len;
  skip_ws(&p, end);
  if (p >= end || *p != '{') return JSON_ERR_SYNTAX;
  if (!skip_value(&p, end, 0)) return JSON_ERR_SYNTAX;
  skip_ws(&p, end);
  if (p != end) return JSON_ERR_SYNTAX;
  return JSON_OK;
}

static bool buf_append(char **buf, size_t *len, size_t *cap, const char *s, size_t n) {
  if (n == 0) return true;
  if (n > SIZE_MAX - *len - 1U) return false;
  if (*len + n + 1U > *cap) {
    size_t ncap = *cap == 0 ? 64U : *cap;
    while (ncap < *len + n + 1U) {
      if (ncap > SIZE_MAX / 2U) return false;
      ncap *= 2U;
    }
    char *nb = realloc(*buf, ncap);
    if (nb == NULL) return false;
    *buf = nb;
    *cap = ncap;
  }
  memcpy(*buf + *len, s, n);
  *len += n;
  (*buf)[*len] = '\0';
  return true;
}

static bool append_utf8(char **buf, size_t *len, size_t *cap, uint32_t cp) {
  char u[4];
  size_t n = 0;
  if (cp <= 0x7FU) {
    u[0] = (char)cp;
    n = 1;
  } else if (cp <= 0x7FFU) {
    u[0] = (char)(0xC0U | (cp >> 6));
    u[1] = (char)(0x80U | (cp & 0x3FU));
    n = 2;
  } else if (cp <= 0xFFFFU) {
    u[0] = (char)(0xE0U | (cp >> 12));
    u[1] = (char)(0x80U | ((cp >> 6) & 0x3FU));
    u[2] = (char)(0x80U | (cp & 0x3FU));
    n = 3;
  } else if (cp <= 0x10FFFFU) {
    u[0] = (char)(0xF0U | (cp >> 18));
    u[1] = (char)(0x80U | ((cp >> 12) & 0x3FU));
    u[2] = (char)(0x80U | ((cp >> 6) & 0x3FU));
    u[3] = (char)(0x80U | (cp & 0x3FU));
    n = 4;
  } else {
    return false;
  }
  return buf_append(buf, len, cap, u, n);
}

JsonStatus stigi_json_decode_string(const char *raw, size_t raw_len,
                                    char **out_str, size_t *out_len) {
  if (raw == NULL || raw_len < 2U || raw[0] != '"' || raw[raw_len - 1U] != '"' || out_str == NULL) {
    return JSON_ERR_SYNTAX;
  }
  const char *p = raw + 1;
  const char *end = raw + raw_len - 1U;
  char *buf = NULL;
  size_t len = 0, cap = 0;

  while (p < end) {
    unsigned char c = (unsigned char)*p;
    if (c < 0x20U || c == '"') {
      free(buf);
      return JSON_ERR_SYNTAX;
    }
    if (c == '\\') {
      p++;
      if (p >= end) {
        free(buf);
        return JSON_ERR_SYNTAX;
      }
      char esc = *p++;
      char out_ch = 0;
      bool simple = true;
      switch (esc) {
        case '"': out_ch = '"'; break;
        case '\\': out_ch = '\\'; break;
        case '/': out_ch = '/'; break;
        case 'b': out_ch = '\b'; break;
        case 'f': out_ch = '\f'; break;
        case 'n': out_ch = '\n'; break;
        case 'r': out_ch = '\r'; break;
        case 't': out_ch = '\t'; break;
        case 'u': {
          simple = false;
          uint32_t cp = 0;
          if (!parse_hex4(p, end, &cp)) {
            free(buf);
            return JSON_ERR_SYNTAX;
          }
          p += 4;
          /* Handle UTF-16 surrogate pair */
          if (cp >= 0xD800U && cp <= 0xDBFFU) {
            if ((size_t)(end - p) >= 6U && p[0] == '\\' && p[1] == 'u') {
              uint32_t lo = 0;
              if (parse_hex4(p + 2, end, &lo) && lo >= 0xDC00U && lo <= 0xDFFFU) {
                cp = 0x10000U + (((cp - 0xD800U) << 10) | (lo - 0xDC00U));
                p += 6;
              }
            }
          }
          if (cp == 0U) {
            free(buf);
            return JSON_ERR_SYNTAX; /* reject embedded NUL \u0000 */
          }
          if (!append_utf8(&buf, &len, &cap, cp)) {
            free(buf);
            return JSON_ERR_OOM;
          }
          break;
        }
        default:
          free(buf);
          return JSON_ERR_SYNTAX;
      }
      if (simple) {
        if (!buf_append(&buf, &len, &cap, &out_ch, 1U)) {
          free(buf);
          return JSON_ERR_OOM;
        }
      }
    } else {
      if (!buf_append(&buf, &len, &cap, p, 1U)) {
        free(buf);
        return JSON_ERR_OOM;
      }
      p++;
    }
  }

  if (buf == NULL) {
    buf = malloc(1U);
    if (buf == NULL) return JSON_ERR_OOM;
    buf[0] = '\0';
  }
  *out_str = buf;
  if (out_len != NULL) *out_len = len;
  return JSON_OK;
}

JsonStatus stigi_json_obj_get_raw(const char *obj, size_t obj_len, const char *key,
                                  JsonSpan *out_val) {
  if (obj == NULL || key == NULL || out_val == NULL) return JSON_ERR_SYNTAX;
  const char *p = obj;
  const char *end = obj + obj_len;
  skip_ws(&p, end);
  if (p >= end || *p != '{') return JSON_ERR_SYNTAX;
  p++;
  skip_ws(&p, end);
  if (p < end && *p == '}') return JSON_NOT_FOUND;

  size_t klen = strlen(key);
  for (;;) {
    skip_ws(&p, end);
    const char *kstart = p;
    if (!skip_string(&p, end)) return JSON_ERR_SYNTAX;
    const char *kend = p;
    skip_ws(&p, end);
    if (p >= end || *p != ':') return JSON_ERR_SYNTAX;
    p++;
    skip_ws(&p, end);
    const char *vstart = p;
    if (!skip_value(&p, end, 1)) return JSON_ERR_SYNTAX;
    const char *vend = p;

    char *dec_key = NULL;
    size_t dec_len = 0;
    JsonStatus ks = stigi_json_decode_string(kstart, (size_t)(kend - kstart), &dec_key, &dec_len);
    if (ks != JSON_OK) return ks;
    bool match = (dec_len == klen && memcmp(dec_key, key, klen) == 0);
    free(dec_key);
    if (match) {
      out_val->ptr = vstart;
      out_val->len = (size_t)(vend - vstart);
      return JSON_OK;
    }

    skip_ws(&p, end);
    if (p >= end) return JSON_ERR_SYNTAX;
    if (*p == '}') return JSON_NOT_FOUND;
    if (*p != ',') return JSON_ERR_SYNTAX;
    p++;
  }
}

JsonStatus stigi_json_obj_get_string(const char *obj, size_t obj_len, const char *key,
                                     char **out_str, size_t *out_len) {
  JsonSpan sp = {NULL, 0};
  JsonStatus st = stigi_json_obj_get_raw(obj, obj_len, key, &sp);
  if (st != JSON_OK) return st;
  if (sp.len < 2U || sp.ptr[0] != '"') return JSON_ERR_SYNTAX;
  return stigi_json_decode_string(sp.ptr, sp.len, out_str, out_len);
}

JsonStatus stigi_json_obj_get_bool(const char *obj, size_t obj_len, const char *key,
                                   bool *out_val) {
  if (out_val == NULL) return JSON_ERR_SYNTAX;
  JsonSpan sp = {NULL, 0};
  JsonStatus st = stigi_json_obj_get_raw(obj, obj_len, key, &sp);
  if (st != JSON_OK) return st;
  if (sp.len == 4U && memcmp(sp.ptr, "true", 4) == 0) {
    *out_val = true;
    return JSON_OK;
  }
  if (sp.len == 5U && memcmp(sp.ptr, "false", 5) == 0) {
    *out_val = false;
    return JSON_OK;
  }
  return JSON_ERR_SYNTAX;
}

JsonStatus stigi_json_obj_get_object(const char *obj, size_t obj_len, const char *key,
                                     JsonSpan *out_obj) {
  if (out_obj == NULL) return JSON_ERR_SYNTAX;
  JsonSpan sp = {NULL, 0};
  JsonStatus st = stigi_json_obj_get_raw(obj, obj_len, key, &sp);
  if (st != JSON_OK) return st;
  if (sp.len < 2U || sp.ptr[0] != '{' || sp.ptr[sp.len - 1U] != '}') return JSON_ERR_SYNTAX;
  *out_obj = sp;
  return JSON_OK;
}
