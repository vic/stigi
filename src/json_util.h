#ifndef STIGI_JSON_UTIL_H
#define STIGI_JSON_UTIL_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
  JSON_OK = 0,
  JSON_NOT_FOUND = 1,
  JSON_ERR_SYNTAX = 2,
  JSON_ERR_OOM = 3
} JsonStatus;

typedef struct {
  const char *ptr;
  size_t len;
} JsonSpan;

/* Strictly validate that buf[0..len) is a single complete JSON object {...} */
JsonStatus stigi_json_validate_object(const char *buf, size_t len);

/* Look up a top-level key in a JSON object {...}; returns raw value slice */
JsonStatus stigi_json_obj_get_raw(const char *obj, size_t obj_len, const char *key,
                                  JsonSpan *out_val);

/* Decode a raw JSON string token ("...") into a newly malloc'd NUL-terminated buffer */
JsonStatus stigi_json_decode_string(const char *raw, size_t raw_len,
                                    char **out_str, size_t *out_len);

/* Convenience helpers on a JSON object {...} */
JsonStatus stigi_json_obj_get_string(const char *obj, size_t obj_len, const char *key,
                                     char **out_str, size_t *out_len);
JsonStatus stigi_json_obj_get_bool(const char *obj, size_t obj_len, const char *key,
                                   bool *out_val);
JsonStatus stigi_json_obj_get_object(const char *obj, size_t obj_len, const char *key,
                                     JsonSpan *out_obj);

#endif
