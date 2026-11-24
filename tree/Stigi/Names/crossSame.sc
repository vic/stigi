// prompt: compare a byte run of one pool with a byte run of another pool: equal when both runs end together with the same bytes.
def Stigi.Names.crossSame(leftPool: Array[U8](Stigi.Limit.maxIndexBytes), leftFirst: U64, leftEnd: U64, rightPool: Array[U8](Stigi.Limit.maxIndexBytes), rightFirst: U64, rightEnd: U64) <lessOrEqual(leftFirst, leftEnd), lessOrEqual(leftEnd, Stigi.Limit.maxIndexBytes), lessOrEqual(rightFirst, rightEnd), lessOrEqual(rightEnd, Stigi.Limit.maxIndexBytes)>: Bool =
  var left: U64 = leftFirst
  var right: U64 = rightFirst
  var alike: Bool = true
  while alike && left < leftEnd && right < rightEnd do
    @<lessOrEqual(left, leftEnd), lessOrEqual(right, rightEnd)>
    alike = leftPool(left) == rightPool(right)
    left = left + 1
    right = right + 1
  alike && left == leftEnd && right == rightEnd
