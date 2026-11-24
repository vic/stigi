// prompt: represent the outcome of lowering a parsed syntax unit into flat Checked-IR tables.
enum Stigi.Elaborate.LowerOutcome:
  case Lowered(rootCore: U64, exprCount: U64, armCount: U64, listCount: U64)
  case LowerOverflow
  case LowerInvalid
