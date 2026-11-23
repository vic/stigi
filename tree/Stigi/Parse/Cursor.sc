// prompt: parser cursor capability tracking token position and AST arena allocation.
capability Stigi.Parse.Cursor:
  def curPos(): U64
  def stepForward(): Unit
  def emitNode {Stigi.Mut} (nodeCap: Stigi.NodeCap, nodes: Array[Stigi.Ast.Node](nodeCap), node: Stigi.Ast.Node): Stigi.Lookup
  def usedNodes(): U64
  def markFailed(): Unit
  def hasFailed(): Bool
