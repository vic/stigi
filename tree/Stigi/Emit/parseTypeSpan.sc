// prompt: parse a Stigi type expression (primitive, qualified enum/cap, or Array[T](len)) from tokens.
def Stigi.Emit.parseTypeSpan(tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), startTok: U64, endTok: U64) <lessOrEqual(startTok, endTok), lessOrEqual(endTok, tokCap)>: Stigi.Emit.TypeSpan =
  var pos: U64 = startTok
  if pos >= endTok then TypeSpan(0, 0, 0, 0, pos) else
    val Token(_, firstBeg, firstFin, _, _) = toks(pos)
    var typeEnd: U64 = firstFin
    pos = pos + 1
    var dotGuard: U64 = 0
    while pos < endTok && dotGuard < 16 do
      @<lessOrEqual(pos, endTok), lessOrEqual(firstBeg, typeEnd), lessOrEqual(dotGuard, 16)>
      val Token(dotKind, _, _, _, _) = toks(pos)
      if dotKind == TokDot && pos + 1 < endTok then
        val segPos: U64 = pos + 1
        val Token(_, _, segFin, _, _) = toks(segPos)
        if firstBeg <= segFin then typeEnd = segFin else ()
        pos = segPos + 1
        dotGuard = dotGuard + 1
      else
        dotGuard = 16
    var elemStart: U64 = 0
    var elemEnd: U64 = 0
    if pos < endTok then
      val Token(brackKind, _, _, _, _) = toks(pos)
      if brackKind == TokLeftBrack then
        pos = pos + 1
        if pos < endTok then
          val Token(_, elemBeg, elemFin, _, _) = toks(pos)
          elemStart = elemBeg
          elemEnd = elemFin
          pos = pos + 1
          var elemGuard: U64 = 0
          while pos < endTok && elemGuard < 16 do
            @<lessOrEqual(pos, endTok), lessOrEqual(elemStart, elemEnd), lessOrEqual(elemGuard, 16)>
            val Token(elemDot, _, _, _, _) = toks(pos)
            if elemDot == TokDot && pos + 1 < endTok then
              val segPos: U64 = pos + 1
              val Token(_, _, segFin, _, _) = toks(segPos)
              if elemStart <= segFin then elemEnd = segFin else ()
              pos = segPos + 1
              elemGuard = elemGuard + 1
            else
              elemGuard = 16
        else ()
        var depth: U64 = 1
        while pos < endTok && depth > 0 do
          @<lessOrEqual(pos, endTok), lessOrEqual(depth, Stigi.Limit.maxTokens)>
          val Token(curKind, _, _, _, _) = toks(pos)
          if curKind == TokLeftBrack && depth < Stigi.Limit.maxTokens then depth = depth + 1 else if curKind == TokRightBrack then depth = depth - 1 else ()
          pos = pos + 1
        if pos < endTok then
          val Token(parKind, _, _, _, _) = toks(pos)
          if parKind == TokLeftParen then
            pos = pos + 1
            var parenDepth: U64 = 1
            while pos < endTok && parenDepth > 0 do
              @<lessOrEqual(pos, endTok), lessOrEqual(parenDepth, Stigi.Limit.maxTokens)>
              val Token(curKind, _, _, _, _) = toks(pos)
              if curKind == TokLeftParen && parenDepth < Stigi.Limit.maxTokens then parenDepth = parenDepth + 1 else if curKind == TokRightParen then parenDepth = parenDepth - 1 else ()
              pos = pos + 1
          else ()
        else ()
      else ()
    else ()
    TypeSpan(firstBeg, typeEnd, elemStart, elemEnd, pos)
