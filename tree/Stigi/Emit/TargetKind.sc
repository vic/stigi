// prompt: closed statement-to-value emission target mode for ISO C11 lowering.
enum Stigi.Emit.TargetKind:
  case TargetReturn
  case TargetAssign(tempNum: U64)
  case TargetDiscard
