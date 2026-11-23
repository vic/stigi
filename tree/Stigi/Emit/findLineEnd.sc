// prompt: find the token index of the newline terminating the line starting at lineStart.
pub def Stigi.Emit.findLineEnd(tokCap: U64, tokKind: Array[U8](tokCap), tokCount: U64, lineStart: U64) <lessOrEqual(tokCount, tokCap), below(tokCap, 65536), lessOrEqual(lineStart, tokCount)>: U64 <lessOrEqual(lineStart, result), lessOrEqual(result, tokCount)> =
  var cursor: U64 = lineStart
  while cursor < tokCount && tokKind(cursor) != 7 do
    @<lessOrEqual(lineStart, cursor), lessOrEqual(cursor, tokCount), decreases(tokCount - cursor)>
    cursor = cursor + 1
  cursor
