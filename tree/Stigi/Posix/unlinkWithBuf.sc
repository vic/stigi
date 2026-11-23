// prompt: unlink a file path using a caller-supplied 512-byte path buffer.
def Stigi.Posix.unlinkWithBuf {Stigi.Mut} (pathCap: U64, pathBytes: Array[U8](pathCap), pathStart: U64, pathEnd: U64, pathBuf: Array[U8](512)) <lessOrEqual(pathStart, pathEnd), lessOrEqual(pathEnd, pathCap), below(pathCap, 0x100000000)>: Bool =
  if Stigi.Posix.copyNullTerminatedPath(pathCap, pathBytes, pathStart, pathEnd, pathBuf) then
    Stigi.Posix.unlink(pathBuf) == 0
  else
    false
