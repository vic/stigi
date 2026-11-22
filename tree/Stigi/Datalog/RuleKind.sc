// prompt: kind of rule executed by the kernel Datalog evaluator.
pub enum Stigi.Datalog.RuleKind:
  case RuleCopy
  case RuleTransitiveJoin
  case RuleNegatedFilter
