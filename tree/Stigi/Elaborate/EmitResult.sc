// prompt: represent the outcome of emitting one postorder Checked-IR expression node.
enum Stigi.Elaborate.EmitResult:
  case EmitDone(nextCore: U64, nextList: U64)
  case EmitFull
  case EmitInvalid
