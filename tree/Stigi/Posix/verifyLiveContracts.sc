// prompt: verify live POSIX contracts for clock_gettime, getrusage, openat, write, lseek, read, close, and unlink.
pub def Stigi.Posix.verifyLiveContracts {region: Stigi.Region, Stigi.Mut} () <lessOrEqual(0, 1)>: Bool =
  val zeroByte: U8 = 0
  val zeroWord: U64 = 0
  val probePath: Array[U8](24) = "/tmp/stigi-s39-probe.tmp"
  val probePayload: Array[U8](12) = "stigi-s39-ok"
  val clockGood: Bool = Stigi.Posix.readMonotonicClock{region}() match
    case ClockFailed => false
    case ClockData(secOne, nanoOne) =>
      Stigi.Posix.readMonotonicClock{region}() match
        case ClockFailed => false
        case ClockData(secTwo, nanoTwo) =>
          secTwo > secOne || secTwo == secOne && nanoTwo >= nanoOne
  val usageGood: Bool = Stigi.Posix.readSelfUsage{region}() match
    case UsageFailed => false
    case UsageData(maxRss, _, _) => maxRss > 0
  val fileGood: Bool = Array.tryFill{region}(512, zeroByte) match
    case FillFailed => false
    case Filled(pathBuf) =>
      Array.tryFill{region}(65536, zeroByte) match
        case FillFailed => false
        case Filled(chunkBuf) =>
          Array.tryFill{region}(2, zeroWord) match
            case FillFailed => false
            case Filled(stateBox) =>
              Array.tryFill{region}(64, zeroByte) match
                case FillFailed => false
                case Filled(readBack) =>
                  val wroteGood: Bool = Stigi.Posix.openWriteTruncWithBuf(24, probePath, 0, 24, pathBuf) match
                    case OpenFailed => false
                    case FileOpened(writeDesc) =>
                      val writeRes: Bool = Stigi.Posix.writeAllFromSlice{region}(writeDesc, 12, probePayload, 0, 12) match
                        case WriteFailed => false
                        case WroteBytes(wroteLen) => wroteLen == 12
                      val closedWrite: Bool = Stigi.Posix.closeDescriptor(writeDesc)
                      writeRes && closedWrite
                  val readGood: Bool = if wroteGood then
                    Stigi.Posix.openReadWithBuf(24, probePath, 0, 24, pathBuf) match
                      case OpenFailed => false
                      case FileOpened(readDesc) =>
                        val firstRead: Bool = Stigi.Posix.readAllWithChunk(readDesc, chunkBuf, stateBox, 64, readBack, 0) match
                          case ReadFailed => false
                          case ReadBytes(gotLen) => gotLen == 12
                        val rewound: Bool = Stigi.Posix.seekToStart(readDesc)
                        val secondRead: Bool = Stigi.Posix.readAllWithChunk(readDesc, chunkBuf, stateBox, 64, readBack, 0) match
                          case ReadFailed => false
                          case ReadBytes(gotLen) => gotLen == 12 && Stigi.Emit.slicesEqual(64, readBack, 0, 12, 12, probePayload, 0, 12)
                        val closedRead: Bool = Stigi.Posix.closeDescriptor(readDesc)
                        firstRead && rewound && secondRead && closedRead
                  else
                    false
                  val removed: Bool = Stigi.Posix.unlinkWithBuf(24, probePath, 0, 24, pathBuf)
                  wroteGood && readGood && removed
  clockGood && usageGood && fileGood
