// prompt: place the full select of one lowered match over the arms from the arm mark to the arm fill end.
def Stigi.Elaborate.emitSelect {Stigi.Mut} (core: Stigi.Core.Tables, scrutCore: U64): Stigi.Elaborate.Placed =
  core match
    case CoreTables(expressionCapacity, exprs, expressionMark, selectIdx, listCapacity, childLists, listMark, listEnd, armCapacity, arms, armMark, endArm) =>
      if selectIdx < expressionCapacity then
        @<below(selectIdx, expressionCapacity)>
        if scrutCore < selectIdx then
          val nextCore: U64 = selectIdx + 1
          exprs(selectIdx) := SelectExpr(selectIdx, scrutCore, CoreSpan(armMark, endArm), 0)
          PlacedAt(CoreTables(expressionCapacity, exprs, expressionMark, nextCore, listCapacity, childLists, listMark, listEnd, armCapacity, arms, armMark, endArm), selectIdx)
        else PlaceInvalid
      else PlaceFull
