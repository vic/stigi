// prompt: open or create and truncate a file for writing using a caller-supplied 512-byte path buffer.
pub def Stigi.Posix.openWriteTruncWithBuf {Stigi.Mut} (pathCap: U64, pathBytes: Array[U8](pathCap), pathStart: U64, pathEnd: U64, pathBuf: Array[U8](512)) <lessOrEqual(pathStart, pathEnd), lessOrEqual(pathEnd, pathCap), below(pathCap, 0x100000000)>: Stigi.Posix.OpenOutcome =
  if Stigi.Posix.copyNullTerminatedPath(pathCap, pathBytes, pathStart, pathEnd, pathBuf) then
    val atCurDir: I32 = -100
    val rawDesc: I32 = Stigi.Posix.openat(atCurDir, pathBuf, 577, 420)
    Stigi.Posix.classifyOpenResult(rawDesc)
  else
    OpenFailed
