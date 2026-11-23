// prompt: read and verify the eight-byte POSIX contract fixture through typestate Stigi.File.
def Stigi.Posix.readContractFixture {region: Stigi.Region, Stigi.Mut} (readDesc: I32) <lessOrEqual(0, readDesc)>: Stigi.RunOutcome =
  handle inFile = Stigi.Posix.File(readDesc)
  val sizeRes: Stigi.File.SizeResult = inFile.fileSize()
  val Filled(readBuf) = Array.tryFill[U8]{region}(8, 0) else
    case FillFailed =>
      val closeMem: Stigi.File.CloseResult = inFile.closeFile()
      closeMem match
        case ClosedClean => RunOutOfMemory
        case CloseFailed => RunOutOfMemory
  val readRes: Stigi.File.ReadResult = inFile.readChunk(8, readBuf)
  val closeIn: Stigi.File.CloseResult = inFile.closeFile()
  val readValid: Bool = (sizeRes, readRes, closeIn) match
    case (SizeBytes(totalLen), ReadBytes(gotLen), ClosedClean) =>
      if totalLen == 8 && gotLen == 8 then readBuf(0) == 83 && readBuf(7) == 10 else false
    case (SizeBytes(_), ReadEnd, ClosedClean) => false
    case (SizeBytes(_), ReadFailed, ClosedClean) => false
    case (SizeFailed, _, ClosedClean) => false
    case (_, _, CloseFailed) => false
  if readValid then RunSuccess else RunContractError
