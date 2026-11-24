// prompt: move the gathered pending items of a list into the list table and give their span.
def Stigi.Syntax.Parse.closeList {Stigi.Mut} (tables: Stigi.Syntax.Tables, base: U64, gathered: Stigi.Syntax.Parse.Progress) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Listed =
  val Progress(token, nodes, lists, pending) = gathered
  val Tables(_, _, _, listTable, pendingTable, _) = tables
  if pending < base || Stigi.Limit.maxUnitNodes < pending || Stigi.Limit.maxUnitNodes < lists then ListFailed(UnitTooLarge, token) else
    val end: U64 = Stigi.Array.copy[Stigi.Syntax.NodeAt](Stigi.Limit.maxUnitNodes, pendingTable, base, pending, Stigi.Limit.maxUnitNodes, listTable, lists)
    if end - lists < pending - base then ListFailed(UnitTooLarge, token) else Listed(Span(lists, end), Progress(token, nodes, end, base))
