// prompt: represent the outcome of lowering a parsed syntax unit into flat Checked-IR tables.
enum Stigi.Elaborate.LowerOutcome:
  case Lowered(rootCore: U64, core: Stigi.Core.Tables, scratch: Stigi.Elaborate.Scratch)
  case LowerOverflow
  case LowerInvalid
