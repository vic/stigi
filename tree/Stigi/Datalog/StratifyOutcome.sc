// prompt: outcome of stratifying a relation dependency graph.
pub enum Stigi.Datalog.StratifyOutcome:
  case StratifiedValid(maxStratum: U64)
  case StratifyCycle
