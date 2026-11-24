// prompt: resolve a trait goal from the chosen root through the unique matching givens until one is ground or the fuel runs out.
def Stigi.Kanren.resolveFrom(clauseCap: Stigi.UnitCap, clauses: Array[Stigi.Kanren.GivenClause](clauseCap), clauseCount: U64, goalTrait: U64, goalArg: U64, root: Stigi.Kanren.Root, fuel: U64) <lessOrEqual(clauseCount, clauseCap), decreases(fuel)>: Stigi.Kanren.SolveResult =
  if fuel < 1 then KanrenOutOfFuel else
    Stigi.Kanren.scanClauses(clauseCap, clauses, clauseCount, goalTrait, goalArg, 0, Unmatched, clauseCount) match
      case Unmatched => KanrenMissing
      case TwoMatches => KanrenAmbiguous
      case EffectMatch(slot) => KanrenEffectful(slot)
      case OneMatch(slot, need) =>
        val chosen: U64 = root match
          case Unrooted => slot
          case Rooted(first) => first
        need match
          case Ground => KanrenFound(chosen)
          case Requires(nextTrait, nextArg) => Stigi.Kanren.resolveFrom(clauseCap, clauses, clauseCount, nextTrait, nextArg, Rooted(chosen), fuel - 1)
