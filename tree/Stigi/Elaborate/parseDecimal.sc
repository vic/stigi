// prompt: parse an unsigned decimal integer literal from a source byte span.
def Stigi.Elaborate.parseDecimal(srcCap: U64, source: Array[U8](srcCap), first: U64, endPos: U64) <lessOrEqual(first, endPos), lessOrEqual(endPos, srcCap)>: U64 =
  var acc: U64 = 0
  var pos: U64 = first
  while pos < endPos do
    @<lessOrEqual(first, pos), lessOrEqual(pos, endPos)>
    val nextPos: U64 = pos + 1
    val byteVal: U64 = U64.ofU8(source(pos))
    if byteVal >= 48 && byteVal <= 57 then
      val digit: U64 = byteVal - 48
      acc = U64.wrappingAdd(U64.wrappingMultiply(acc, 10), digit)
    else
      ()
    pos = nextPos
  acc
