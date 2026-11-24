// prompt: place the tail select over the arms from the arm mark and the choice node of one guarded arm.
def Stigi.Elaborate.emitGuardPair {Stigi.Mut} (core: Stigi.Core.Tables, scrutCore: U64, guardCore: U64, bodyCore: U64): Stigi.Elaborate.Placed =
  core match
    case CoreTables(expressionCapacity, exprs, expressionMark, coreAt, listCapacity, childLists, listMark, listEnd, armCapacity, arms, armMark, armEnd) =>
      if coreAt < expressionCapacity then
        val choiceAt: U64 = coreAt + 1
        if choiceAt < expressionCapacity then
          @<below(choiceAt, expressionCapacity)>
          if scrutCore < coreAt && guardCore < coreAt && bodyCore < coreAt then
            val afterChoice: U64 = choiceAt + 1
            exprs(coreAt) := SelectExpr(coreAt, scrutCore, CoreSpan(armMark, armEnd), 0)
            exprs(choiceAt) := ChoiceExpr(choiceAt, guardCore, bodyCore, coreAt, 0)
            PlacedAt(CoreTables(expressionCapacity, exprs, expressionMark, afterChoice, listCapacity, childLists, listMark, listEnd, armCapacity, arms, armMark, armEnd), choiceAt)
          else PlaceInvalid
        else PlaceFull
      else PlaceFull
