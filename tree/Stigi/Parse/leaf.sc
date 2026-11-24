// prompt: emit a one-token leaf node of an expression kind at the cursor.
def Stigi.Parse.leaf {cursor: Stigi.Parse.Cursor, Stigi.Mut} (nodeCap: Stigi.NodeCap, nodes: Array[Stigi.Ast.Node](nodeCap), kind: Stigi.Ast.ExprKind): Stigi.Lookup =
  val pos: U64 = cursor.curPos()
  cursor.stepForward()
  cursor.emitNode(nodeCap, nodes, Node(kind, pos, 0, 0, 0, 0))
