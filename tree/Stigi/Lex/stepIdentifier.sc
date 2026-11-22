// prompt: advance past one dotted identifier starting at a valid offset.
def Stigi.Lex.stepIdentifier(length: U64, source: Array[U8](length), startOffset: U64) <lessOrEqual(startOffset, length)>: U64 <lessOrEqual(startOffset, result), lessOrEqual(result, length)> =
  var cursor: U64 = startOffset
  while cursor < length && Stigi.Lex.isIdentByte(source(cursor)) do
    @<lessOrEqual(startOffset, cursor), lessOrEqual(cursor, length), decreases(length - cursor)>
    cursor = cursor + 1
  cursor
