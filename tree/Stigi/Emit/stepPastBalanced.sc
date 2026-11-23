// prompt: advance past a balanced delimiter pair starting at the opening token index.
pub def Stigi.Emit.stepPastBalanced(srcCap: U64, srcBytes: Array[U8](srcCap), tokCap: U64, tokStart: Array[U64](tokCap), tokEnd: Array[U64](tokCap), tokCount: U64, startTok: U64, openByte: U8, closeByte: U8) <lessOrEqual(tokCount, tokCap), below(tokCap, 65536), below(startTok, tokCount), below(srcCap, 0x100000000)>: U64 <lessOrEqual(startTok, result), lessOrEqual(result, tokCount)> =
  var cursor: U64 = startTok + 1
  var extraOpens: U64 = 0
  var closeCount: U64 = 0
  while cursor < tokCount && closeCount <= extraOpens do
    @<lessOrEqual(startTok, cursor), lessOrEqual(cursor, tokCount), lessOrEqual(extraOpens, cursor), lessOrEqual(closeCount, cursor), decreases(tokCount - cursor)>
    val nextCursor: U64 = cursor + 1
    val tokBeg: U64 = tokStart(cursor)
    val tokFin: U64 = tokEnd(cursor)
    if tokBeg < tokFin && tokFin <= srcCap && tokFin - tokBeg == 1 then
      val tokByte: U8 = srcBytes(tokBeg)
      if tokByte == openByte then
        extraOpens = extraOpens + 1
      else if tokByte == closeByte then
        closeCount = closeCount + 1
      else
        ()
    else
      ()
    cursor = nextCursor
  cursor
