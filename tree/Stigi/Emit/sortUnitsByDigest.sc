// prompt: sort unit indices deterministically by 256-bit unit identity digest without pointer ordering.
pub def Stigi.Emit.sortUnitsByDigest {Stigi.Mut} (unitCount: U64, digestLow: Array[U64](unitCount), digestMid: Array[U64](unitCount), digestHigh: Array[U64](unitCount), digestTop: Array[U64](unitCount), order: Array[U64](unitCount)) <below(unitCount, 4096)>: Unit =
  var initIndex: U64 = 0
  while initIndex < unitCount do
    @<lessOrEqual(initIndex, unitCount), decreases(unitCount - initIndex)>
    order(initIndex) := initIndex
    initIndex = initIndex + 1
  var outerIndex: U64 = 0
  while outerIndex < unitCount do
    @<lessOrEqual(outerIndex, unitCount), decreases(unitCount - outerIndex)>
    var bestIndex: U64 = outerIndex
    var scanIndex: U64 = outerIndex + 1
    while scanIndex < unitCount do
      @<lessOrEqual(scanIndex, unitCount), below(bestIndex, unitCount), decreases(unitCount - scanIndex)>
      val scanUnit: U64 = order(scanIndex)
      val bestUnit: U64 = order(bestIndex)
      if scanUnit < unitCount && bestUnit < unitCount then
        if Stigi.Emit.digestSlotLess(unitCount, digestLow, digestMid, digestHigh, digestTop, scanUnit, bestUnit) then
          bestIndex = scanIndex
        else
          ()
      else
        ()
      scanIndex = scanIndex + 1
    if bestIndex != outerIndex then
      val savedUnit: U64 = order(outerIndex)
      order(outerIndex) := order(bestIndex)
      order(bestIndex) := savedUnit
    else
      ()
    outerIndex = outerIndex + 1
