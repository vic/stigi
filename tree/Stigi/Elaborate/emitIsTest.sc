// prompt: place the truth, rest arm and select nodes of one lowered is-test whose pattern arms run from the arm mark and give back the index of its select.
def Stigi.Elaborate.emitIsTest {Stigi.Mut} (core: Stigi.Core.Tables, subjCore: U64): Stigi.Elaborate.Placed =
  core match
    case CoreTables(expressionCapacity, exprs, expressionMark, emitIdx, listCapacity, childLists, listMark, listEnd, armCapacity, arms, armMark, endArm) =>
      if emitIdx < expressionCapacity then
        @<below(emitIdx, expressionCapacity)>
        val nextCore: U64 = emitIdx + 1
        if nextCore < expressionCapacity then
          if subjCore < emitIdx then
            if armMark < endArm && arms(endArm - 1) is RestArm(_) then
              val selectIdx: U64 = nextCore + 1
              exprs(emitIdx) := TruthExpr(emitIdx, true)
              exprs(nextCore) := SelectExpr(nextCore, subjCore, CoreSpan(armMark, endArm), 0)
              PlacedAt(CoreTables(expressionCapacity, exprs, expressionMark, selectIdx, listCapacity, childLists, listMark, listEnd, armCapacity, arms, armMark, endArm), nextCore)
            else if endArm < armCapacity then
              val armEnd: U64 = endArm + 1
              val selectIdx: U64 = nextCore + 1
              if selectIdx < expressionCapacity then
                val afterSelect: U64 = selectIdx + 1
                arms(endArm) := RestArm(nextCore)
                exprs(emitIdx) := TruthExpr(emitIdx, true)
                exprs(nextCore) := TruthExpr(nextCore, false)
                exprs(selectIdx) := SelectExpr(selectIdx, subjCore, CoreSpan(armMark, armEnd), 0)
                PlacedAt(CoreTables(expressionCapacity, exprs, expressionMark, afterSelect, listCapacity, childLists, listMark, listEnd, armCapacity, arms, armMark, armEnd), selectIdx)
              else PlaceFull
            else PlaceFull
          else PlaceInvalid
        else PlaceFull
      else PlaceFull
