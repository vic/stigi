// prompt: whether the slot holds an absent node.
def Stigi.Syntax.Print.isAbsent(tables: Stigi.Syntax.Tables, node: Stigi.Syntax.NodeAt) <below(node, Stigi.Limit.maxUnitNodes)>: Bool =
  Stigi.Syntax.readSlot(tables, node) is Fetched(Absent(_))
