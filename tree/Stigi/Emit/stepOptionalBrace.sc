// prompt: step past an optional capability brace group clamped to the expression end.
def Stigi.Emit.stepOptionalBrace(srcCap: U64, srcBytes: Array[U8](srcCap), tokCap: U64, tokStart: Array[U64](tokCap), tokEnd: Array[U64](tokCap), tokCount: U64, posTok: U64, exprEnd: U64) <lessOrEqual(tokCount, tokCap), below(tokCap, 65536), lessOrEqual(posTok, exprEnd), lessOrEqual(exprEnd, tokCount), below(srcCap, 0x100000000)>: U64 <lessOrEqual(posTok, result), lessOrEqual(result, exprEnd)> =
  if posTok < exprEnd && Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, posTok, 1, "{") then
    val afterBrace: U64 = Stigi.Emit.stepPastBalanced(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, posTok, 123, 125)
    if afterBrace <= exprEnd then afterBrace else exprEnd
  else
    posTok
