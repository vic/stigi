// prompt: the parenthesizing shape of the node at one slot; a malformed slot is plain and fails when it is printed.
def Stigi.Syntax.Print.shapeOf(tables: Stigi.Syntax.Tables, node: Stigi.Syntax.NodeAt) <below(node, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Print.Shape =
  Stigi.Syntax.readSlot(tables, node) match
    case Fetched(found) => Stigi.Syntax.Print.shapeOfNode(found)
    case MalformedTable(_) => ShapePlain
