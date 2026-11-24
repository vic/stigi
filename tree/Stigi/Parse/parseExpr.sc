// prompt: parse one L0 expression at the cursor into the flat AST node arena.
def Stigi.Parse.parseExpr {cursor: Stigi.Parse.Cursor, Stigi.Mut} (tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), tokCount: U64, nodeCap: Stigi.NodeCap, nodes: Array[Stigi.Ast.Node](nodeCap)) <lessOrEqual(tokCount, tokCap)>: Stigi.Lookup =
  val pos: U64 = cursor.curPos()
  Stigi.Parse.leadOfExpr(Stigi.Parse.peekKind{cursor}(tokCap, toks, tokCount)) match
    case LeadHead(kind) =>
      cursor.stepForward()
      val Found(condIdx) = Stigi.Parse.parseAtom{cursor}(tokCap, toks, tokCount, nodeCap, nodes) else
        case Missing => Missing
      cursor.emitNode(nodeCap, nodes, Node(kind, pos, condIdx, cursor.curPos(), 0, 0))
    case LeadBind(kind) =>
      cursor.stepForward()
      val namePos: U64 = cursor.curPos()
      if Stigi.Parse.peekKind{cursor}(tokCap, toks, tokCount) is TokEnd then Missing else
        cursor.stepForward()
        cursor.emitNode(nodeCap, nodes, Node(kind, pos, namePos, cursor.curPos(), 0, 0))
    case LeadHandle =>
      cursor.stepForward()
      cursor.emitNode(nodeCap, nodes, Node(ExprHandle, pos, cursor.curPos(), 0, 0, 0))
    case LeadOperand =>
      val Found(lhs) = Stigi.Parse.parseAtom{cursor}(tokCap, toks, tokCount, nodeCap, nodes) else
        case Missing => Missing
      Stigi.Parse.parseBinary{cursor}(tokCap, toks, tokCount, nodeCap, nodes, lhs, tokCount)
