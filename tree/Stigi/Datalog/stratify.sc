// prompt: compute stratified levels for a relation dependency graph and reject any negation cycle.
pub def Stigi.Datalog.stratify {region: Stigi.Region, Stigi.Mut} (relationCount: U64, edgeCount: U64, edgeHead: Array[U64](edgeCount), edgeBody: Array[U64](edgeCount), edgeNegated: Array[U8](edgeCount)) <below(0, relationCount), below(relationCount, 1024), below(edgeCount, 65536)>: Stigi.Datalog.StratifyOutcome =
  Array.tryFill{region}(relationCount, 0) match
    case FillFailed => StratifyCycle
    case Filled(strata) =>
      var pass: U64 = 0
      var cycleFound: U64 = 0
      var maxLevel: U64 = 0
      while pass < relationCount do
        @<lessOrEqual(pass, relationCount), lessOrEqual(cycleFound, 1), decreases(relationCount - pass)>
        var edgeIndex: U64 = 0
        while edgeIndex < edgeCount do
          @<lessOrEqual(edgeIndex, edgeCount), lessOrEqual(cycleFound, 1), decreases(edgeCount - edgeIndex)>
          val headRel: U64 = edgeHead(edgeIndex)
          val bodyRel: U64 = edgeBody(edgeIndex)
          val isNeg: U8 = edgeNegated(edgeIndex)
          if headRel < relationCount && bodyRel < relationCount then
            val headLevel: U64 = strata(headRel)
            val bodyLevel: U64 = strata(bodyRel)
            if isNeg != 0 then
              if bodyLevel < relationCount then
                val needed: U64 = bodyLevel + 1
                if headLevel < needed then
                  if needed >= relationCount then
                    cycleFound = 1
                  else
                    strata(headRel) := needed
                    if needed > maxLevel then maxLevel = needed else ()
                else
                  ()
              else
                cycleFound = 1
            else
              if headLevel < bodyLevel then
                strata(headRel) := bodyLevel
                if bodyLevel > maxLevel then maxLevel = bodyLevel else ()
              else
                ()
          else
            ()
          edgeIndex = edgeIndex + 1
        pass = pass + 1
      if cycleFound == 1 then StratifyCycle else StratifiedValid(maxLevel)
