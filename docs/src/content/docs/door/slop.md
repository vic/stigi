---
title: Shaking out slop
description: Dead code, unused values and constant work never enter the image.
sidebar:
  order: 3
---

Agents love extra helpers, unused variables, defensive branches that can never fail, and constants computed again at run time.

Every time a batch passes the checks, the door runs whole-image **tree shaking** and **compile-time evaluation**:

- Unreachable functions, unused variables, unused results, unused parameters and facts that change no proof are removed.
- Pure constant expressions are evaluated at compile time and replaced by their values.
- The shaken form is checked again.

The reply says what was removed and gives the smallest checked source:

```text
admitted Demo.Parser.readHeader   laws ok   tests 12/12
  shaken: 2 unused helpers, 1 unused parameter, 3 constant folds
  minimal: (checked) the shaken source of Demo.Parser.readHeader
```

Some slop is a hard error rather than a cleanup. An expression whose value nobody uses and that has no effect is rejected. A `pub` unit that no other namespace uses is rejected. A unit that no root reaches does not stay.

The result: only the smallest sound program lives in the image.

Next: [no panics, no overflow, no stack explosions](/door/safety/).
