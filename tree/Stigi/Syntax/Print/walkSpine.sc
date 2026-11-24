// prompt: walk an expression left spine iteratively to its leaf, opening left parentheses along the way.
def Stigi.Syntax.Print.walkSpine {Stigi.Mut} (tables: Stigi.Syntax.Tables, node: Stigi.Syntax.NodeAt, pen: Stigi.Syntax.Print.Pen) <below(node, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Print.SpineResult =
  Stigi.Syntax.Print.leftChild(tables, node) match
    case SpineLeaf => SpineBad(PenFailed(MalformedNode(node)))
    case BareAssert(facts) => SpineBare(facts)
    case SpineNext(firstChild, firstWrap) =>
      var total: U64 = 1
      var cursor: U64 = firstChild
      var walking: Bool = true
      var parenPen: Stigi.Syntax.Print.Pen = if firstWrap then Stigi.Syntax.Print.putLiteral(tables, pen, 1, "(") else pen
      while total < Stigi.Limit.maxUnitNodes && walking && parenPen is Pen(_) do
        @<lessOrEqual(total, Stigi.Limit.maxUnitNodes)>
        if cursor < Stigi.Limit.maxUnitNodes then
          Stigi.Syntax.Print.leftChild(tables, cursor) match
            case SpineLeaf | BareAssert(_) => walking = false
            case SpineNext(child, wrapParen) =>
              parenPen = if wrapParen then Stigi.Syntax.Print.putLiteral(tables, parenPen, 1, "(") else parenPen
              cursor = child
              total = total + 1
        else parenPen = PenFailed(MalformedNode(node))
      if !(parenPen is Pen(_)) then SpineBad(parenPen) else if walking || !(cursor < Stigi.Limit.maxUnitNodes) then SpineBad(PenFailed(MalformedNode(node))) else SpineChain(cursor, total, parenPen)
