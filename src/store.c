#define _POSIX_C_SOURCE 200809L

#include "store.h"
#include "blake3.h"
#include "print.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define STIGI_CACHE_MAGIC "STGCACHE"
#define STIGI_CACHE_VERSION 1U

static void hex64(const uint8_t id[32], char out[65]) {
  static const char H[] = "0123456789abcdef";
  for (int i = 0; i < 32; i++) {
    out[i * 2] = H[(id[i] >> 4) & 0x0f];
    out[i * 2 + 1] = H[id[i] & 0x0f];
  }
  out[64] = '\0';
}

static bool is_hex64(const char *s, size_t len) {
  if (len != 64U) return false;
  for (size_t i = 0; i < 64U; i++) {
    char c = s[i];
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
  }
  return true;
}

static const char *unit_kind_str(StigiDeclKind k) {
  switch (k) {
    case STIGI_DECL_ENUM: return "Enum";
    case STIGI_DECL_TYPE: return "Type";
    case STIGI_DECL_DEF: return "Def";
    case STIGI_DECL_LEMMA_DEF: return "Lemma";
    case STIGI_DECL_VAL: return "Val";
    case STIGI_DECL_CAPABILITY: return "Capability";
    case STIGI_DECL_HANDLER: return "Handler";
    case STIGI_DECL_EXTERN: return "Extern";
    case STIGI_DECL_RELATION: return "Relation";
    case STIGI_DECL_RULE: return "Rule";
    case STIGI_DECL_TEST: return "Test";
    case STIGI_DECL_NEG_TEST: return "NegTest";
  }
  return "Unknown";
}

static bool is_valid_kind_str(const char *s, size_t len) {
  static const char *const KINDS[] = {
      "Enum", "Type", "Def", "Lemma", "Val", "Capability",
      "Handler", "Extern", "Relation", "Rule", "Test", "NegTest"
  };
  for (size_t i = 0; i < sizeof(KINDS) / sizeof(KINDS[0]); i++) {
    if (strlen(KINDS[i]) == len && memcmp(KINDS[i], s, len) == 0) return true;
  }
  return false;
}

static bool is_safe_unit_name(const char *name, size_t len) {
  if (name == NULL || len == 0) return false;
  if (name[0] == '.' || name[len - 1U] == '.') return false;
  for (size_t i = 0; i < len; i++) {
    unsigned char c = (unsigned char)name[i];
    if (c == '/' || c == '\\' || c < 0x20U || c == 0x7FU) return false;
    if (c == '.' && i + 1U < len && name[i + 1U] == '.') return false;
  }
  return true;
}

static int cmp_unit_name(const void *pa, const void *pb) {
  const StigiImageUnit *a = *(const StigiImageUnit *const *)pa;
  const StigiImageUnit *b = *(const StigiImageUnit *const *)pb;
  return strcmp(a->name, b->name);
}

static int cmp_key32(const void *pa, const void *pb) {
  return memcmp(pa, pb, 32U);
}

static void put_u32_le(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)(v & 0xFFU);
  p[1] = (uint8_t)((v >> 8) & 0xFFU);
  p[2] = (uint8_t)((v >> 16) & 0xFFU);
  p[3] = (uint8_t)((v >> 24) & 0xFFU);
}

static uint32_t get_u32_le(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static Status ensure_parent_dirs(const char *path, char *err_buf, size_t err_cap) {
  if (path == NULL) {
    if (err_buf != NULL && err_cap > 0) (void)snprintf(err_buf, err_cap, "null path");
    return STIGI_ERR_INVALID;
  }
  size_t len = strlen(path);
  if (len == 0 || len >= 1024U) {
    if (err_buf != NULL && err_cap > 0) (void)snprintf(err_buf, err_cap, "invalid path length");
    return STIGI_ERR_INVALID;
  }
  char tmp[1024];
  memcpy(tmp, path, len + 1U);
  for (char *p = tmp + 1; *p != '\0'; p++) {
    if (*p == '/') {
      *p = '\0';
      if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
        if (err_buf != NULL && err_cap > 0) {
          (void)snprintf(err_buf, err_cap, "cannot create directory '%s'", tmp);
        }
        return STIGI_ERR_IO;
      }
      *p = '/';
    }
  }
  return STIGI_OK;
}

