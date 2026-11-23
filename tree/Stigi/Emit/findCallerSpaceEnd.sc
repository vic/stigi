// prompt: find the end offset of the namespace prefix ending at the last dot in a unit name slice.
def Stigi.Emit.findCallerSpaceEnd(indexCap: U64, indexBytes: Array[U8](indexCap), nameStart: U64, nameEnd: U64) <lessOrEqual(nameStart, nameEnd), lessOrEqual(nameEnd, indexCap), below(indexCap, 0x100000000)>: U64 <lessOrEqual(nameStart, result), lessOrEqual(result, nameEnd)> =
  var lastDotEnd: U64 = nameStart
  var scanPos: U64 = nameStart
  while scanPos < nameEnd do
    @<lessOrEqual(nameStart, scanPos), lessOrEqual(scanPos, nameEnd), lessOrEqual(nameStart, lastDotEnd), lessOrEqual(lastDotEnd, nameEnd), decreases(nameEnd - scanPos)>
    if indexBytes(scanPos) == 46 then
      lastDotEnd = scanPos + 1
    else
      ()
    scanPos = scanPos + 1
  lastDotEnd
