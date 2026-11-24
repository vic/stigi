// prompt: the position of the first newline, or tab when asked, at or after a position of the index text.
pub def Stigi.Store.indexStop(index: Array[U8](262144), indexLen: U64, pos: U64, tabStops: Bool) <lessOrEqual(indexLen, 262144), lessOrEqual(pos, indexLen)>: U64 <lessOrEqual(pos, result), lessOrEqual(result, indexLen)> =
  var endPos: U64 = pos
  while endPos < indexLen && index(endPos) != '\n' && !(tabStops && index(endPos) == '\t') do
    @<lessOrEqual(pos, endPos), lessOrEqual(endPos, indexLen)>
    endPos = endPos + 1
  endPos
