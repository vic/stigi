// prompt: push VisitNode work items for every syntax node in a span in forward or reverse order.
def Stigi.Store.pushVisitSpan {Stigi.Mut} (synTables: Stigi.Syntax.Tables, state: Stigi.Store.LengthUnitState, span: Stigi.Syntax.Span, reverseOrder: Bool): Stigi.Store.LengthUnitState =
  val Tables(_, _, _, synLists, _, _) = synTables
  span match
    case Span(first, endPos) =>
      var current: Stigi.Store.LengthUnitState = state
      if reverseOrder then
        var pos: U64 = endPos
        while first < pos do
          @<lessOrEqual(pos, endPos)>
          pos = pos - 1
          current = Stigi.Store.pushVisitWork(current, VisitNode(synLists(pos)))
      else
        var pos: U64 = first
        while pos < endPos do
          @<lessOrEqual(first, pos), lessOrEqual(pos, endPos)>
          current = Stigi.Store.pushVisitWork(current, VisitNode(synLists(pos)))
          pos = pos + 1
      current
