// prompt: represent the outcome of pushing syntax child work items onto the elaboration stack.
enum Stigi.Elaborate.PushResult:
  case PushDone(nextTop: U64)
  case PushFull
