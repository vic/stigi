// prompt: represent the verdict of static safety and stratification checking on a Datalog program.
pub enum Stigi.Datalog.Check.Outcome:
  case CheckPassed(maxStratum: U64)
  case UnresolvedRelation(ruleIndex: U64, relNum: U64)
  case ArityMismatch(ruleIndex: U64, relNum: U64)
  case RuleUnsafe(ruleIndex: U64, varNum: U64)
  case NegationCycle(relNum: U64)
