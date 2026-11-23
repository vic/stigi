// prompt: closed outcome of executing the semi-naive hash-indexed Datalog engine.
pub enum Stigi.Datalog.EngineResult:
  case EngineConverged(rowCount: U64, rejectCount: U64)
  case EngineBudgetExceeded
  case EngineOutOfMemory
