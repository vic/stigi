// prompt: compute the exact constant offset between two normalized linear length terms when their variables match.
pub def Stigi.Types.linearConstantOffset(leftConst: I64, leftCap: U64, leftBinders: Array[U64](leftCap), leftCoeffs: Array[I64](leftCap), leftCount: U64, rightConst: I64, rightCap: U64, rightBinders: Array[U64](rightCap), rightCoeffs: Array[I64](rightCap), rightCount: U64) <lessOrEqual(leftCount, leftCap), lessOrEqual(rightCount, rightCap), below(leftCap, 64), below(rightCap, 64), lessOrEqual(-1000000, leftConst), lessOrEqual(leftConst, 1000000), lessOrEqual(-1000000, rightConst), lessOrEqual(rightConst, 1000000)>: Stigi.Types.OffsetOutcome =
  if leftCount != rightCount then
    Stigi.Types.OffsetOutcome.OffsetDistinct
  else
    var index: U64 = 0
    var matched: Bool = true
    while index < leftCount && matched do
      @<lessOrEqual(index, leftCount), decreases(leftCount - index)>
      if leftBinders(index) != rightBinders(index) || leftCoeffs(index) != rightCoeffs(index) then
        matched = false
        index = index + 1
      else
        index = index + 1
    if matched then
      Stigi.Types.OffsetOutcome.OffsetExact(leftConst - rightConst)
    else
      Stigi.Types.OffsetOutcome.OffsetDistinct
