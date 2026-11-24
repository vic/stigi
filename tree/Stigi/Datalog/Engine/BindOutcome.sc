// prompt: represent the outcome of matching an atom against a stored tuple row.
enum Stigi.Datalog.Engine.BindOutcome:
  case BindMatched(boundMask: U64)
  case BindFailed
