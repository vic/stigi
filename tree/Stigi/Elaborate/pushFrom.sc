// prompt: push an enter item for every syntax child of a span from one position to its end, the decreasing count bounding the recursion.
def Stigi.Elaborate.pushFrom {Stigi.Mut} (tables: Stigi.Syntax.Tables, span: Stigi.Syntax.Span, position: U64, scratch: Stigi.Elaborate.Scratch, remaining: U64) <decreases(remaining)>: Stigi.Elaborate.PushResult =
  span match
    case Span(_, endPos) =>
      if remaining < 1 || endPos <= position then PushDone(scratch) else
        @<below(position, Stigi.Limit.maxUnitNodes)>
        val Tables(_, _, _, synLists, _, _) = tables
        val child: Stigi.Syntax.NodeAt = synLists(position)
        Stigi.Elaborate.pushWork(scratch, Enter(child)) match
          case PushFull => PushFull
          case PushDone(pushed) => Stigi.Elaborate.pushFrom(tables, span, position + 1, pushed, remaining - 1)
