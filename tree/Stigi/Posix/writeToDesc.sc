// prompt: write an entire source slice to an opened descriptor via typestate Stigi.File and close it once.
def Stigi.Posix.writeToDesc {region: Stigi.Region, Stigi.Mut} (desc: I32, srcCap: Stigi.EmitCap, srcBuf: Array[U8](srcCap), srcLen: U64) <lessOrEqual(srcLen, srcCap)>: Bool =
  handle outFile = Stigi.Posix.File(desc)
  val Filled(chunkBuf) = Array.tryFill[U8]{region}(4096, 0) else
    case FillFailed =>
      val _ = outFile.closeFile()
      false
  var sent: U64 = 0
  var failed: Bool = false
  var stepGuard: U64 = 0
  while sent + 4096 <= srcLen && !failed && stepGuard < 65536 do
    @<lessOrEqual(sent, srcLen), lessOrEqual(stepGuard, 65536)>
    var copyIdx: U64 = 0
    while copyIdx < 4096 do
      @<lessOrEqual(copyIdx, 4096)>
      val srcPos: U64 = sent + copyIdx
      if srcPos < srcCap then chunkBuf(copyIdx) := srcBuf(srcPos) else ()
      copyIdx = copyIdx + 1
    val writeRes: Stigi.File.WriteResult = outFile.writeChunk(4096, chunkBuf)
    val wroteLen: U64 = writeRes match
      case WroteBytes(wroteBytes) => wroteBytes
      case WriteFailed => 0
    if wroteLen == 0 then
      failed = true
    else if wroteLen <= 4096 then
      val nextPos: U64 = sent + wroteLen
      if nextPos <= srcLen then sent = nextPos else sent = srcLen
    else
      sent = srcLen
    stepGuard = stepGuard + 1
  if !failed && sent < srcLen then
    val tailLen: U64 = srcLen - sent
    if tailLen < 4096 then
      val Filled(tailBuf) = Array.tryFill[U8]{region}(tailLen, 0) else
        case FillFailed =>
          val _ = outFile.closeFile()
          false
      var tailIdx: U64 = 0
      while tailIdx < tailLen do
        @<lessOrEqual(tailIdx, tailLen)>
        val srcPos: U64 = sent + tailIdx
        if srcPos < srcCap then tailBuf(tailIdx) := srcBuf(srcPos) else ()
        tailIdx = tailIdx + 1
      val tailRes: Stigi.File.WriteResult = outFile.writeChunk(tailLen, tailBuf)
      val tailWrote: U64 = tailRes match
        case WroteBytes(wroteBytes) => wroteBytes
        case WriteFailed => 0
      val closeTail: Stigi.File.CloseResult = outFile.closeFile()
      val closedClean: Bool = closeTail match
        case ClosedClean => true
        case CloseFailed => false
      tailWrote == tailLen && closedClean
    else
      val _ = outFile.closeFile()
      false
  else
    val closeRes: Stigi.File.CloseResult = outFile.closeFile()
    val closedClean: Bool = closeRes match
      case ClosedClean => true
      case CloseFailed => false
    !failed && sent == srcLen && closedClean