static Status atomic_write_file(const char *path, const void *data, size_t len,
                                char *err_buf, size_t err_cap) {
  char tmp_path[1100];
  int tn = snprintf(tmp_path, sizeof tmp_path, "%s.tmp.%ld", path, (long)getpid());
  if (tn < 0 || (size_t)tn >= sizeof tmp_path) {
    if (err_buf != NULL && err_cap > 0) (void)snprintf(err_buf, err_cap, "path too long");
    return STIGI_ERR_INVALID;
  }

  FILE *f = fopen(tmp_path, "wb");
  if (f == NULL) {
    if (err_buf != NULL && err_cap > 0) {
      (void)snprintf(err_buf, err_cap, "cannot open temp file '%s'", tmp_path);
    }
    return STIGI_ERR_IO;
  }
  if (len > 0 && fwrite(data, 1, len, f) != len) {
    (void)fclose(f);
    (void)unlink(tmp_path);
    if (err_buf != NULL && err_cap > 0) {
      (void)snprintf(err_buf, err_cap, "write failed for '%s'", path);
    }
    return STIGI_ERR_IO;
  }
  if (fflush(f) != 0 || fsync(fileno(f)) != 0) {
    (void)fclose(f);
    (void)unlink(tmp_path);
    if (err_buf != NULL && err_cap > 0) {
      (void)snprintf(err_buf, err_cap, "fsync failed for '%s'", path);
    }
    return STIGI_ERR_IO;
  }
  if (fclose(f) != 0) {
    (void)unlink(tmp_path);
    if (err_buf != NULL && err_cap > 0) {
      (void)snprintf(err_buf, err_cap, "fclose failed for '%s'", path);
    }
    return STIGI_ERR_IO;
  }
  if (rename(tmp_path, path) != 0) {
    (void)unlink(tmp_path);
    if (err_buf != NULL && err_cap > 0) {
      (void)snprintf(err_buf, err_cap, "rename failed for '%s'", path);
    }
    return STIGI_ERR_IO;
  }
  return STIGI_OK;
}

typedef struct {
  char **paths;
  uint32_t count;
  uint32_t cap;
} PathList;

static void pathlist_free(PathList *pl) {
  for (uint32_t i = 0; i < pl->count; i++) free(pl->paths[i]);
  free(pl->paths);
  pl->paths = NULL;
  pl->count = 0;
  pl->cap = 0;
}

