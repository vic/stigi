// prompt: find matching then and else token positions for an if expression, skipping val-else clauses.
def Stigi.Emit.findIfParts(tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), startTok: U64, endTok: U64) <lessOrEqual(startTok, endTok), lessOrEqual(endTok, tokCap)>: Stigi.Emit.IfSplit =
  if startTok >= endTok then IfIncomplete
  else
    var pos: U64 = startTok + 1
    var nestedIf: U64 = 0
    var foundThen: Bool = false
    var thenPos: U64 = startTok
    var foundElse: Bool = false
    var elsePos: U64 = startTok
    while pos < endTok && !foundElse do
      @<lessOrEqual(startTok, pos), lessOrEqual(pos, endTok), lessOrEqual(startTok, thenPos), lessOrEqual(thenPos, pos), lessOrEqual(nestedIf, Stigi.Limit.maxTokens)>
      val Token(kind, _, _, _, _) = toks(pos)
      if kind == TokIf && nestedIf < Stigi.Limit.maxTokens then
        nestedIf = nestedIf + 1
      else if kind == TokThen && nestedIf == 0 && !foundThen then
        foundThen = true
        thenPos = pos
      else if kind == TokElse then
        var peekPos: U64 = pos + 1
        var peekDone: Bool = false
        var isValElse: Bool = false
        while peekPos < endTok && !peekDone do
          @<lessOrEqual(pos, peekPos), lessOrEqual(peekPos, endTok)>
          val Token(peekKind, _, _, _, _) = toks(peekPos)
          if peekKind == TokNewline then peekPos = peekPos + 1
          else
            if peekKind == TokCase then isValElse = true else ()
            peekDone = true
        if !isValElse then
          if nestedIf > 0 then nestedIf = nestedIf - 1
          else if foundThen then
            foundElse = true
            elsePos = pos
          else ()
        else ()
      else ()
      pos = pos + 1
    if foundThen && foundElse && thenPos <= elsePos then IfParts(thenPos, elsePos)
    else IfIncomplete
