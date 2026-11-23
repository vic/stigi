// prompt: verify POSIX struct decoders, path NUL termination, and typed syscall result classifiers.
val Stigi.Tests.Posix.decodeAndContractSuite {Stigi.Test}: Stigi.Test.Result =
  val zeroByte: U8 = 0
  val samplePath: Array[U8](11) = "/tmp/sample"
  Stigi.Posix.pathLengthWithinLimit(11, 512)
  Array.tryFill{Stigi.Region}(144, zeroByte) match
    case FillFailed => Stigi.Test.expect(false, true, "usage alloc failed")
    case Filled(usageBuf) =>
      Array.tryFill{Stigi.Region}(16, zeroByte) match
        case FillFailed => Stigi.Test.expect(false, true, "clock alloc failed")
        case Filled(clockBuf) =>
          Array.tryFill{Stigi.Region}(512, zeroByte) match
            case FillFailed => Stigi.Test.expect(false, true, "path alloc failed")
            case Filled(pathBuf) =>
              usageBuf(32) := 40
              usageBuf(33) := 1
              usageBuf(64) := 7
              usageBuf(72) := 2
              clockBuf(0) := 10
              clockBuf(8) := 99
              val usageGood: Bool = Stigi.Posix.decodeUsageBuffer(usageBuf) match
                case UsageFailed => false
                case UsageData(rssVal, minVal, majVal) => rssVal == 296 && minVal == 7 && majVal == 2
              val clockGood: Bool = Stigi.Posix.decodeClockBuffer(clockBuf) match
                case ClockFailed => false
                case ClockData(secVal, nanoVal) => secVal == 10 && nanoVal == 99
              val copied: Bool = Stigi.Posix.copyNullTerminatedPath(11, samplePath, 0, 11, pathBuf)
              val nulGood: Bool = copied && pathBuf(0) == 47 && pathBuf(10) == 101 && pathBuf(11) == 0
              val openFailGood: Bool = Stigi.Posix.classifyOpenResult(-1) match
                case OpenFailed => true
                case FileOpened(_) => false
              val openHitGood: Bool = Stigi.Posix.classifyOpenResult(5) match
                case OpenFailed => false
                case FileOpened(descVal) => descVal == 5
              val readHitGood: Bool = Stigi.Posix.classifyReadResult(42, 64) match
                case ReadFailed => false
                case ReadBytes(cntVal) => cntVal == 42
              val writeHitGood: Bool = Stigi.Posix.classifyWriteResult(16, 64) match
                case WriteFailed => false
                case WroteBytes(cntVal) => cntVal == 16
              val allGood: Bool = usageGood && clockGood && nulGood && openFailGood && openHitGood && readHitGood && writeHitGood
              Stigi.Test.expect(allGood, true, "Stigi.Posix decoders and classifiers must pass")
