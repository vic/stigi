// prompt: failure reason codes for the self-hosted Datalog engine matching vic/spec/datalog-engine.md.
pub enum Stigi.Datalog.RejectReason:
  case ReasonNegationCycle
  case ReasonBudgetExceeded
  case ReasonArithError
  case ReasonUnsafeRule
  case ReasonArityError
  case ReasonUnknownRelation
  case ReasonOutOfMemory
