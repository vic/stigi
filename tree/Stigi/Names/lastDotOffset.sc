// prompt: find the offset of the last dot separator in a dotted name, or length if none exists.
def Stigi.Names.lastDotOffset(length: U64, name: Array[U8](length)) <below(length, 0x100000000)>: U64 <lessOrEqual(result, length)> =
  var cursor: U64 = 0
  var lastDot: U64 = length
  while cursor < length do
    @<lessOrEqual(cursor, length), lessOrEqual(lastDot, length), decreases(length - cursor)>
    lastDot = if name(cursor) == 46 then cursor else lastDot
    cursor = cursor + 1
  lastDot