static Status pathlist_push(PathList *pl, const char *path, Diag *diag) {
  if (pl->count == pl->cap) {
    uint32_t ncap = pl->cap == 0 ? 16U : pl->cap * 2U;
    if (ncap <= pl->cap) return diag_set(diag, STIGI_ERR_OVERFLOW, -1, "pathlist overflow");
    char **np = realloc(pl->paths, (size_t)ncap * sizeof *np);
    if (np == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
    pl->paths = np;
    pl->cap = ncap;
  }
  size_t clen = strlen(path);
  pl->paths[pl->count] = malloc(clen + 1U);
  if (pl->paths[pl->count] == NULL) return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
  memcpy(pl->paths[pl->count], path, clen + 1U);
  pl->count++;
  return STIGI_OK;
}

static Status collect_sc_files(const char *dir, PathList *pl, Diag *diag) {
  DIR *dp = opendir(dir);
  if (dp == NULL) {
    return diag_set(diag, STIGI_ERR_IO, errno, "cannot open directory '%s'", dir);
  }
  struct dirent *ent;
  Status st = STIGI_OK;
  while ((ent = readdir(dp)) != NULL && st == STIGI_OK) {
    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
    char child[1024];
    int cn = snprintf(child, sizeof child, "%s/%s", dir, ent->d_name);
    if (cn < 0 || (size_t)cn >= sizeof child) {
      st = diag_set(diag, STIGI_ERR_INVALID, -1, "path too long under '%s'", dir);
      break;
    }
    struct stat cst;
    if (lstat(child, &cst) != 0) continue;
    if (S_ISLNK(cst.st_mode)) continue; /* never follow symlinks */
    if (S_ISDIR(cst.st_mode)) {
      st = collect_sc_files(child, pl, diag);
    } else if (S_ISREG(cst.st_mode)) {
      size_t n = strlen(ent->d_name);
      if (n > 3U && strcmp(ent->d_name + n - 3U, ".sc") == 0) {
        st = pathlist_push(pl, child, diag);
      }
    }
  }
  (void)closedir(dp);
  return st;
}

static Status read_whole_file(const char *path, char **out_buf, size_t *out_len, Diag *diag) {
  FILE *f = fopen(path, "rb");
  if (f == NULL) {
    return diag_set(diag, STIGI_ERR_IO, errno, "cannot read '%s'", path);
  }
  if (fseek(f, 0, SEEK_END) != 0) {
    (void)fclose(f);
    return diag_set(diag, STIGI_ERR_IO, errno, "fseek failed on '%s'", path);
  }
  long sz = ftell(f);
  if (sz < 0) {
    (void)fclose(f);
    return diag_set(diag, STIGI_ERR_IO, errno, "ftell failed on '%s'", path);
  }
  if (fseek(f, 0, SEEK_SET) != 0) {
    (void)fclose(f);
    return diag_set(diag, STIGI_ERR_IO, errno, "fseek rewind failed on '%s'", path);
  }
  char *buf = malloc((size_t)sz + 1U);
  if (buf == NULL) {
    (void)fclose(f);
    return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
  }
  size_t nr = sz > 0 ? fread(buf, 1, (size_t)sz, f) : 0U;
  if (nr != (size_t)sz || ferror(f)) {
    free(buf);
    (void)fclose(f);
    return diag_set(diag, STIGI_ERR_IO, errno, "fread failed on '%s'", path);
  }
  if (fclose(f) != 0) {
    free(buf);
    return diag_set(diag, STIGI_ERR_IO, errno, "fclose failed on '%s'", path);
  }
  buf[nr] = '\0';
  *out_buf = buf;
  *out_len = nr;
  return STIGI_OK;
}

static Status write_cache_file(const StigiDoor *door, const char *dir,
                               char *err_buf, size_t err_cap) {
  uint32_t vcnt = door->verdict_cache.count;
  uint32_t tcnt = door->test_cache.count;

  size_t total_keys = (size_t)vcnt + (size_t)tcnt;
  size_t payload_len = 8U + 4U + 4U + 4U + total_keys * 32U;
  size_t total_len = payload_len + 32U;
  uint8_t *buf = malloc(total_len);
  if (buf == NULL) return STIGI_ERR_OOM;

  memcpy(buf, STIGI_CACHE_MAGIC, 8U);
  put_u32_le(buf + 8U, STIGI_CACHE_VERSION);
  put_u32_le(buf + 12U, vcnt);
  put_u32_le(buf + 16U, tcnt);

  uint8_t *vptr = buf + 20U;
  uint32_t vi = 0;
  for (uint32_t i = 0; i < door->verdict_cache.cap && vi < vcnt; i++) {
    if (door->verdict_cache.slots[i].used) {
      memcpy(vptr + (size_t)vi * 32U, door->verdict_cache.slots[i].key, 32U);
      vi++;
    }
  }
  if (vcnt > 1U) qsort(vptr, vcnt, 32U, cmp_key32);

  uint8_t *tptr = vptr + (size_t)vcnt * 32U;
  uint32_t ti = 0;
  for (uint32_t i = 0; i < door->test_cache.cap && ti < tcnt; i++) {
    if (door->test_cache.slots[i].used) {
      memcpy(tptr + (size_t)ti * 32U, door->test_cache.slots[i].key, 32U);
      ti++;
    }
  }
  if (tcnt > 1U) qsort(tptr, tcnt, 32U, cmp_key32);

  blake3_hash(buf, payload_len, buf + payload_len);

  char cache_path[1024];
  int cn = snprintf(cache_path, sizeof cache_path, "%s/cache", dir);
  if (cn < 0 || (size_t)cn >= sizeof cache_path) {
    free(buf);
    if (err_buf != NULL && err_cap > 0) (void)snprintf(err_buf, err_cap, "cache path too long");
    return STIGI_ERR_INVALID;
  }
  Status st = atomic_write_file(cache_path, buf, total_len, err_buf, err_cap);
  free(buf);
  return st;
}

Status stigi_store_dir(const StigiDoor *door, const char *dir,
                       char *err_buf, size_t err_cap) {
  if (door == NULL || dir == NULL || dir[0] == '\0') {
    if (err_buf != NULL && err_cap > 0) (void)snprintf(err_buf, err_cap, "invalid arguments");
    return STIGI_ERR_INVALID;
  }
  /* door.md:124: missing directory is a typed error (never auto-create dir) */
  struct stat st_dir;
  if (stat(dir, &st_dir) != 0 || !S_ISDIR(st_dir.st_mode)) {
    if (err_buf != NULL && err_cap > 0) {
      (void)snprintf(err_buf, err_cap, "store directory does not exist or is not a directory: '%s'", dir);
    }
    return STIGI_ERR_IO;
  }

  /* Collect existing .sc files so stale files can be pruned after write */
  PathList old_files = {NULL, 0, 0};
  Diag tmp_diag;
  diag_clear(&tmp_diag);
  Status col_st = collect_sc_files(dir, &old_files, &tmp_diag);
  if (col_st != STIGI_OK) {
    pathlist_free(&old_files);
    if (err_buf != NULL && err_cap > 0) (void)snprintf(err_buf, err_cap, "%s", tmp_diag.msg);
    return col_st;
  }

  const StigiImage *img = &door->image;
  const StigiImageUnit **sorted = NULL;
  if (img->unit_count > 0) {
    sorted = malloc((size_t)img->unit_count * sizeof *sorted);
    if (sorted == NULL) {
      pathlist_free(&old_files);
      return STIGI_ERR_OOM;
    }
    for (uint32_t i = 0; i < img->unit_count; i++) sorted[i] = &img->units[i];
    qsort(sorted, img->unit_count, sizeof *sorted, cmp_unit_name);
  }

  /* Build index in memory */
  size_t idx_cap = 1024U;
  size_t idx_len = 0;
  char *idx_buf = malloc(idx_cap);
  if (idx_buf == NULL) {
    free(sorted);
    pathlist_free(&old_files);
    return STIGI_ERR_OOM;
  }
  idx_buf[0] = '\0';

  PathList written_files = {NULL, 0, 0};
  Arena tmp;
  arena_init(&tmp);
  Status res = STIGI_OK;

  for (uint32_t i = 0; i < img->unit_count && res == STIGI_OK; i++) {
    const StigiImageUnit *u = sorted[i];
    if (!is_safe_unit_name(u->name, u->name_len)) {
      if (err_buf != NULL && err_cap > 0) {
        (void)snprintf(err_buf, err_cap, "unsafe unit name '%s'", u->name);
      }
      res = STIGI_ERR_INVALID;
      break;
    }

    char rel[512];
    if (u->name_len + 4U > sizeof rel) {
      if (err_buf != NULL && err_cap > 0) (void)snprintf(err_buf, err_cap, "unit name too long");
      res = STIGI_ERR_INVALID;
      break;
    }
    for (size_t k = 0; k < u->name_len; k++) {
      rel[k] = u->name[k] == '.' ? '/' : u->name[k];
    }
    memcpy(rel + u->name_len, ".sc", 4U);

    char fpath[1024];
    int fn = snprintf(fpath, sizeof fpath, "%s/%s", dir, rel);
    if (fn < 0 || (size_t)fn >= sizeof fpath) {
      if (err_buf != NULL && err_cap > 0) (void)snprintf(err_buf, err_cap, "unit path too long");
      res = STIGI_ERR_INVALID;
      break;
    }
    res = ensure_parent_dirs(fpath, err_buf, err_cap);
    if (res != STIGI_OK) break;

    arena_reset(&tmp);
    char *printed = NULL;
    size_t plen = 0;
    Diag d;
    diag_clear(&d);
    res = stigi_print_unit(u->ast, &printed, &plen, &tmp, &d);
    if (res != STIGI_OK) {
      if (err_buf != NULL && err_cap > 0) (void)snprintf(err_buf, err_cap, "%s", d.msg);
      break;
    }

    res = atomic_write_file(fpath, printed, plen, err_buf, err_cap);
    if (res != STIGI_OK) break;

    res = pathlist_push(&written_files, fpath, &d);
    if (res != STIGI_OK) {
      if (err_buf != NULL && err_cap > 0) (void)snprintf(err_buf, err_cap, "%s", d.msg);
      break;
    }

    char h[65];
    hex64(u->unit_id, h);
    const char *kstr = unit_kind_str(u->ast->decl->kind);
    char line[512];
    int ln = snprintf(line, sizeof line, "%s\t%s\t%s\n", u->name, h, kstr);
    if (ln < 0 || (size_t)ln >= sizeof line) {
      if (err_buf != NULL && err_cap > 0) (void)snprintf(err_buf, err_cap, "index line too long");
      res = STIGI_ERR_INVALID;
      break;
    }
    if (idx_len + (size_t)ln + 1U > idx_cap) {
      while (idx_len + (size_t)ln + 1U > idx_cap) {
        if (idx_cap > SIZE_MAX / 2U) { res = STIGI_ERR_OOM; break; }
        idx_cap *= 2U;
      }
      if (res != STIGI_OK) break;
      char *nb = realloc(idx_buf, idx_cap);
      if (nb == NULL) { res = STIGI_ERR_OOM; break; }
      idx_buf = nb;
    }
    memcpy(idx_buf + idx_len, line, (size_t)ln);
    idx_len += (size_t)ln;
    idx_buf[idx_len] = '\0';
  }

  if (res == STIGI_OK) {
    char idx_path[1024];
    int in = snprintf(idx_path, sizeof idx_path, "%s/index", dir);
    if (in < 0 || (size_t)in >= sizeof idx_path) {
      if (err_buf != NULL && err_cap > 0) (void)snprintf(err_buf, err_cap, "index path too long");
      res = STIGI_ERR_INVALID;
    } else {
      res = atomic_write_file(idx_path, idx_buf, idx_len, err_buf, err_cap);
    }
  }

  if (res == STIGI_OK) {
    res = write_cache_file(door, dir, err_buf, err_cap);
  }

  /* Prune stale .sc files not in the image */
  if (res == STIGI_OK) {
    for (uint32_t i = 0; i < old_files.count; i++) {
      bool kept = false;
      for (uint32_t j = 0; j < written_files.count; j++) {
        if (strcmp(old_files.paths[i], written_files.paths[j]) == 0) {
          kept = true;
          break;
        }
      }
      if (!kept) (void)unlink(old_files.paths[i]);
    }
  }

  free(idx_buf);
  arena_free(&tmp);
  free(sorted);
  pathlist_free(&written_files);
  pathlist_free(&old_files);
  return res;
}

static int cmp_cstr(const void *pa, const void *pb) {
  return strcmp(*(const char *const *)pa, *(const char *const *)pb);
}

static Status verify_index_file(const char *dir, Diag *diag) {
  char idx_path[1024];
  int in = snprintf(idx_path, sizeof idx_path, "%s/index", dir);
  if (in < 0 || (size_t)in >= sizeof idx_path) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "index path too long");
  }
  struct stat st;
  if (lstat(idx_path, &st) != 0) {
    if (errno == ENOENT) return STIGI_OK;
    return diag_set(diag, STIGI_ERR_IO, errno, "cannot stat index '%s'", idx_path);
  }
  if (!S_ISREG(st.st_mode)) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "index '%s' is not a regular file", idx_path);
  }
  char *buf = NULL;
  size_t len = 0;
  Status rs = read_whole_file(idx_path, &buf, &len, diag);
  if (rs != STIGI_OK) return rs;

  if (memchr(buf, '\0', len) != NULL) {
    free(buf);
    return diag_set(diag, STIGI_ERR_INVALID, -1, "corrupt index: embedded NUL byte");
  }

  const char *p = buf;
  const char *end = buf + len;
  while (p < end) {
    const char *nl = memchr(p, '\n', (size_t)(end - p));
    if (nl == NULL) {
      free(buf);
      return diag_set(diag, STIGI_ERR_INVALID, -1, "corrupt index: unterminated line");
    }
    const char *t1 = memchr(p, '\t', (size_t)(nl - p));
    if (t1 == NULL) {
      free(buf);
      return diag_set(diag, STIGI_ERR_INVALID, -1, "corrupt index: missing tab");
    }
    const char *t2 = memchr(t1 + 1, '\t', (size_t)(nl - (t1 + 1)));
    if (t2 == NULL) {
      free(buf);
      return diag_set(diag, STIGI_ERR_INVALID, -1, "corrupt index: missing second tab");
    }
    if (!is_safe_unit_name(p, (size_t)(t1 - p)) ||
        !is_hex64(t1 + 1, (size_t)(t2 - (t1 + 1))) ||
        !is_valid_kind_str(t2 + 1, (size_t)(nl - (t2 + 1)))) {
      free(buf);
      return diag_set(diag, STIGI_ERR_INVALID, -1, "corrupt index: invalid entry format");
    }
    p = nl + 1;
  }
  free(buf);
  return STIGI_OK;
}

