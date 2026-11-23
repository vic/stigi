// prompt: advance past an optional angle-bracketed fact group <...> with balanced nesting.
def Stigi.Emit.passAngles(tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), startTok: U64, endTok: U64) <lessOrEqual(startTok, endTok), lessOrEqual(endTok, tokCap)>: U64 <lessOrEqual(startTok, result), lessOrEqual(result, endTok)> =
  var pos: U64 = startTok
  if pos < endTok then
    val Token(leadKind, _, _, _, _) = toks(pos)
    if leadKind == TokLess then
      pos = pos + 1
      var depth: U64 = 1
      while pos < endTok && depth > 0 do
        @<lessOrEqual(startTok, pos), lessOrEqual(pos, endTok), lessOrEqual(depth, Stigi.Limit.maxTokens)>
        val Token(curKind, _, _, _, _) = toks(pos)
        if curKind == TokLess && depth < Stigi.Limit.maxTokens then depth = depth + 1 else if curKind == TokGreat then depth = depth - 1 else ()
        pos = pos + 1
    else ()
  else ()
  pos
