// prompt: parse a primary or unary expression atom into the flat AST node arena.
def Stigi.Parse.parseAtom {cursor: Stigi.Parse.Cursor, Stigi.Mut} (tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), tokCount: U64, nodeCap: Stigi.NodeCap, nodes: Array[Stigi.Ast.Node](nodeCap)) <lessOrEqual(tokCount, tokCap)>: Stigi.Lookup =
  val pos: U64 = cursor.curPos()
  val kind: Stigi.Lex.TokenKind = Stigi.Parse.peekKind{cursor}(tokCap, toks, tokCount)
  if kind == TokInt then
    cursor.stepForward()
    cursor.emitNode(nodeCap, nodes, Node(ExprInt, pos, 0, 0, 0, 0))
  else if kind == TokTrue || kind == TokFalse then
    cursor.stepForward()
    cursor.emitNode(nodeCap, nodes, Node(ExprBool, pos, 0, 0, 0, 0))
  else if kind == TokStr then
    cursor.stepForward()
    cursor.emitNode(nodeCap, nodes, Node(ExprStr, pos, 0, 0, 0, 0))
  else if kind == TokChar then
    cursor.stepForward()
    cursor.emitNode(nodeCap, nodes, Node(ExprChar, pos, 0, 0, 0, 0))
  else if kind == TokIdent || kind == TokUpper then
    cursor.stepForward()
    var endTok: U64 = if pos < tokCount then pos + 1 else tokCount
    while Stigi.Parse.peekKind{cursor}(tokCap, toks, tokCount) == TokDot do
      @<lessOrEqual(endTok, Stigi.Limit.maxTokens)>
      cursor.stepForward()
      val segKind: Stigi.Lex.TokenKind = Stigi.Parse.peekKind{cursor}(tokCap, toks, tokCount)
      if segKind == TokIdent || segKind == TokUpper then
        cursor.stepForward()
        val nowPos: U64 = cursor.curPos()
        if nowPos <= Stigi.Limit.maxTokens then endTok = nowPos else ()
      else ()
    val nextKind: Stigi.Lex.TokenKind = Stigi.Parse.peekKind{cursor}(tokCap, toks, tokCount)
    if nextKind == TokLeftParen then
      cursor.stepForward()
      var argCount: U64 = 0
      var firstArg: U64 = 0
      var depth: U64 = 1
      while depth > 0 && Stigi.Parse.peekKind{cursor}(tokCap, toks, tokCount) != TokEnd do
        @<lessOrEqual(depth, Stigi.Limit.maxTokens), lessOrEqual(argCount, Stigi.Limit.maxTokens)>
        val innerKind: Stigi.Lex.TokenKind = Stigi.Parse.peekKind{cursor}(tokCap, toks, tokCount)
        if innerKind == TokLeftParen then
          if depth < Stigi.Limit.maxTokens then depth = depth + 1 else ()
          cursor.stepForward()
        else if innerKind == TokRightParen then
          depth = depth - 1
          cursor.stepForward()
        else if innerKind == TokComma && depth == 1 then
          if argCount < Stigi.Limit.maxTokens then argCount = argCount + 1 else ()
          cursor.stepForward()
        else
          if argCount == 0 && depth == 1 then
            firstArg = cursor.curPos()
            argCount = 1
          else ()
          cursor.stepForward()
      cursor.emitNode(nodeCap, nodes, Node(ExprCall, pos, endTok, firstArg, argCount, 0))
    else
      cursor.emitNode(nodeCap, nodes, Node(ExprIdent, pos, endTok, 0, 0, 0))
  else if kind == TokBang || kind == TokMinus then
    cursor.stepForward()
    val operandPos: U64 = cursor.curPos()
    val innerKind: Stigi.Lex.TokenKind = Stigi.Parse.peekKind{cursor}(tokCap, toks, tokCount)
    if innerKind != TokEnd && innerKind != TokNewline then cursor.stepForward() else ()
    cursor.emitNode(nodeCap, nodes, Node(ExprUnary, pos, operandPos, 0, 0, 0))
  else if kind == TokLeftParen then
    cursor.stepForward()
    if Stigi.Parse.peekKind{cursor}(tokCap, toks, tokCount) == TokRightParen then
      cursor.stepForward()
      cursor.emitNode(nodeCap, nodes, Node(ExprUnit, pos, 0, 0, 0, 0))
    else
      val innerPos: U64 = cursor.curPos()
      var depth: U64 = 1
      while depth > 0 && Stigi.Parse.peekKind{cursor}(tokCap, toks, tokCount) != TokEnd do
        @<lessOrEqual(depth, Stigi.Limit.maxTokens)>
        val innerKind: Stigi.Lex.TokenKind = Stigi.Parse.peekKind{cursor}(tokCap, toks, tokCount)
        if innerKind == TokLeftParen then
          if depth < Stigi.Limit.maxTokens then depth = depth + 1 else ()
        else if innerKind == TokRightParen then
          depth = depth - 1
        else ()
        cursor.stepForward()
      cursor.emitNode(nodeCap, nodes, Node(ExprBlock, pos, innerPos, cursor.curPos(), 0, 0))
  else
    cursor.markFailed()
    Missing
