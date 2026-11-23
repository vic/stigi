#ifndef STIGI_DOOR_H
#define STIGI_DOOR_H

#include "common.h"
#include "datalog.h"
#include "extract.h"
#include "names.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define STIGI_DEFAULT_ROW_BUDGET 1000000ULL

typedef struct {
  char subject[128];
  char code[64];
  char msg[256];
} StigiDoorReject;

typedef struct {
  bool accepted;
  Status status;
  StigiDoorReject *rejects;
  uint32_t reject_count;
  uint32_t reject_cap;
  uint32_t verdicts_computed;
  uint32_t verdicts_cached;
  uint32_t tests_computed;
  uint32_t tests_cached;
} StigiDoorResult;

typedef struct {
  uint8_t key[32];
  bool used;
} StigiCacheSlot;

typedef struct {
  StigiCacheSlot *slots;
  uint32_t count;
  uint32_t cap;
} StigiKeySet;

typedef struct {
  StigiImage image;
  char **unit_srcs;
  size_t *unit_lens;
  uint32_t src_count;
  uint32_t src_cap;

  StigiDl dl;
  char *base_rules_dl;
  size_t base_rules_dl_len;
  char *rules_dl;
  size_t rules_dl_len;
  uint8_t base_rules_hash[32];
  uint8_t seed_id[32]; /* BLAKE3 of this seed binary: the verdict logic is part of every cache key (T1) */
  uint64_t row_budget;
  bool kernel_mode;

  StigiKeySet verdict_cache;
  StigiKeySet test_cache;
} StigiDoor;

void stigi_door_result_init(StigiDoorResult *r);
void stigi_door_result_free(StigiDoorResult *r);

/* Initialize door by preprocessing all_dl_path (e.g. "vic/spec/datalog/all.dl") */
Status stigi_door_init(StigiDoor *door, const char *all_dl_path,
                       uint64_t row_budget, bool kernel_mode, Diag *diag);
void stigi_door_free(StigiDoor *door);

/* Keep only the cache keys that the current image, rules and seed produce (a stored cache holds no garbage) */
Status stigi_door_prune_caches(StigiDoor *door, Diag *diag);

/* Atomic transaction: submit adds/replaces units; check runs pipeline and drops candidate */
Status stigi_door_submit(StigiDoor *door, const char *const *srcs, const size_t *lens,
                         uint32_t count, StigiDoorResult *out, Diag *diag);
Status stigi_door_check(StigiDoor *door, const char *const *srcs, const size_t *lens,
                        uint32_t count, StigiDoorResult *out, Diag *diag);
Status stigi_door_drop(StigiDoor *door, const char *const *names, uint32_t count,
                       StigiDoorResult *out, Diag *diag);
Status stigi_door_tx(StigiDoor *door, const char *const *srcs, const size_t *lens,
                     uint32_t src_count, const char *const *drop_names, uint32_t drop_count,
                     bool commit, StigiDoorResult *out, Diag *diag);
Status stigi_door_replace_all(StigiDoor *door, const char *const *srcs, const size_t *lens,
                              uint32_t count, StigiDoorResult *out, Diag *diag);

/* S126: garbage-collect unreachable units from roots (main, pub, Test, NegTest) atomically */
Status stigi_door_gc(StigiDoor *door, uint32_t *out_removed_units,
                     StigiDoorResult *out, Diag *diag);

/* Cache persistence helpers (for store.c) */
Status stigi_door_cache_add_verdict(StigiDoor *door, const uint8_t key[32], Diag *diag);
Status stigi_door_cache_add_test(StigiDoor *door, const uint8_t key[32], Diag *diag);

/* Ruling S38: re-check all stored units under new_budget; reject if any hits datalog-budget */
Status stigi_door_set_budget(StigiDoor *door, uint64_t new_budget,
                             StigiDoorResult *out, Diag *diag);

/* Extract full image facts including door metadata (imageMode, Test/NegTest verdicts) */
Status stigi_door_extract_image(const StigiDoor *door, StigiFactSet *fs, Diag *diag);

typedef struct {
  uint32_t unit_dirs;                 /* admitted units: one fact directory each */
  uint32_t negtest_dirs;              /* NegTest bodies whose rejection comes from Datalog: one directory each */
  uint32_t negtest_local_dirs;        /* their local units: one evaluation, one directory inside it */
  uint32_t negtests_without_datalog;  /* NegTest bodies rejected before Datalog (names, types): no directory */
  uint64_t rows;
} StigiExportStats;

/* S134 2: `dir/rules.dl` once, and one fact directory `dir/<hex unit id>` per door evaluation (the fact set the
 * door loads for that unit; a NegTest body is one directory, named by the NegTest's id). */
Status stigi_door_export(StigiDoor *door, const char *dir, StigiExportStats *stats, Diag *diag);

#endif