static Status load_and_verify_cache(StigiDoor *door, const char *dir, Diag *diag) {
  char cache_path[1024];
  int cn = snprintf(cache_path, sizeof cache_path, "%s/cache", dir);
  if (cn < 0 || (size_t)cn >= sizeof cache_path) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "cache path too long");
  }
  struct stat st;
  if (lstat(cache_path, &st) != 0) {
    if (errno == ENOENT) return STIGI_OK;
    return diag_set(diag, STIGI_ERR_IO, errno, "cannot stat cache '%s'", cache_path);
  }
  if (!S_ISREG(st.st_mode)) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "cache '%s' is not a regular file", cache_path);
  }
  char *buf = NULL;
  size_t len = 0;
  Status rs = read_whole_file(cache_path, &buf, &len, diag);
  if (rs != STIGI_OK) return rs;

  if (len < 20U + 32U) {
    free(buf);
    return diag_set(diag, STIGI_ERR_INVALID, -1, "corrupt cache: file too short");
  }
  const uint8_t *u = (const uint8_t *)buf;
  if (memcmp(u, STIGI_CACHE_MAGIC, 8U) != 0) {
    free(buf);
    return diag_set(diag, STIGI_ERR_INVALID, -1, "corrupt cache: invalid magic");
  }
  uint32_t ver = get_u32_le(u + 8U);
  if (ver != STIGI_CACHE_VERSION) {
    free(buf);
    return diag_set(diag, STIGI_ERR_INVALID, -1, "corrupt cache: unsupported version %u", ver);
  }
  uint32_t vcnt = get_u32_le(u + 12U);
  uint32_t tcnt = get_u32_le(u + 16U);
  uint64_t total_keys = (uint64_t)vcnt + (uint64_t)tcnt;
  if (total_keys > (SIZE_MAX - 52U) / 32U) {
    free(buf);
    return diag_set(diag, STIGI_ERR_INVALID, -1, "corrupt cache: key count overflow");
  }
  size_t expected_len = 20U + (size_t)total_keys * 32U + 32U;
  if (len != expected_len) {
    free(buf);
    return diag_set(diag, STIGI_ERR_INVALID, -1, "corrupt cache: length mismatch");
  }
  uint8_t digest[32];
  blake3_hash(u, expected_len - 32U, digest);
  if (memcmp(digest, u + (expected_len - 32U), 32U) != 0) {
    free(buf);
    return diag_set(diag, STIGI_ERR_INVALID, -1, "corrupt cache: BLAKE3 checksum mismatch");
  }

  Status is = STIGI_OK;
  const uint8_t *vptr = u + 20U;
  for (uint32_t i = 0; i < vcnt && is == STIGI_OK; i++) {
    is = stigi_door_cache_add_verdict(door, vptr + (size_t)i * 32U, diag);
  }
  const uint8_t *tptr = vptr + (size_t)vcnt * 32U;
  for (uint32_t i = 0; i < tcnt && is == STIGI_OK; i++) {
    is = stigi_door_cache_add_test(door, tptr + (size_t)i * 32U, diag);
  }
  free(buf);
  return is;
}

