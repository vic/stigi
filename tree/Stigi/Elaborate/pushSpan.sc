// prompt: push every syntax child listed in a span onto the elaboration work stack.
def Stigi.Elaborate.pushSpan {Stigi.Mut} (tables: Stigi.Syntax.Tables, span: Stigi.Syntax.Span, workCap: U64, workNode: Array[U64](workCap), workPhase: Array[U8](workCap), topStart: U64) <lessOrEqual(topStart, workCap)>: Stigi.Elaborate.PushResult =
  val Tables(_, _, _, synLists, _, _) = tables
  span match
    case Span(first, endPos) =>
      var pos: U64 = first
      var curTop: U64 = topStart
      var fullFlag: Bool = false
      while pos < endPos && !fullFlag do
        @<lessOrEqual(first, pos), lessOrEqual(pos, endPos), lessOrEqual(curTop, workCap)>
        val nextPos: U64 = pos + 1
        if curTop < workCap then
          val advTop: U64 = curTop + 1
          workNode(curTop) := synLists(pos)
          workPhase(curTop) := U8.ofU64(0)
          curTop = advTop
        else
          fullFlag = true
        pos = nextPos
      if fullFlag then PushFull else PushDone(curTop)
