// prompt: place an integer literal core expression for an inferred byte-literal length with zero IR change.
def Stigi.Elaborate.emitInferredLength {Stigi.Mut} (core: Stigi.Core.Tables, byteCount: U64) <lessOrEqual(byteCount, Stigi.Limit.maxUnitBytes)>: Stigi.Elaborate.Placed =
  core match
    case CoreTables(expressionCapacity, exprs, expressionMark, emitIdx, listCapacity, childLists, listMark, listEnd, armCapacity, arms, armMark, armEnd) =>
      if emitIdx < expressionCapacity then
        val nextCore: U64 = emitIdx + 1
        exprs(emitIdx) := IntExpr(emitIdx, byteCount, 0, 0)
        PlacedAt(CoreTables(expressionCapacity, exprs, expressionMark, nextCore, listCapacity, childLists, listMark, listEnd, armCapacity, arms, armMark, armEnd), emitIdx)
      else PlaceFull
