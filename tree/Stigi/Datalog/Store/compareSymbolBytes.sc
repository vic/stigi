// prompt: compare two pooled symbol byte slices in unsigned lexicographical byte order.
def Stigi.Datalog.Store.compareSymbolBytes(poolCap: U64, pool: Array[U8](poolCap), leftFirst: U64, leftEnd: U64, rightFirst: U64, rightEnd: U64) <lessOrEqual(leftFirst, leftEnd), lessOrEqual(leftEnd, poolCap), lessOrEqual(rightFirst, rightEnd), lessOrEqual(rightEnd, poolCap)>: I64 =
  var leftPos: U64 = leftFirst
  var rightPos: U64 = rightFirst
  var cmpRes: I64 = 0
  while cmpRes == 0 && leftPos < leftEnd && rightPos < rightEnd do
    val nextLeft: U64 = leftPos + 1
    val nextRight: U64 = rightPos + 1
    val leftByte: U64 = U64.ofU8(pool(leftPos))
    val rightByte: U64 = U64.ofU8(pool(rightPos))
    if leftByte < rightByte then
      cmpRes = -1
    else if leftByte > rightByte then
      cmpRes = 1
    else
      ()
    leftPos = nextLeft
    rightPos = nextRight
  if cmpRes != 0 then
    cmpRes
  else
    val leftLen: U64 = leftEnd - leftFirst
    val rightLen: U64 = rightEnd - rightFirst
    if leftLen < rightLen then
      -1
    else if leftLen > rightLen then
      1
    else
      0
