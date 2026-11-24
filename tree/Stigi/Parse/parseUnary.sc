// prompt: parse a prefix operator and its one operand token at the cursor.
def Stigi.Parse.parseUnary {cursor: Stigi.Parse.Cursor, Stigi.Mut} (tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), tokCount: U64, nodeCap: Stigi.NodeCap, nodes: Array[Stigi.Ast.Node](nodeCap)) <lessOrEqual(tokCount, tokCap)>: Stigi.Lookup =
  val pos: U64 = cursor.curPos()
  cursor.stepForward()
  val operandPos: U64 = cursor.curPos()
  if Stigi.Parse.peekKind{cursor}(tokCap, toks, tokCount) is TokEnd | TokNewline then Missing else
    cursor.stepForward()
    cursor.emitNode(nodeCap, nodes, Node(ExprUnary, pos, operandPos, 0, 0, 0))
