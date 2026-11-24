// prompt: resolve lowered LocalExpr references in a Checked-IR slice to local binder slots or global val unit slots.
def Stigi.Store.resolveLengthRefs {Stigi.Mut} (synTables: Stigi.Syntax.Tables, image: Stigi.Names.Image, unitIdx: U64, scratch: Stigi.Store.LengthScratch, localCount: U64, exprCount: U64) <below(unitIdx, Stigi.Limit.maxUnits), lessOrEqual(localCount, 256), lessOrEqual(exprCount, Stigi.Limit.maxUnitNodes)>: Unit =
  val Tables(synSource, _, _, _, _, _) = synTables
  scratch match
    case LengthScratch(_, _, _, CoreTables(exprCap, exprs, _, _, _, _, _, _, _, _, _, _), _, _, _, localStarts, localEnds, _, _, _, _, _, _, _) =>
      var pos: U64 = 0
      while pos < exprCount && pos < exprCap do
        @<lessOrEqual(pos, exprCount)>
        exprs(pos) match
          case LocalExpr(at, refFirst, typeSlot) =>
            if refFirst < Stigi.Limit.maxUnitBytes then
              val refEnd: U64 = Stigi.Store.findRefEnd(synSource, refFirst)
              var foundLocal: Bool = false
              var foundSlot: U64 = 0
              var idx: U64 = localCount
              while 0 < idx && !foundLocal do
                @<lessOrEqual(idx, localCount)>
                val checkIdx: U64 = idx - 1
                val leftStart: U64 = localStarts(checkIdx)
                val leftEnd: U64 = localEnds(checkIdx)
                if leftStart <= leftEnd && leftEnd <= Stigi.Limit.maxUnitBytes then
                  if Stigi.Store.sameSourceSlice(synSource, refFirst, refEnd, leftStart, leftEnd) then
                    foundLocal = true
                    foundSlot = checkIdx
                  else ()
                else ()
                idx = checkIdx
              if foundLocal then
                exprs(pos) := LocalExpr(at, foundSlot, typeSlot)
              else
                Stigi.Names.searchName(image, synTables, unitIdx, refFirst, refEnd) match
                  case UnitFound(targetUnit) => exprs(pos) := GlobalExpr(at, ExternalRef(targetUnit, 0), typeSlot)
                  case UnitAbsent => exprs(pos) := LocalExpr(at, 256, typeSlot)
            else exprs(pos) := LocalExpr(at, 256, typeSlot)
          case GlobalExpr(_, _, _) | IntExpr(_, _, _, _) | TruthExpr(_, _) | NothingExpr(_) | TextExpr(_, _, _) | ConstructExpr(_, _, _, _, _) | CallExpr(_, _, _, _, _) | OperCallExpr(_, _, _, _, _) | ExactExpr(_, _, _, _, _) | CompareExpr(_, _, _, _) | BothExpr(_, _, _) | EitherExpr(_, _, _) | NegateExpr(_, _) | ChoiceExpr(_, _, _, _, _) | LoopExpr(_, _, _, _, _) | SelectExpr(_, _, _, _) | BindExpr(_, _, _, _, _) | MutableExpr(_, _, _, _, _, _) | AssignExpr(_, _, _) | ElemReadExpr(_, _, _, _) | ElemWriteExpr(_, _, _, _) | HandleExpr(_, _, _, _, _, _) | AssertExpr(_, _, _, _) | AssertValExpr(_, _, _, _) | DenyExpr(_, _, _, _) => ()
        pos = pos + 1
