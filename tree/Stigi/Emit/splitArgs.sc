// prompt: split a parenthesized comma-separated argument list into up to 32 bounded start/end token slices.
def Stigi.Emit.splitArgs {Stigi.Mut} (tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), startTok: U64, endTok: U64, argBegs: Array[U64](32), argFins: Array[U64](32)) <lessOrEqual(startTok, endTok), lessOrEqual(endTok, tokCap)>: U64 <lessOrEqual(result, 32)> =
  var count: U64 = 0
  if startTok < endTok then
    val Token(openKind, _, _, _, _) = toks(startTok)
    if openKind == TokLeftParen then
      var pos: U64 = startTok + 1
      var segStart: U64 = pos
      var parenDepth: U64 = 0
      var brackDepth: U64 = 0
      var braceDepth: U64 = 0
      var done: Bool = false
      while pos < endTok && !done do
        @<lessOrEqual(startTok, segStart), lessOrEqual(segStart, pos), lessOrEqual(pos, endTok), lessOrEqual(count, 32), lessOrEqual(parenDepth, Stigi.Limit.maxTokens), lessOrEqual(brackDepth, Stigi.Limit.maxTokens), lessOrEqual(braceDepth, Stigi.Limit.maxTokens)>
        val Token(kind, _, _, _, _) = toks(pos)
        if kind == TokRightParen && parenDepth == 0 && brackDepth == 0 && braceDepth == 0 then
          if segStart < pos && count < 32 then
            argBegs(count) := segStart
            argFins(count) := pos
            count = count + 1
          else ()
          done = true
        else if kind == TokComma && parenDepth == 0 && brackDepth == 0 && braceDepth == 0 then
          if count < 32 then
            argBegs(count) := segStart
            argFins(count) := pos
            count = count + 1
          else ()
          pos = pos + 1
          segStart = pos
        else
          if kind == TokLeftParen && parenDepth < Stigi.Limit.maxTokens then parenDepth = parenDepth + 1
          else if kind == TokRightParen && parenDepth > 0 then parenDepth = parenDepth - 1
          else if kind == TokLeftBrack && brackDepth < Stigi.Limit.maxTokens then brackDepth = brackDepth + 1
          else if kind == TokRightBrack && brackDepth > 0 then brackDepth = brackDepth - 1
          else if kind == TokLeftBrace && braceDepth < Stigi.Limit.maxTokens then braceDepth = braceDepth + 1
          else if kind == TokRightBrace && braceDepth > 0 then braceDepth = braceDepth - 1
          else ()
          pos = pos + 1
    else ()
  else ()
  count
