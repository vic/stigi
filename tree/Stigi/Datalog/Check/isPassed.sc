// prompt: return true when a checker outcome indicates success.
def Stigi.Datalog.Check.isPassed(outcome: Stigi.Datalog.Check.Outcome): Bool =
  outcome match
    case CheckPassed(_) => true
    case UnresolvedRelation(_, _) => false
    case ArityMismatch(_, _) => false
    case RuleUnsafe(_, _) => false
    case NegationCycle(_) => false
