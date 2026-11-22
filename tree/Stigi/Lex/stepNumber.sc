// prompt: advance past one integer literal starting at a valid offset.
def Stigi.Lex.stepNumber(length: U64, source: Array[U8](length), startOffset: U64) <lessOrEqual(startOffset, length)>: U64 <lessOrEqual(startOffset, result), lessOrEqual(result, length)> =
  var cursor: U64 = startOffset
  while cursor < length && Stigi.Lex.isNumberByte(source(cursor)) do
    @<lessOrEqual(startOffset, cursor), lessOrEqual(cursor, length), decreases(length - cursor)>
    cursor = cursor + 1
  cursor
