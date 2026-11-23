// prompt: write the eight-byte POSIX contract fixture through typestate Stigi.File.
def Stigi.Posix.writeContractFixture(writeDesc: I32) <lessOrEqual(0, writeDesc)>: Bool =
  handle outFile = Stigi.Posix.File(writeDesc)
  val payload: Array[U8](8) = "STIGIV2\n"
  val writeRes: Stigi.File.WriteResult = outFile.writeChunk(8, payload)
  val closeOut: Stigi.File.CloseResult = outFile.closeFile()
  (writeRes, closeOut) match
    case (WroteBytes(wroteLen), ClosedClean) if wroteLen == 8 => true
    case (_, _) => false
