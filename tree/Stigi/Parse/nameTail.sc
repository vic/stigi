// prompt: extend a name span over every following dot and name segment.
def Stigi.Parse.nameTail {cursor: Stigi.Parse.Cursor} (tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), tokCount: U64, nameStart: U64, nameEnd: U64, remaining: U64) <lessOrEqual(tokCount, tokCap), lessOrEqual(nameStart, nameEnd), decreases(remaining)>: Stigi.Parse.NameSpan =
  if remaining > 0 && Stigi.Parse.takeKind{cursor}(tokCap, toks, tokCount, TokDot) then
    val segPos: U64 = cursor.curPos()
    if segPos < tokCount then
      val Token(segKind, _, segEnd, _, _) = toks(segPos)
      if segKind is TokIdent | TokUpper && nameStart <= segEnd then
        cursor.stepForward()
        Stigi.Parse.nameTail{cursor}(tokCap, toks, tokCount, nameStart, segEnd, remaining - 1)
      else Unnamed
    else Unnamed
  else Named(nameStart, nameEnd)
