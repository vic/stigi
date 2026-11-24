// prompt: read one slot of the node table, failing when the slot does not hold its own index.
def Stigi.Syntax.readSlot(tables: Stigi.Syntax.Tables, slot: Stigi.Syntax.NodeAt) <below(slot, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Fetch =
  val Tables(_, _, nodes, _, _, _) = tables
  val node: Stigi.Syntax.Node = nodes(slot)
  if Stigi.Syntax.atOf(node) == slot then Fetched(node) else MalformedTable(slot)
