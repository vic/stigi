// prompt: outcome of a checked signed 128-bit integer arithmetic operation.
pub enum Stigi.Datalog.IntegerOutcome:
  case IntegerValue(value: Stigi.Datalog.Integer)
  case IntegerOverflow
