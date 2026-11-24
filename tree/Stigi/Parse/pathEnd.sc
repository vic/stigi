// prompt: the token index after the last dot and name segment of a path.
def Stigi.Parse.pathEnd {cursor: Stigi.Parse.Cursor} (tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), tokCount: U64, endTok: U64, remaining: U64) <lessOrEqual(tokCount, tokCap), decreases(remaining)>: U64 =
  if remaining > 0 && Stigi.Parse.takeKind{cursor}(tokCap, toks, tokCount, TokDot) then
    if Stigi.Parse.peekKind{cursor}(tokCap, toks, tokCount) is TokIdent | TokUpper then
      cursor.stepForward()
      Stigi.Parse.pathEnd{cursor}(tokCap, toks, tokCount, cursor.curPos(), remaining - 1)
    else endTok
  else endTok
