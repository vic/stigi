// prompt: find a top-level unit index by exact dotted name slice in the index table.
pub def Stigi.Emit.findUnitByName(unitCount: U64, indexCap: U64, indexBytes: Array[U8](indexCap), nameStarts: Array[U64](unitCount), nameEnds: Array[U64](unitCount), srcCap: U64, srcBytes: Array[U8](srcCap), queryStart: U64, queryEnd: U64) <below(unitCount, 4096), below(indexCap, 0x100000000), lessOrEqual(queryStart, queryEnd), lessOrEqual(queryEnd, srcCap), below(srcCap, 0x100000000)>: U64 <lessOrEqual(result, unitCount)> =
  var foundIdx: U64 = unitCount
  var idx: U64 = 0
  while idx < unitCount && foundIdx == unitCount do
    @<lessOrEqual(idx, unitCount), lessOrEqual(foundIdx, unitCount), decreases(unitCount - idx)>
    val unitBeg: U64 = nameStarts(idx)
    val unitFin: U64 = nameEnds(idx)
    if unitBeg <= unitFin && unitFin <= indexCap then
      if Stigi.Emit.slicesEqual(indexCap, indexBytes, unitBeg, unitFin, srcCap, srcBytes, queryStart, queryEnd) then
        foundIdx = idx
      else
        ()
    else
      ()
    idx = idx + 1
  foundIdx
