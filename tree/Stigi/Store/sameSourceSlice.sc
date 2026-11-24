// prompt: compare two byte slices inside one unit source buffer for exact byte equality.
def Stigi.Store.sameSourceSlice(src: Array[U8](Stigi.Limit.maxUnitBytes), leftStart: U64, leftEnd: U64, rightStart: U64, rightEnd: U64) <lessOrEqual(leftStart, leftEnd), lessOrEqual(leftEnd, Stigi.Limit.maxUnitBytes), lessOrEqual(rightStart, rightEnd), lessOrEqual(rightEnd, Stigi.Limit.maxUnitBytes)>: Bool =
  val leftLen: U64 = leftEnd - leftStart
  val rightLen: U64 = rightEnd - rightStart
  if leftLen != rightLen then false else
    var offset: U64 = 0
    var matched: Bool = true
    while offset < leftLen && matched do
      @<lessOrEqual(offset, leftLen)>
      val leftPos: U64 = leftStart + offset
      val rightPos: U64 = rightStart + offset
      if leftPos < leftEnd && rightPos < rightEnd then
        if src(leftPos) != src(rightPos) then matched = false else ()
      else matched = false
      offset = offset + 1
    matched
