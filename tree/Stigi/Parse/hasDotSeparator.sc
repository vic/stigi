// prompt: check whether an identifier slice contains a dot separator indicating a qualified reference.
def Stigi.Parse.hasDotSeparator(length: U64, source: Array[U8](length), startOffset: U64, endOffset: U64) <lessOrEqual(startOffset, endOffset), lessOrEqual(endOffset, length)>: Bool =
  var cursor: U64 = startOffset
  var found: Bool = false
  while cursor < endOffset && !found do
    @<lessOrEqual(startOffset, cursor), lessOrEqual(cursor, endOffset), decreases(endOffset - cursor)>
    if source(cursor) == 46 then
      found = true
      cursor = cursor + 1
    else
      cursor = cursor + 1
  found
