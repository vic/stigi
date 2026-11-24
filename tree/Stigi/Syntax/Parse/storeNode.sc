// prompt: write one node into one entry of the node table.
def Stigi.Syntax.Parse.storeNode {Stigi.Mut} (nodes: Array[Stigi.Syntax.Node](Stigi.Limit.maxUnitNodes), at: Stigi.Syntax.NodeAt, node: Stigi.Syntax.Node) <below(at, Stigi.Limit.maxUnitNodes)>: Unit =
  nodes(at) := node
