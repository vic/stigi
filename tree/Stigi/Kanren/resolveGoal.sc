// prompt: resolve a trait goal through Bezem-bounded uKanren search with coherence and purity checks.
def Stigi.Kanren.resolveGoal(clauseCap: Stigi.UnitCap, clauses: Array[Stigi.Kanren.GivenClause](clauseCap), clauseCount: U64, goalTrait: U64, goalArg: U64, fuelMax: U64) <lessOrEqual(clauseCount, clauseCap), lessOrEqual(fuelMax, 256)>: Stigi.Kanren.SolveResult =
  Stigi.Kanren.resolveFrom(clauseCap, clauses, clauseCount, goalTrait, goalArg, Unrooted, fuelMax)
