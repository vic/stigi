// prompt: outcome of stepping one syntax node during unit length walk: updated state or stopped failure.
enum Stigi.Store.StepUnitOutcome:
  case StepUnitDone(state: Stigi.Store.LengthUnitState)
  case StepUnitStopped(failure: Stigi.Store.FactsFailure)
