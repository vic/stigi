// prompt: peek the token kind at the parser cursor position without advancing.
def Stigi.Parse.peekKind {cursor: Stigi.Parse.Cursor} (tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), tokCount: U64) <lessOrEqual(tokCount, tokCap)>: Stigi.Lex.TokenKind =
  val pos: U64 = cursor.curPos()
  if pos < tokCount then
    val Token(kind, _, _, _, _) = toks(pos)
    kind
  else TokEnd
