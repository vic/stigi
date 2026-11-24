// prompt: write one node into its claimed entry of the node table and move the free entry past it.
def Stigi.Syntax.Parse.place {Stigi.Mut} (tables: Stigi.Syntax.Tables, at: Stigi.Syntax.NodeAt, node: Stigi.Syntax.Node, after: Stigi.Syntax.Parse.Progress) <below(at, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Built =
  val Tables(_, _, nodes, _, _, _) = tables
  val Progress(token, _, lists, pending) = after
  Stigi.Syntax.Parse.storeNode(nodes, at, node)
  Built(at, Progress(token, at + 1, lists, pending))
