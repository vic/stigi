// prompt: count occurrences of one rejection code in an actual rejection list.
def Stigi.Door.countCodeMatches(actualCap: U64, actualCodes: Array[U64](actualCap), actualCount: U64, targetCode: U64) <lessOrEqual(actualCount, actualCap), below(actualCap, 256)>: U64 =
  var innerIndex: U64 = 0
  var matchCount: U64 = 0
  while innerIndex < actualCount do
    @<lessOrEqual(innerIndex, actualCount), lessOrEqual(matchCount, innerIndex), decreases(actualCount - innerIndex)>
    val nextInner: U64 = innerIndex + 1
    matchCount = if actualCodes(innerIndex) == targetCode then matchCount + 1 else matchCount
    innerIndex = nextInner
  matchCount
