#ifndef STIGI_PATTERN_H
#define STIGI_PATTERN_H

#include "ast.h"
#include "common.h"
#include "names.h"
#include "types.h"

/* Elaborate S121-S123 surface constructs across every unit in the image:
 * - Erlang-style multi-head `def` (`clauses`) -> 1-level decision tree
 * - Deep nested patterns, multi-scrutinee `match`, `if` guards -> Maranget 1-level IR
 * - Flat exhaustive `val Pat = expr else case ...` -> exhaustive 1-level match
 * - First-argument pipe `x |> f(a)` -> `f(x, a)`
 * Populates `elab_body` on every def/lemma/val/test/handler-op without altering surface AST
 * (preserving `print(parse(x)) == x`). When `diags != NULL`, emits `not-exhaustive` (with
 * concrete witness), `wildcard-arm`, `pattern-arity`, and `never-reached`. */
Status stigi_elaborate_image(StigiImage *img, StigiTypeDiags *diags, Diag *diag);

#endif
