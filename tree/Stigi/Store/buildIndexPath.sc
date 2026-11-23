// prompt: build a null-terminated index file path <dir>/index into a 4096-byte buffer.
pub def Stigi.Store.buildIndexPath {Stigi.Mut} (dirCap: Stigi.ByteCap, dirBuf: Array[U8](dirCap), dirStart: U64, dirEnd: U64, outPath: Array[U8](4096)) <lessOrEqual(dirStart, dirEnd), lessOrEqual(dirEnd, dirCap)>: Stigi.Lookup =
  val dirLen: U64 = dirEnd - dirStart
  if dirLen < 4000 then
    var outPos: U64 = 0
    var dirIdx: U64 = dirStart
    while dirIdx < dirEnd && outPos < 4000 do
      @<lessOrEqual(dirStart, dirIdx), lessOrEqual(dirIdx, dirEnd), lessOrEqual(outPos, 4000)>
      outPath(outPos) := dirBuf(dirIdx)
      outPos = outPos + 1
      dirIdx = dirIdx + 1
    val suffix: Array[U8](6) = "/index"
    var sufIdx: U64 = 0
    while sufIdx < 6 && outPos < 4095 do
      @<lessOrEqual(sufIdx, 6), lessOrEqual(outPos, 4095)>
      outPath(outPos) := suffix(sufIdx)
      outPos = outPos + 1
      sufIdx = sufIdx + 1
    outPath(outPos) := 0
    Found(outPos)
  else Missing
