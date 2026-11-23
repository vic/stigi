// prompt: read all bytes from an opened descriptor via typestate Stigi.File and close it once.
def Stigi.Posix.readFromDesc {region: Stigi.Region, Stigi.Mut} (desc: I32, outCap: Stigi.ByteCap, outBuf: Array[U8](outCap), startOff: U64) <lessOrEqual(startOff, outCap)>: Stigi.Posix.LoadResult =
  handle inFile = Stigi.Posix.File(desc)
  val Filled(chunkBuf) = Array.tryFill[U8]{region}(4096, 0) else
    case FillFailed =>
      val _ = inFile.closeFile()
      LoadFailed
  var writePos: U64 = startOff
  var totalRead: U64 = 0
  var finished: Bool = false
  var failed: Bool = false
  var overflowed: Bool = false
  var stepGuard: U64 = 0
  while !finished && !failed && !overflowed && stepGuard < 65536 do
    @<lessOrEqual(startOff, writePos), lessOrEqual(writePos, outCap), lessOrEqual(totalRead, outCap), lessOrEqual(stepGuard, 65536)>
    val readRes: Stigi.File.ReadResult = inFile.readChunk(4096, chunkBuf)
    val gotLen: U64 = readRes match
      case ReadBytes(gotBytes) => gotBytes
      case ReadEnd => 0
      case ReadFailed => 0
    val isEnd: Bool = readRes match
      case ReadBytes(_) => false
      case ReadEnd => true
      case ReadFailed => false
    val isErr: Bool = readRes match
      case ReadBytes(_) => false
      case ReadEnd => false
      case ReadFailed => true
    if isErr then
      failed = true
    else if isEnd then
      finished = true
    else if gotLen <= 4096 then
      var copyIdx: U64 = 0
      while copyIdx < gotLen && !overflowed do
        @<lessOrEqual(copyIdx, gotLen), lessOrEqual(startOff, writePos), lessOrEqual(writePos, outCap), lessOrEqual(totalRead, outCap)>
        if writePos < outCap && totalRead < outCap then
          outBuf(writePos) := chunkBuf(copyIdx)
          writePos = writePos + 1
          totalRead = totalRead + 1
        else
          overflowed = true
        copyIdx = copyIdx + 1
    else
      failed = true
    stepGuard = stepGuard + 1
  val closeRes: Stigi.File.CloseResult = inFile.closeFile()
  val closedClean: Bool = closeRes match
    case ClosedClean => true
    case CloseFailed => false
  if overflowed then LoadOverflow else if failed || !closedClean then LoadFailed else LoadedBytes(totalRead)
