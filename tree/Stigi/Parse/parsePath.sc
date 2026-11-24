// prompt: parse a dotted path at the cursor as an identifier or a call with its arguments.
def Stigi.Parse.parsePath {cursor: Stigi.Parse.Cursor, Stigi.Mut} (tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), tokCount: U64, nodeCap: Stigi.NodeCap, nodes: Array[Stigi.Ast.Node](nodeCap)) <lessOrEqual(tokCount, tokCap)>: Stigi.Lookup =
  val pos: U64 = cursor.curPos()
  cursor.stepForward()
  val endTok: U64 = Stigi.Parse.pathEnd{cursor}(tokCap, toks, tokCount, cursor.curPos(), tokCount)
  if Stigi.Parse.takeKind{cursor}(tokCap, toks, tokCount, TokLeftParen) then
    Stigi.Parse.scanArgs{cursor}(tokCap, toks, tokCount, ArgState(1, 0, 0), tokCount) match
      case ArgsClosed(firstArg, argCount) => cursor.emitNode(nodeCap, nodes, Node(ExprCall, pos, endTok, firstArg, argCount, 0))
      case ArgsOpen => Missing
  else cursor.emitNode(nodeCap, nodes, Node(ExprIdent, pos, endTok, 0, 0, 0))
