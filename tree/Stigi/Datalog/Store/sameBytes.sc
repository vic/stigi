// prompt: test byte-for-byte equality of two symbol slices.
def Stigi.Datalog.Store.sameBytes(poolCap: U64, pool: Array[U8](poolCap), leftFirst: U64, leftEnd: U64, srcCap: U64, src: Array[U8](srcCap), rightFirst: U64, rightEnd: U64) <lessOrEqual(leftFirst, leftEnd), lessOrEqual(leftEnd, poolCap), lessOrEqual(rightFirst, rightEnd), lessOrEqual(rightEnd, srcCap)>: Bool =
  val leftLen: U64 = leftEnd - leftFirst
  val rightLen: U64 = rightEnd - rightFirst
  if leftLen != rightLen then
    false
  else
    var leftPos: U64 = leftFirst
    var rightPos: U64 = rightFirst
    var sameFlag: Bool = true
    while sameFlag && leftPos < leftEnd && rightPos < rightEnd do
      val nextLeft: U64 = leftPos + 1
      val nextRight: U64 = rightPos + 1
      if pool(leftPos) != src(rightPos) then
        sameFlag = false
      else
        ()
      leftPos = nextLeft
      rightPos = nextRight
    sameFlag