Status stigi_load_dir(StigiDoor *door, const char *dir,
                      StigiDoorResult *out, Diag *diag) {
  if (door == NULL || dir == NULL || out == NULL) {
    return diag_set(diag, STIGI_ERR_INVALID, -1, "stigi_load_dir: invalid arguments");
  }

  Status vs = verify_index_file(dir, diag);
  if (vs != STIGI_OK) return vs;

  vs = load_and_verify_cache(door, dir, diag);
  if (vs != STIGI_OK) return vs;

  PathList pl = {NULL, 0, 0};
  Status st = collect_sc_files(dir, &pl, diag);
  if (st != STIGI_OK) {
    pathlist_free(&pl);
    return st;
  }
  if (pl.count > 1U) qsort(pl.paths, pl.count, sizeof *pl.paths, cmp_cstr);

  char **srcs = calloc(pl.count > 0 ? pl.count : 1U, sizeof *srcs);
  size_t *lens = calloc(pl.count > 0 ? pl.count : 1U, sizeof *lens);
  if (srcs == NULL || lens == NULL) {
    free(srcs);
    free(lens);
    pathlist_free(&pl);
    return diag_set(diag, STIGI_ERR_OOM, -1, "OOM");
  }

  for (uint32_t i = 0; i < pl.count && st == STIGI_OK; i++) {
    st = read_whole_file(pl.paths[i], &srcs[i], &lens[i], diag);
  }

  if (st == STIGI_OK) {
    st = stigi_door_replace_all(door, (const char *const *)srcs, lens, pl.count, out, diag);
  }

  for (uint32_t i = 0; i < pl.count; i++) {
    free(srcs[i]);
  }
  free(srcs);
  free(lens);
  pathlist_free(&pl);
  return st;
}
