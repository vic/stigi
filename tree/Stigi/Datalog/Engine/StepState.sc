// prompt: represent the updated row and derivation counters after evaluating one rule body.
enum Stigi.Datalog.Engine.StepState:
  case StepDone(rowCount: U64, derivedCount: U64)
  case StepFull
  case StepArithError
