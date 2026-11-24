// prompt: write an entire source slice to an opened descriptor via typestate Stigi.File and close it once.
def Stigi.Posix.writeToDesc {region: Stigi.Region, Stigi.Mut} (desc: I32, srcCap: Stigi.EmitCap, srcBuf: Array[U8](srcCap), srcLen: U64) <lessOrEqual(srcLen, srcCap)>: Stigi.RunOutcome =
  handle outFile = Stigi.Posix.File(desc)
  val Filled(chunkBuf) = Array.tryFill[U8]{region}(4096, 0) else
    case FillFailed =>
      val _ = outFile.closeFile()
      RunOutOfMemory
  val written: Stigi.RunOutcome = Stigi.Posix.writeChunks{outFile, region}(chunkBuf, srcCap, srcBuf, srcLen, 0, srcLen + 1)
  outFile.closeFile() match
    case ClosedClean => written
    case CloseFailed => RunFileError
