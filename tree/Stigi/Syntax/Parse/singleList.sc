// prompt: gather one node into a list of one item in the list table.
def Stigi.Syntax.Parse.singleList {Stigi.Mut} (tables: Stigi.Syntax.Tables, item: Stigi.Syntax.NodeAt, progress: Stigi.Syntax.Parse.Progress) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Listed =
  val Progress(token, nodes, lists, pending) = progress
  val Tables(_, _, _, _, pendingTable, _) = tables
  if Stigi.Limit.maxUnitNodes <= pending then ListFailed(UnitTooLarge, token) else
    Stigi.Syntax.Parse.storePending(pendingTable, pending, item)
    Stigi.Syntax.Parse.closeList(tables, pending, Progress(token, nodes, lists, pending + 1))
