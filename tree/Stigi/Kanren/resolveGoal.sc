// prompt: resolve a trait goal iteratively through Bezem-bounded uKanren search with coherence and purity checks.
def Stigi.Kanren.resolveGoal(clauseCap: Stigi.UnitCap, clauses: Array[Stigi.Kanren.GivenClause](clauseCap), clauseCount: U64, goalTrait: U64, goalArg: U64, fuelMax: U64) <lessOrEqual(clauseCount, clauseCap), lessOrEqual(fuelMax, 256)>: Stigi.Kanren.SolveResult =
  var curTrait: U64 = goalTrait
  var curArg: U64 = goalArg
  var fuelUsed: U64 = 0
  var rootChosen: U64 = 0
  var hasRoot: Bool = false
  var result: Stigi.Kanren.SolveResult = KanrenMissing
  var running: Bool = true
  while running do
    @<lessOrEqual(fuelUsed, fuelMax)>
    if fuelUsed >= fuelMax then
      result = KanrenOutOfFuel
      running = false
    else
      var idx: U64 = 0
      var hits: U64 = 0
      var hitSlot: U64 = 0
      var nextNeed: Bool = false
      var nextTrait: U64 = 0
      var nextArg: U64 = 0
      var effFound: Bool = false
      var effSlot: U64 = 0
      while idx < clauseCount && hits < 2 && !effFound do
        @<lessOrEqual(idx, clauseCount), lessOrEqual(hits, 2)>
        val GivenClause(traitTag, argTag, hasReq, reqTrait, reqArg, isEff) = clauses(idx)
        if traitTag == curTrait && argTag == curArg then
          if isEff then
            effFound = true
            effSlot = idx
          else
            if hits == 0 then
              hitSlot = idx
              nextNeed = hasReq
              nextTrait = reqTrait
              nextArg = reqArg
            else ()
            hits = hits + 1
        else ()
        idx = idx + 1
      if effFound then
        result = KanrenEffectful(effSlot)
        running = false
      else if hits == 0 then
        result = KanrenMissing
        running = false
      else if hits >= 2 then
        result = KanrenAmbiguous
        running = false
      else
        if !hasRoot then
          rootChosen = hitSlot
          hasRoot = true
        else ()
        if !nextNeed then
          result = KanrenFound(rootChosen)
          running = false
        else
          curTrait = nextTrait
          curArg = nextArg
          fuelUsed = fuelUsed + 1
  result
