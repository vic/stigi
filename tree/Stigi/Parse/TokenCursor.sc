// prompt: stack-allocated parser cursor handler implementing Stigi.Parse.Cursor.
handler Stigi.Parse.TokenCursor(tokLimit: Stigi.TokenCap, nodeLimit: Stigi.NodeCap) for Stigi.Parse.Cursor:
  var tokPos: U64 = 0
  var nodeUsed: U64 = 0
  def curPos(): U64 = tokPos
  def stepForward(): Unit =
    val curTok: U64 = tokPos
    if curTok < tokLimit then tokPos = curTok + 1 else ()
  def emitNode {Stigi.Mut} (nodeCap: Stigi.NodeCap, nodes: Array[Stigi.Ast.Node](nodeCap), node: Stigi.Ast.Node): Stigi.Lookup =
    val slot: U64 = nodeUsed
    if slot < nodeCap && slot < nodeLimit then
      nodes(slot) := node
      nodeUsed = slot + 1
      Found(slot)
    else Missing
  def usedNodes(): U64 = nodeUsed
