// prompt: advance past a single-line comment up to newline or end of source.
def Stigi.Lex.stepLineComment(length: U64, source: Array[U8](length), startOffset: U64) <lessOrEqual(startOffset, length)>: U64 <lessOrEqual(startOffset, result), lessOrEqual(result, length)> =
  var cursor: U64 = startOffset
  while cursor < length && source(cursor) != 10 do
    @<lessOrEqual(startOffset, cursor), lessOrEqual(cursor, length), decreases(length - cursor)>
    cursor = cursor + 1
  cursor
