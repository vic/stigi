// prompt: parse a binary and pipe expression sequence with Pratt precedence into AST nodes.
def Stigi.Parse.parseExpr {cursor: Stigi.Parse.Cursor, Stigi.Mut} (tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), tokCount: U64, nodeCap: Stigi.NodeCap, nodes: Array[Stigi.Ast.Node](nodeCap)) <lessOrEqual(tokCount, tokCap)>: Stigi.Lookup =
  val firstKind: Stigi.Lex.TokenKind = Stigi.Parse.peekKind{cursor}(tokCap, toks, tokCount)
  if firstKind == TokIf then
    val ifPos: U64 = cursor.curPos()
    cursor.stepForward()
    val condNode: Stigi.Lookup = Stigi.Parse.parseAtom{cursor}(tokCap, toks, tokCount, nodeCap, nodes)
    val Found(condIdx) = condNode else
      case Missing => Missing
    cursor.emitNode(nodeCap, nodes, Node(ExprIf, ifPos, condIdx, cursor.curPos(), 0, 0))
  else if firstKind == TokWhile then
    val whilePos: U64 = cursor.curPos()
    cursor.stepForward()
    val condNode: Stigi.Lookup = Stigi.Parse.parseAtom{cursor}(tokCap, toks, tokCount, nodeCap, nodes)
    val Found(condIdx) = condNode else
      case Missing => Missing
    cursor.emitNode(nodeCap, nodes, Node(ExprWhile, whilePos, condIdx, cursor.curPos(), 0, 0))
  else if firstKind == TokVal || firstKind == TokVar then
    val bindPos: U64 = cursor.curPos()
    cursor.stepForward()
    val bindKind: Stigi.Ast.ExprKind = if firstKind == TokVal then ExprVal else ExprVar
    val namePos: U64 = cursor.curPos()
    if Stigi.Parse.peekKind{cursor}(tokCap, toks, tokCount) != TokEnd then cursor.stepForward() else ()
    cursor.emitNode(nodeCap, nodes, Node(bindKind, bindPos, namePos, cursor.curPos(), 0, 0))
  else if firstKind == TokHandle then
    val handlePos: U64 = cursor.curPos()
    cursor.stepForward()
    cursor.emitNode(nodeCap, nodes, Node(ExprHandle, handlePos, cursor.curPos(), 0, 0, 0))
  else
    val Found(leftInit) = Stigi.Parse.parseAtom{cursor}(tokCap, toks, tokCount, nodeCap, nodes) else
      case Missing => Missing
    var lhsNode: U64 = leftInit
    var keepGoing: Bool = true
    var steps: U64 = 0
    while keepGoing && steps < tokCount do
      @<lessOrEqual(steps, tokCount)>
      val operKind: Stigi.Lex.TokenKind = Stigi.Parse.peekKind{cursor}(tokCap, toks, tokCount)
      val prec: U64 = Stigi.Parse.binPrecedence(operKind)
      if prec == 0 then
        keepGoing = false
      else
        val operPos: U64 = cursor.curPos()
        cursor.stepForward()
        val rhsRes: Stigi.Lookup = Stigi.Parse.parseAtom{cursor}(tokCap, toks, tokCount, nodeCap, nodes)
        rhsRes match
          case Found(rhsNode) =>
            val nodeKind: Stigi.Ast.ExprKind = if operKind == TokPipe then ExprPipe else ExprBin
            val combRes: Stigi.Lookup = cursor.emitNode(nodeCap, nodes, Node(nodeKind, operPos, lhsNode, rhsNode, prec, 0))
            combRes match
              case Found(combNode) => lhsNode = combNode
              case Missing => keepGoing = false
          case Missing =>
            keepGoing = false
      steps = steps + 1
    if cursor.hasFailed() then Missing else Found(lhsNode)
