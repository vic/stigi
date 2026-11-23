// prompt: check whether a referenced enum type slice has already been emitted.
def Stigi.Emit.isTypeSliceEmitted(unitCount: U64, indexCap: U64, indexBytes: Array[U8](indexCap), nameStarts: Array[U64](unitCount), nameEnds: Array[U64](unitCount), kindCodes: Array[U8](unitCount), enumEmitted: Array[U8](unitCount), srcCap: U64, srcBytes: Array[U8](srcCap), typeStart: U64, typeEnd: U64) <below(unitCount, 4096), below(indexCap, 0x100000000), lessOrEqual(typeStart, typeEnd), lessOrEqual(typeEnd, srcCap), below(srcCap, 0x100000000)>: Bool =
  if Stigi.Emit.isPrimitiveTypeSlice(srcCap, srcBytes, typeStart, typeEnd) then
    true
  else
    var ready: Bool = true
    var depIdx: U64 = 0
    while depIdx < unitCount && ready do
      @<lessOrEqual(depIdx, unitCount), decreases(unitCount - depIdx)>
      if kindCodes(depIdx) == 1 then
        val depStart: U64 = nameStarts(depIdx)
        val depEnd: U64 = nameEnds(depIdx)
        if depStart <= depEnd && depEnd <= indexCap then
          if Stigi.Emit.slicesEqual(indexCap, indexBytes, depStart, depEnd, srcCap, srcBytes, typeStart, typeEnd) && enumEmitted(depIdx) == 0 then
            ready = false
          else
            ()
        else
          ()
      else
        ()
      depIdx = depIdx + 1
    ready
