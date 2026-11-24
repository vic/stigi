// prompt: represent the final outcome of stratified semi-naive Datalog evaluation.
pub enum Stigi.Datalog.Engine.EvalResult:
  case EvalDone(rowCount: U64, derivedCount: U64)
  case EvalRefused(reason: Stigi.Datalog.Check.Outcome)
  case EvalBudgetExceeded(derivedCount: U64)
  case EvalTableFull
  case EvalArithError
