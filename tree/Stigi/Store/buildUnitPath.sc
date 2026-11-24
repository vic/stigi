// prompt: build a null-terminated unit file path <dir>/Unit/Name.sc into a 4096-byte buffer.
pub def Stigi.Store.buildUnitPath {Stigi.Mut} (dirCap: Stigi.ByteCap, dirBuf: Array[U8](dirCap), dirStart: U64, dirEnd: U64, nameCap: Stigi.ByteCap, nameBuf: Array[U8](nameCap), nameStart: U64, nameEnd: U64, outPath: Array[U8](4096)) <lessOrEqual(dirStart, dirEnd), lessOrEqual(dirEnd, dirCap), lessOrEqual(nameStart, nameEnd), lessOrEqual(nameEnd, nameCap)>: Stigi.Lookup =
  val suffix: Array[U8](3) = ".sc"
  if dirEnd - dirStart < 2000 && nameEnd - nameStart < 2000 then
    val dirTo: U64 = Stigi.Array.copy(dirCap, dirBuf, dirStart, dirEnd, 4096, outPath, 0)
    if dirTo < 4090 then
      outPath(dirTo) := '/'
      var outPos: U64 = dirTo + 1
      var nameIdx: U64 = nameStart
      while nameIdx < nameEnd && outPos < 4090 do
        @<lessOrEqual(nameStart, nameIdx), lessOrEqual(nameIdx, nameEnd), lessOrEqual(outPos, 4090)>
        outPath(outPos) := if nameBuf(nameIdx) is '.' then '/' else nameBuf(nameIdx)
        outPos = outPos + 1
        nameIdx = nameIdx + 1
      val pathEnd: U64 = Stigi.Array.copy(3, suffix, 0, 3, 4096, outPath, outPos)
      if pathEnd < 4096 then
        outPath(pathEnd) := '\0'
        Found(pathEnd)
      else Missing
    else Missing
  else Missing
