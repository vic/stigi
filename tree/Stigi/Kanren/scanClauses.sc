// prompt: scan the given clauses from an index for a goal, stopping at a second or an effectful match.
def Stigi.Kanren.scanClauses(clauseCap: Stigi.UnitCap, clauses: Array[Stigi.Kanren.GivenClause](clauseCap), clauseCount: U64, goalTrait: U64, goalArg: U64, idx: U64, found: Stigi.Kanren.Match, remaining: U64) <lessOrEqual(clauseCount, clauseCap), decreases(remaining)>: Stigi.Kanren.Match =
  if remaining < 1 || clauseCount <= idx then found else
    val GivenClause(traitTag, argTag, need, isEffectful) = clauses(idx)
    if traitTag != goalTrait || argTag != goalArg then Stigi.Kanren.scanClauses(clauseCap, clauses, clauseCount, goalTrait, goalArg, idx + 1, found, remaining - 1) else if isEffectful then EffectMatch(idx) else found match
      case Unmatched => Stigi.Kanren.scanClauses(clauseCap, clauses, clauseCount, goalTrait, goalArg, idx + 1, OneMatch(idx, need), remaining - 1)
      case OneMatch(_, _) | TwoMatches | EffectMatch(_) => TwoMatches
