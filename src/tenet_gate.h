#ifndef STIGI_TENET_GATE_H
#define STIGI_TENET_GATE_H

#include "common.h"
#include "lex.h"

#include <stddef.h>
#include <stdint.h>

typedef enum {
  STIGI_GATE_FORBIDDEN_WORD = 0,
  STIGI_GATE_BARE_POINTER,
  STIGI_GATE_EXPECT_FORM,
  STIGI_GATE_CRYPTIC_NAME,
  STIGI_GATE_CASE_PAIR,
  STIGI_GATE_PROMPT_MISSING,
  STIGI_GATE_NAME_CASE
} StigiGateCode;

typedef struct {
  StigiGateCode code;
  StigiSpan span;
  char msg[256];
} StigiGateFinding;

typedef struct {
  StigiGateFinding *items;
  uint32_t count;
  uint32_t cap;
} StigiGateFindings;

void stigi_gate_findings_init(StigiGateFindings *f);
void stigi_gate_findings_free(StigiGateFindings *f);
const char *stigi_gate_code_name(StigiGateCode code);

Status stigi_gate_check_tokens(const StigiTok *toks, uint32_t count,
                               StigiGateFindings *out, Diag *diag);
Status stigi_gate_check_source(const char *src, size_t len,
                               StigiGateFindings *out, Diag *diag);
Status stigi_gate_check_c_source(const char *src, size_t len,
                                 StigiGateFindings *out, Diag *diag);
Status stigi_gate_check_case_pairs(const char *const *unit_names, size_t count,
                                   StigiGateFindings *out, Diag *diag);

/* Stigi.Help for not-in-language constructs (ruling S56) */
const char *stigi_help_not_in_language(const char *kw);

/* Word table self-check helpers */
size_t stigi_gate_word_count(void);
size_t stigi_gate_word_decode(size_t idx, char *buf, size_t cap);
size_t stigi_gate_exit_word_count(void);
size_t stigi_gate_exit_word_decode(size_t idx, char *buf, size_t cap);

#endif
