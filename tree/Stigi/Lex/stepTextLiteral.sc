// prompt: advance past a quoted text literal with backslash escapes.
def Stigi.Lex.stepTextLiteral(length: U64, source: Array[U8](length), startOffset: U64) <below(startOffset, length)>: U64 <lessOrEqual(startOffset, result), lessOrEqual(result, length)> =
  var cursor: U64 = startOffset + 1
  var escaped: Bool = false
  var done: Bool = false
  while cursor < length && !done do
    @<lessOrEqual(startOffset, cursor), lessOrEqual(cursor, length), decreases(length - cursor)>
    val byte: U8 = source(cursor)
    if escaped then
      escaped = false
      cursor = cursor + 1
    else
      if byte == 92 then
        escaped = true
        cursor = cursor + 1
      else
        if byte == 34 then
          done = true
          cursor = cursor + 1
        else
          cursor = cursor + 1
  cursor
