// prompt: find the first occurrence of targetByte inside startOffset..endOffset or return endOffset.
def Stigi.Store.findByteInRange(cap: U64, text: Array[U8](cap), startOffset: U64, endOffset: U64, targetByte: U8) <lessOrEqual(startOffset, endOffset), lessOrEqual(endOffset, cap), below(cap, 0x100000000)>: U64 <lessOrEqual(startOffset, result), lessOrEqual(result, endOffset)> =
  var cursor: U64 = startOffset
  var found: Bool = false
  var matchPos: U64 = endOffset
  while cursor < endOffset && !found do
    @<lessOrEqual(startOffset, cursor), lessOrEqual(cursor, endOffset), lessOrEqual(startOffset, matchPos), lessOrEqual(matchPos, endOffset), decreases(endOffset - cursor)>
    if text(cursor) == targetByte then
      matchPos = cursor
      found = true
      cursor = endOffset
    else
      cursor = cursor + 1
  if found then matchPos else endOffset
