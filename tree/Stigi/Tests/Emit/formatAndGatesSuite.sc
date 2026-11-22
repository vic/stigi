// prompt: verify C mangling, source paths, decimal and hex formatting, line directives, static asserts, Spectre mask, digest sorting, and crash line scan.
val Stigi.Tests.Emit.formatAndGatesSuite {Stigi.Test}: Stigi.Test.Result =
  val zeroByte: U8 = 0
  val zeroWord: U64 = 0
  Array.tryFill{Stigi.Region}(2048, zeroByte) match
    case FillFailed => Stigi.Test.expect(false, true, "out alloc failed")
    case Filled(outBuf) =>
      val endLit: U64 = Stigi.Emit.writeLiteral(2, "/*", 2048, outBuf, 0)
      val endByte: U64 = Stigi.Emit.writeByte(2048, outBuf, endLit, 10)
      val endMangled: U64 = Stigi.Emit.writeMangledName(10, "Demo.Alpha", 2048, outBuf, endByte)
      val mangledValid: Bool = Stigi.Parse.sliceEquals(2048, outBuf, endByte, endMangled, 16, "Stigi_Demo_Alpha")
      val endPath: U64 = Stigi.Emit.writeSourcePath(10, "Demo.Alpha", 2048, outBuf, endMangled)
      val pathValid: Bool = Stigi.Parse.sliceEquals(2048, outBuf, endMangled, endPath, 13, "Demo/Alpha.sc")
      val endDec: U64 = Stigi.Emit.writeUnsignedDecimal{Stigi.Region}(4096, 2048, outBuf, endPath)
      val decValid: Bool = Stigi.Parse.sliceEquals(2048, outBuf, endPath, endDec, 4, "4096")
      val endZero: U64 = Stigi.Emit.writeUnsignedDecimal{Stigi.Region}(0, 2048, outBuf, endDec)
      val zeroValid: Bool = Stigi.Parse.sliceEquals(2048, outBuf, endDec, endZero, 1, "0")
      val digestAbc: Stigi.Hash.Digest = Stigi.Hash.hashBytes{Stigi.Region}(3, "abc", 3)
      val endHex: U64 = Stigi.Emit.writeHexDigest{Stigi.Region}(digestAbc, 2048, outBuf, endZero)
      val hexValid: Bool = endHex == endZero + 64 && Stigi.Parse.sliceEquals(2048, outBuf, endZero, endZero + 8, 8, "6437b3ac")
      val endLine: U64 = Stigi.Emit.writeLineDirective{Stigi.Region}(7, 10, "Demo.Alpha", 2048, outBuf, endHex)
      val lineValid: Bool = endLine > endHex
      val endAssert: U64 = Stigi.Emit.writeStaticAssertSizeAlign{Stigi.Region}(10, "Demo.Alpha", 16, 8, 2048, outBuf, endLine)
      val endMask: U64 = Stigi.Emit.writeSpectreMaskHelper(2048, outBuf, endAssert)
      Stigi.Emit.outputOffsetBounded(endAssert, endMask, 2048)
      val cleanScan: Bool = !Stigi.Emit.hasForbiddenCrashCall(2048, outBuf, endMask)
      Array.tryFill{Stigi.Region}(8, zeroByte) match
        case FillFailed => Stigi.Test.expect(false, true, "bad alloc failed")
        case Filled(badBuf) =>
          badBuf(0) := 32
          badBuf(1) := 101
          badBuf(2) := 120
          badBuf(3) := 105
          badBuf(4) := 116
          badBuf(5) := 40
          badBuf(6) := 48
          badBuf(7) := 41
          val catchesBad: Bool = Stigi.Emit.hasForbiddenCrashCall(8, badBuf, 8)
          val outcomeSample: Stigi.Emit.EmitOutcome = Emitted(endMask, 1, 0, 1)
          val outcomeValid: Bool = outcomeSample match
            case Emitted(byteCount, unitsEmitted, lemmasErased, spectreMasks) => byteCount == endMask && unitsEmitted == 1 && lemmasErased == 0 && spectreMasks == 1
            case EmitBufferFull => false
            case EmitCrashLineViolation => false
          Array.tryFill{Stigi.Region}(3, zeroWord) match
            case FillFailed => Stigi.Test.expect(false, true, "sort low alloc failed")
            case Filled(digestLow) =>
              Array.tryFill{Stigi.Region}(3, zeroWord) match
                case FillFailed => Stigi.Test.expect(false, true, "sort mid alloc failed")
                case Filled(digestMid) =>
                  Array.tryFill{Stigi.Region}(3, zeroWord) match
                    case FillFailed => Stigi.Test.expect(false, true, "sort high alloc failed")
                    case Filled(digestHigh) =>
                      Array.tryFill{Stigi.Region}(3, zeroWord) match
                        case FillFailed => Stigi.Test.expect(false, true, "sort top alloc failed")
                        case Filled(digestTop) =>
                          Array.tryFill{Stigi.Region}(3, zeroWord) match
                            case FillFailed => Stigi.Test.expect(false, true, "order alloc failed")
                            case Filled(order) =>
                              digestLow(0) := 30
                              digestLow(1) := 10
                              digestLow(2) := 20
                              Stigi.Emit.sortUnitsByDigest(3, digestLow, digestMid, digestHigh, digestTop, order)
                              val sortValid: Bool = order(0) == 1 && order(1) == 2 && order(2) == 0
                              val allPassed: Bool = mangledValid && pathValid && decValid && zeroValid && hexValid && lineValid && cleanScan && catchesBad && outcomeValid && sortValid
                              Stigi.Test.expect(allPassed, true, "emitter formatting, Spectre mask, digest sort, and crash line scan must pass")
