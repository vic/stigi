// prompt: verify all protocol frame classifications, S114 component splitting, and NegTest code checks.
val Stigi.Tests.Door.framesAndSplitSuite {Stigi.Test}: Stigi.Test.Result =
  val frameChecksOne: Bool = Stigi.Door.classifyFrame(5, "batch") == FrameBatch && Stigi.Door.classifyFrame(5, "check") == FrameCheck && Stigi.Door.classifyFrame(6, "accept") == FrameAccept && Stigi.Door.classifyFrame(7, "summary") == FrameSummary && Stigi.Door.classifyFrame(6, "budget") == FrameBudget && Stigi.Door.classifyFrame(6, "submit") == FrameSubmit && Stigi.Door.classifyFrame(4, "drop") == FrameDrop && Stigi.Door.classifyFrame(4, "show") == FrameShow && Stigi.Door.classifyFrame(4, "find") == FrameFind && Stigi.Door.classifyFrame(4, "list") == FrameList
  val frameChecksTwo: Bool = Stigi.Door.classifyFrame(5, "query") == FrameQuery && Stigi.Door.classifyFrame(4, "laws") == FrameLaws && Stigi.Door.classifyFrame(5, "store") == FrameStore && Stigi.Door.classifyFrame(4, "load") == FrameLoad && Stigi.Door.classifyFrame(6, "export") == FrameExport && Stigi.Door.classifyFrame(4, "emit") == FrameEmit && Stigi.Door.classifyFrame(5, "tests") == FrameTests && Stigi.Door.classifyFrame(4, "json") == FrameJson && Stigi.Door.classifyFrame(4, "stop") == FrameStop && Stigi.Door.classifyFrame(7, "invalid") == FrameUnknown
  val zeroWord: U64 = 0
  Array.tryFill{Stigi.Region}(5, zeroWord) match
    case FillFailed => Stigi.Test.expect(false, true, "parent alloc failed")
    case Filled(parentOf) =>
      Array.tryFill{Stigi.Region}(3, zeroWord) match
        case FillFailed => Stigi.Test.expect(false, true, "from alloc failed")
        case Filled(edgeFrom) =>
          Array.tryFill{Stigi.Region}(3, zeroWord) match
            case FillFailed => Stigi.Test.expect(false, true, "to alloc failed")
            case Filled(edgeTo) =>
              edgeFrom(0) := 0
              edgeTo(0) := 1
              edgeFrom(1) := 1
              edgeTo(1) := 2
              edgeFrom(2) := 3
              edgeTo(2) := 4
              val compCount: U64 = Stigi.Door.splitComponents(5, parentOf, 3, edgeFrom, edgeTo)
              Stigi.Door.componentCountBounded(compCount, 5, 5)
              val splitValid: Bool = compCount == 2 && parentOf(0) == 0 && parentOf(2) == 0 && parentOf(3) == 3 && parentOf(4) == 3
              Array.tryFill{Stigi.Region}(4, zeroWord) match
                case FillFailed => Stigi.Test.expect(false, true, "exp alloc failed")
                case Filled(expectedCodes) =>
                  Array.tryFill{Stigi.Region}(4, zeroWord) match
                    case FillFailed => Stigi.Test.expect(false, true, "act alloc failed")
                    case Filled(actualCodes) =>
                      expectedCodes(0) := 11
                      actualCodes(0) := 11
                      actualCodes(1) := 11
                      val negValid: Bool = Stigi.Door.checkNegTest(4, expectedCodes, 1, 4, actualCodes, 1) == NegTestPassed
                      val negNone: Bool = Stigi.Door.checkNegTest(4, expectedCodes, 1, 4, actualCodes, 0) == NegTestNotRejected
                      val negDup: Bool = Stigi.Door.checkNegTest(4, expectedCodes, 1, 4, actualCodes, 2) == NegTestMultipleSubjects
                      actualCodes(0) := 99
                      val negWrong: Bool = Stigi.Door.checkNegTest(4, expectedCodes, 1, 4, actualCodes, 1) == NegTestWrongCode
                      val allPassed: Bool = frameChecksOne && frameChecksTwo && splitValid && negValid && negNone && negDup && negWrong
                      Stigi.Test.expect(allPassed, true, "door frames, S114 split, and NegTest code checks must pass")
