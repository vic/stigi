// prompt: find the end token of one indented statement starting at startTok with column baseCol.
def Stigi.Emit.findStmtEnd(tokCap: Stigi.TokenCap, toks: Array[Stigi.Lex.Token](tokCap), startTok: U64, endTok: U64, baseCol: U64) <lessOrEqual(startTok, endTok), lessOrEqual(endTok, tokCap)>: U64 <lessOrEqual(startTok, result), lessOrEqual(result, endTok)> =
  var pos: U64 = startTok
  var parenDepth: U64 = 0
  var brackDepth: U64 = 0
  var braceDepth: U64 = 0
  var seenNewline: Bool = false
  var done: Bool = false
  while pos < endTok && !done do
    @<lessOrEqual(startTok, pos), lessOrEqual(pos, endTok), lessOrEqual(parenDepth, Stigi.Limit.maxTokens), lessOrEqual(brackDepth, Stigi.Limit.maxTokens), lessOrEqual(braceDepth, Stigi.Limit.maxTokens)>
    val Token(kind, _, _, _, colNum) = toks(pos)
    if kind == TokEnd then
      done = true
    else if kind == TokNewline then
      seenNewline = true
      pos = pos + 1
    else if seenNewline && parenDepth == 0 && brackDepth == 0 && braceDepth == 0 && colNum <= baseCol then
      if colNum == baseCol && kind == TokElse then
        seenNewline = false
        pos = pos + 1
      else
        done = true
    else
      seenNewline = false
      if kind == TokLeftParen && parenDepth < Stigi.Limit.maxTokens then parenDepth = parenDepth + 1 else if kind == TokRightParen && parenDepth > 0 then parenDepth = parenDepth - 1 else if kind == TokLeftBrack && brackDepth < Stigi.Limit.maxTokens then brackDepth = brackDepth + 1 else if kind == TokRightBrack && brackDepth > 0 then brackDepth = brackDepth - 1 else if kind == TokLeftBrace && braceDepth < Stigi.Limit.maxTokens then braceDepth = braceDepth + 1 else if kind == TokRightBrace && braceDepth > 0 then braceDepth = braceDepth - 1 else ()
      pos = pos + 1
  pos
