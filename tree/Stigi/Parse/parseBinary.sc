// prompt: fold the binary and pipe operators after a left operand into AST nodes.
def Stigi.Parse.parseBinary {cursor: Stigi.Parse.Cursor, Stigi.Mut} (tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), tokCount: U64, nodeCap: Stigi.NodeCap, nodes: Array[Stigi.Ast.Node](nodeCap), lhs: U64, remaining: U64) <lessOrEqual(tokCount, tokCap), decreases(remaining)>: Stigi.Lookup =
  val operKind: Stigi.Lex.TokenKind = Stigi.Parse.peekKind{cursor}(tokCap, toks, tokCount)
  val prec: U64 = Stigi.Parse.binPrecedence(operKind)
  if remaining < 1 || prec < 1 then Found(lhs) else
    val operPos: U64 = cursor.curPos()
    cursor.stepForward()
    val Found(rhs) = Stigi.Parse.parseAtom{cursor}(tokCap, toks, tokCount, nodeCap, nodes) else
      case Missing => Missing
    val nodeKind: Stigi.Ast.ExprKind = if operKind is TokPipe then ExprPipe else ExprBin
    val Found(comb) = cursor.emitNode(nodeCap, nodes, Node(nodeKind, operPos, lhs, rhs, prec, 0)) else
      case Missing => Missing
    Stigi.Parse.parseBinary{cursor}(tokCap, toks, tokCount, nodeCap, nodes, comb, remaining - 1)
