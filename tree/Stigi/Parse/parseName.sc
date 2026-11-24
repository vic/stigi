// prompt: parse a dotted unit name at the cursor into its byte span.
def Stigi.Parse.parseName {cursor: Stigi.Parse.Cursor} (tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), tokCount: U64) <lessOrEqual(tokCount, tokCap)>: Stigi.Parse.NameSpan =
  val namePos: U64 = cursor.curPos()
  if namePos < tokCount then
    val Token(kind, nameStart, nameEnd, _, _) = toks(namePos)
    if kind is TokIdent | TokUpper then
      cursor.stepForward()
      Stigi.Parse.nameTail{cursor}(tokCap, toks, tokCount, nameStart, nameEnd, tokCount)
    else Unnamed
  else Unnamed
