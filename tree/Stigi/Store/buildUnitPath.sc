// prompt: build a null-terminated unit file path <dir>/Unit/Name.sc into a 4096-byte buffer.
pub def Stigi.Store.buildUnitPath {Stigi.Mut} (dirCap: Stigi.ByteCap, dirBuf: Array[U8](dirCap), dirStart: U64, dirEnd: U64, nameCap: Stigi.ByteCap, nameBuf: Array[U8](nameCap), nameStart: U64, nameEnd: U64, outPath: Array[U8](4096)) <lessOrEqual(dirStart, dirEnd), lessOrEqual(dirEnd, dirCap), lessOrEqual(nameStart, nameEnd), lessOrEqual(nameEnd, nameCap)>: Stigi.Lookup =
  val dirLen: U64 = dirEnd - dirStart
  val nameLen: U64 = nameEnd - nameStart
  if dirLen < 2000 && nameLen < 2000 then
    var outPos: U64 = 0
    var dirIdx: U64 = dirStart
    while dirIdx < dirEnd && outPos < 4090 do
      @<lessOrEqual(dirStart, dirIdx), lessOrEqual(dirIdx, dirEnd), lessOrEqual(outPos, 4090)>
      outPath(outPos) := dirBuf(dirIdx)
      outPos = outPos + 1
      dirIdx = dirIdx + 1
    if outPos < 4090 then
      outPath(outPos) := 47
      outPos = outPos + 1
      var nameIdx: U64 = nameStart
      while nameIdx < nameEnd && outPos < 4090 do
        @<lessOrEqual(nameStart, nameIdx), lessOrEqual(nameIdx, nameEnd), lessOrEqual(outPos, 4090)>
        val rawByte: U8 = nameBuf(nameIdx)
        val pathByte: U8 = if rawByte == 46 then 47 else rawByte
        outPath(outPos) := pathByte
        outPos = outPos + 1
        nameIdx = nameIdx + 1
      val suffix: Array[U8](3) = ".sc"
      var sufIdx: U64 = 0
      while sufIdx < 3 && outPos < 4095 do
        @<lessOrEqual(sufIdx, 3), lessOrEqual(outPos, 4095)>
        outPath(outPos) := suffix(sufIdx)
        outPos = outPos + 1
        sufIdx = sufIdx + 1
      outPath(outPos) := 0
      Found(outPos)
    else Missing
  else Missing
