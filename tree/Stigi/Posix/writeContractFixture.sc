// prompt: write the eight-byte POSIX contract fixture through typestate Stigi.File.
def Stigi.Posix.writeContractFixture(writeDesc: I32) <lessOrEqual(0, writeDesc)>: Stigi.RunOutcome =
  handle outFile = Stigi.Posix.File(writeDesc)
  val payload: Array[U8](8) = "STIGIV2\n"
  val writeRes: Stigi.File.WriteResult = outFile.writeChunk(8, payload)
  (writeRes, outFile.closeFile()) match
    case (WroteBytes(8), ClosedClean) => RunSuccess
    case (WroteBytes(_), ClosedClean) | (WroteBytes(_), CloseFailed) | (WriteFailed, ClosedClean) | (WriteFailed, CloseFailed) => RunContractError
