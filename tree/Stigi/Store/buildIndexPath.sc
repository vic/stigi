// prompt: build a null-terminated index file path <dir>/index into a 4096-byte buffer.
pub def Stigi.Store.buildIndexPath {Stigi.Mut} (dirCap: Stigi.ByteCap, dirBuf: Array[U8](dirCap), dirStart: U64, dirEnd: U64, outPath: Array[U8](4096)) <lessOrEqual(dirStart, dirEnd), lessOrEqual(dirEnd, dirCap)>: Stigi.Lookup =
  val suffix: Array[U8](6) = "/index"
  if dirEnd - dirStart < 4000 then
    val dirTo: U64 = Stigi.Array.copy(dirCap, dirBuf, dirStart, dirEnd, 4096, outPath, 0)
    val pathEnd: U64 = Stigi.Array.copy(6, suffix, 0, 6, 4096, outPath, dirTo)
    if pathEnd < 4096 then
      outPath(pathEnd) := '\0'
      Found(pathEnd)
    else Missing
  else Missing
