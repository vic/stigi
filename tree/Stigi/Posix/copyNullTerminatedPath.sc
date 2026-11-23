// prompt: copy a path slice into a 512-byte buffer and write the trailing NUL terminator byte.
pub def Stigi.Posix.copyNullTerminatedPath {Stigi.Mut} (srcCap: U64, srcBytes: Array[U8](srcCap), srcStart: U64, srcEnd: U64, pathBuf: Array[U8](512)) <lessOrEqual(srcStart, srcEnd), lessOrEqual(srcEnd, srcCap), below(srcCap, 0x100000000)>: Bool =
  val pathLen: U64 = srcEnd - srcStart
  if pathLen < 512 then
    var idx: U64 = 0
    while idx < pathLen do
      @<lessOrEqual(idx, pathLen), decreases(pathLen - idx)>
      val srcPos: U64 = srcStart + idx
      if srcPos < srcEnd then
        pathBuf(idx) := srcBytes(srcPos)
      else
        ()
      idx = idx + 1
    pathBuf(pathLen) := 0
    true
  else
    false
