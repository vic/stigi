// prompt: whether the BLAKE3 tree walk finished or the input needs a deeper parent stack.
enum Stigi.Hash.TreeOutcome:
  case TreeBuilt
  case TreeTooDeep
