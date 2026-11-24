// prompt: read all bytes from an opened descriptor via typestate Stigi.File and close it once.
def Stigi.Posix.readFromDesc {region: Stigi.Region, Stigi.Mut} (desc: I32, outCap: Stigi.ByteCap, outBuf: Array[U8](outCap), startOff: U64) <lessOrEqual(startOff, outCap)>: Stigi.Posix.LoadResult =
  handle inFile = Stigi.Posix.File(desc)
  val Filled(chunkBuf) = Array.tryFill[U8]{region}(4096, 0) else
    case FillFailed =>
      val _ = inFile.closeFile()
      LoadFailed
  val loaded: Stigi.Posix.LoadResult = Stigi.Posix.readChunks{inFile}(chunkBuf, outCap, outBuf, startOff, startOff, Stigi.Limit.maxSourceBytes)
  val closed: Stigi.File.CloseResult = inFile.closeFile()
  (loaded, closed) match
    case (LoadOverflow, _) => LoadOverflow
    case (LoadedBytes(_) | LoadFailed, CloseFailed) => LoadFailed
    case (LoadedBytes(_) | LoadFailed, ClosedClean) => loaded
