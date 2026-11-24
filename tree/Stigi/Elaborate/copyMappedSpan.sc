// prompt: copy mapped core child indexes for a syntax argument span into the Checked-IR child list.
def Stigi.Elaborate.copyMappedSpan {Stigi.Mut} (tables: Stigi.Syntax.Tables, span: Stigi.Syntax.Span, listCap: U64, childLists: Array[U64](listCap), listStart: U64, mapCap: U64, mapped: Array[U64](mapCap)) <lessOrEqual(listStart, listCap)>: Stigi.Elaborate.EmitResult =
  val Tables(_, _, _, synLists, _, _) = tables
  span match
    case Span(first, endPos) =>
      var pos: U64 = first
      var curList: U64 = listStart
      var fullFlag: Bool = false
      while pos < endPos && !fullFlag do
        @<lessOrEqual(first, pos), lessOrEqual(pos, endPos), lessOrEqual(listStart, curList), lessOrEqual(curList, listCap)>
        val nextPos: U64 = pos + 1
        val argSyn: U64 = synLists(pos)
        if argSyn < mapCap && curList < listCap then
          val advList: U64 = curList + 1
          childLists(curList) := mapped(argSyn)
          curList = advList
        else
          fullFlag = true
        pos = nextPos
      if fullFlag then EmitFull else EmitDone(0, curList, 0)
