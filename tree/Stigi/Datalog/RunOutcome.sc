// prompt: outcome of running the stratified semi-naive Datalog engine.
pub enum Stigi.Datalog.RunOutcome:
  case EngineComplete(derivedRows: U64, deltaRounds: U64, indexProbes: U64, rejectRows: U64)
  case EngineRejected(reason: Stigi.Datalog.RejectReason)
