// prompt: parse a parenthesized parameter or field list and append FieldInfo records.
def Stigi.Emit.parseParamList {Stigi.Mut} (tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), startTok: U64, endTok: U64, fieldKind: Stigi.Emit.FieldKind, fieldCap: Stigi.NodeCap, fields: Array[Stigi.Emit.FieldInfo](fieldCap), initCount: U64) <lessOrEqual(startTok, endTok), lessOrEqual(endTok, tokCap), lessOrEqual(initCount, fieldCap)>: Stigi.Emit.IndexCounts =
  var pos: U64 = startTok
  var count: U64 = initCount
  if pos < endTok then
    val Token(openKind, _, _, _, _) = toks(pos)
    if openKind == TokLeftParen then
      pos = pos + 1
      var done: Bool = false
      while pos < endTok && !done do
        @<lessOrEqual(pos, endTok), lessOrEqual(count, fieldCap)>
        val Token(curKind, nameBeg, nameFin, _, _) = toks(pos)
        if curKind == TokRightParen then
          pos = pos + 1
          done = true
        else if curKind == TokComma || curKind == TokNewline then
          pos = pos + 1
        else if pos + 2 < endTok then
          val colonPos: U64 = pos + 1
          val Token(colonKind, _, _, _, _) = toks(colonPos)
          if colonKind == TokColon then
            val TypeSpan(typeBeg, typeFin, elemBeg, elemFin, afterType) = Stigi.Emit.parseTypeSpan(tokCap, toks, colonPos + 1, endTok)
            if count < fieldCap then
              fields(count) := FieldInfo(fieldKind, nameBeg, nameFin, typeBeg, typeFin, elemBeg, elemFin)
              count = count + 1
            else ()
            pos = if afterType > pos && afterType <= endTok then afterType else pos + 1
          else
            pos = pos + 1
        else
          pos = pos + 1
    else ()
  else ()
  IndexCounts(0, count, pos)
