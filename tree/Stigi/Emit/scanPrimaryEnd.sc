// prompt: advance past one primary expression token span including optional capability and call arguments.
pub def Stigi.Emit.scanPrimaryEnd(srcCap: U64, srcBytes: Array[U8](srcCap), tokCap: U64, tokStart: Array[U64](tokCap), tokEnd: Array[U64](tokCap), tokCount: U64, startTok: U64, exprEnd: U64) <lessOrEqual(tokCount, tokCap), below(tokCap, 65536), lessOrEqual(exprEnd, tokCount), below(startTok, exprEnd), below(srcCap, 0x100000000)>: U64 <lessOrEqual(startTok, result), lessOrEqual(result, exprEnd)> =
  if Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, startTok, 1, "(") then
    val afterParen: U64 = Stigi.Emit.stepPastBalanced(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, startTok, 40, 41)
    if afterParen <= exprEnd then afterParen else exprEnd
  else
    var cursor: U64 = startTok + 1
    if cursor < exprEnd && Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, cursor, 1, "{") then
      val afterBrace: U64 = Stigi.Emit.stepPastBalanced(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, cursor, 123, 125)
      cursor = if afterBrace <= exprEnd then afterBrace else exprEnd
    else
      ()
    if cursor < exprEnd && Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, cursor, 1, "(") then
      val afterArgs: U64 = Stigi.Emit.stepPastBalanced(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, cursor, 40, 41)
      cursor = if afterArgs <= exprEnd then afterArgs else exprEnd
    else
      ()
    cursor
