// prompt: compare two byte slices across distinct buffers for exact byte equality.
pub def Stigi.Emit.slicesEqual(leftCap: U64, leftBytes: Array[U8](leftCap), leftStart: U64, leftEnd: U64, rightCap: U64, rightBytes: Array[U8](rightCap), rightStart: U64, rightEnd: U64) <lessOrEqual(leftStart, leftEnd), lessOrEqual(leftEnd, leftCap), below(leftCap, 0x100000000), lessOrEqual(rightStart, rightEnd), lessOrEqual(rightEnd, rightCap), below(rightCap, 0x100000000)>: Bool =
  val leftLen: U64 = leftEnd - leftStart
  val rightLen: U64 = rightEnd - rightStart
  if leftLen != rightLen then
    false
  else
    var leftPos: U64 = leftStart
    var rightPos: U64 = rightStart
    var equal: Bool = true
    while leftPos < leftEnd && rightPos < rightEnd && equal do
      @<lessOrEqual(leftStart, leftPos), lessOrEqual(leftPos, leftEnd), lessOrEqual(rightStart, rightPos), lessOrEqual(rightPos, rightEnd), decreases(leftEnd - leftPos)>
      if leftBytes(leftPos) != rightBytes(rightPos) then
        equal = false
      else
        ()
      leftPos = leftPos + 1
      rightPos = rightPos + 1
    equal
