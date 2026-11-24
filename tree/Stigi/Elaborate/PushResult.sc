// prompt: represent the outcome of pushing work items: the scratch with the new stack top, or the work stack is full.
enum Stigi.Elaborate.PushResult:
  case PushDone(scratch: Stigi.Elaborate.Scratch)
  case PushFull
