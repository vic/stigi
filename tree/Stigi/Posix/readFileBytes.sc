// prompt: open a verified null-terminated path read-only and load its bytes into a destination slice.
pub def Stigi.Posix.readFileBytes {region: Stigi.Region, Stigi.Mut} (pathLen: U64, path: Array[U8](4096), outCap: Stigi.ByteCap, outBuf: Array[U8](outCap), startOff: U64) <below(pathLen, 4096), lessOrEqual(startOff, outCap)>: Stigi.Posix.LoadResult =
  val Opened(desc) = Stigi.Posix.openPath(pathLen, path, Stigi.Posix.readOnly) else
    case OpenFailed => LoadFailed
  Stigi.Posix.readFromDesc{region}(desc, outCap, outBuf, startOff)
