// prompt: find the innermost local variable or parameter matching a source slice.
pub def Stigi.Emit.findLocalIndex(srcCap: U64, srcBytes: Array[U8](srcCap), nameStart: U64, nameEnd: U64, localCap: U64, localNameStart: Array[U64](localCap), localNameEnd: Array[U64](localCap), localCount: U64) <lessOrEqual(nameStart, nameEnd), lessOrEqual(nameEnd, srcCap), below(srcCap, 0x100000000), lessOrEqual(localCount, localCap), below(localCap, 256)>: U64 <lessOrEqual(result, localCount)> =
  var bestIdx: U64 = localCount
  var idx: U64 = 0
  while idx < localCount do
    @<lessOrEqual(idx, localCount), lessOrEqual(bestIdx, localCount), decreases(localCount - idx)>
    val locBeg: U64 = localNameStart(idx)
    val locFin: U64 = localNameEnd(idx)
    if locBeg <= locFin && locFin <= srcCap then
      if Stigi.Emit.slicesEqual(srcCap, srcBytes, locBeg, locFin, srcCap, srcBytes, nameStart, nameEnd) then
        bestIdx = idx
      else
        ()
    else
      ()
    idx = idx + 1
  bestIdx
