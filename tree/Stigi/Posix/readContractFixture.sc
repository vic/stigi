// prompt: read and verify the eight-byte POSIX contract fixture through typestate Stigi.File.
def Stigi.Posix.readContractFixture {region: Stigi.Region, Stigi.Mut} (readDesc: I32) <lessOrEqual(0, readDesc)>: Stigi.RunOutcome =
  handle inFile = Stigi.Posix.File(readDesc)
  val sizeRes: Stigi.File.SizeResult = inFile.fileSize()
  val Filled(readBuf) = Array.tryFill[U8]{region}(8, 0) else
    case FillFailed =>
      val _ = inFile.closeFile()
      RunOutOfMemory
  val readRes: Stigi.File.ReadResult = inFile.readChunk(8, readBuf)
  (sizeRes, readRes, inFile.closeFile()) match
    case (SizeBytes(8), ReadBytes(8), ClosedClean) => if readBuf(0) == 'S' && readBuf(7) == '\n' then RunSuccess else RunContractError
    case (SizeBytes(_), ReadBytes(_), ClosedClean) | (SizeBytes(_), ReadEnd | ReadFailed, ClosedClean) | (SizeFailed, _, ClosedClean) | (_, _, CloseFailed) => RunContractError
