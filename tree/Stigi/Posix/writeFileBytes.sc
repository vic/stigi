// prompt: open a verified null-terminated path write-only and write all bytes from a source slice.
pub def Stigi.Posix.writeFileBytes {region: Stigi.Region, Stigi.Mut} (pathLen: U64, path: Array[U8](4096), srcCap: Stigi.EmitCap, srcBuf: Array[U8](srcCap), srcLen: U64) <below(pathLen, 4096), lessOrEqual(srcLen, srcCap)>: Stigi.RunOutcome =
  val Opened(desc) = Stigi.Posix.openPath(pathLen, path, Stigi.Posix.writeCreateTruncate) else
    case OpenFailed => RunFileError
  Stigi.Posix.writeToDesc{region}(desc, srcCap, srcBuf, srcLen)
