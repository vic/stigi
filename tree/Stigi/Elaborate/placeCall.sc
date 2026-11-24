// prompt: copy the mapped core indexes of a syntax argument span, after an optional leading argument, onto the end of the child list and place one call expression over them.
def Stigi.Elaborate.placeCall {Stigi.Mut} (tables: Stigi.Syntax.Tables, argsSpan: Stigi.Syntax.Span, core: Stigi.Core.Tables, scratch: Stigi.Elaborate.Scratch, callee: Stigi.Core.Reference, leading: Stigi.Elaborate.Leading): Stigi.Elaborate.Placed =
  val Tables(_, _, _, synLists, _, _) = tables
  scratch match
    case WorkScratch(capacity, _, _, mapped, _) =>
      core match
        case CoreTables(expressionCapacity, exprs, expressionMark, emitIdx, listCapacity, childLists, listMark, listStart, armCapacity, arms, armMark, armEnd) =>
          var curList: U64 = listStart
          var copy: Stigi.Elaborate.ArgumentCopy = ArgumentsCopying
          leading match
            case WithoutLeading => ()
            case LeadingArgument(leadAt) =>
              if curList < listCapacity then
                childLists(curList) := leadAt
                curList = curList + 1
              else
                copy = ArgumentsFull
          argsSpan match
            case Span(first, endPos) =>
              var pos: U64 = first
              while pos < endPos && copy is ArgumentsCopying do
                @<lessOrEqual(first, pos), lessOrEqual(listStart, curList), lessOrEqual(curList, listCapacity)>
                val nextPos: U64 = pos + 1
                val argSyn: U64 = synLists(pos)
                if argSyn < capacity then
                  if curList < listCapacity then
                    val advList: U64 = curList + 1
                    childLists(curList) := mapped(argSyn)
                    curList = advList
                  else
                    copy = ArgumentsFull
                else
                  copy = ArgumentsBad
                pos = nextPos
              copy match
                case ArgumentsFull => PlaceFull
                case ArgumentsBad => PlaceInvalid
                case ArgumentsCopying =>
                  if emitIdx < expressionCapacity then
                    val nextCore: U64 = emitIdx + 1
                    exprs(emitIdx) := CallExpr(emitIdx, callee, CoreSpan(listStart, curList), NormalCall, 0)
                    PlacedAt(CoreTables(expressionCapacity, exprs, expressionMark, nextCore, listCapacity, childLists, listMark, curList, armCapacity, arms, armMark, armEnd), emitIdx)
                  else PlaceFull
