// prompt: verify line-end scanning, sibling detection, single-line if lowering, tail return frees, and pattern bindings.
val Stigi.Tests.Emit.stmtHelpersSuite {Stigi.Test}: Stigi.Test.Result =
  val zeroByte: U8 = 0
  val zeroWord: U64 = 0
  val sampleDef: Array[U8](32) = "if x < 5 then y = x + 1 else ()\n"
  Array.tryFill{Stigi.Region}(32, zeroByte) match
    case FillFailed => Stigi.Test.expect(false, true, "tok failed")
    case Filled(tokKind) =>
      Array.tryFill{Stigi.Region}(32, zeroWord) match
        case FillFailed => Stigi.Test.expect(false, true, "tok failed")
        case Filled(tokStart) =>
          Array.tryFill{Stigi.Region}(32, zeroWord) match
            case FillFailed => Stigi.Test.expect(false, true, "tok failed")
            case Filled(tokEnd) =>
              Array.tryFill{Stigi.Region}(32, zeroWord) match
                case FillFailed => Stigi.Test.expect(false, true, "tok failed")
                case Filled(tokLine) =>
                  Array.tryFill{Stigi.Region}(32, zeroWord) match
                    case FillFailed => Stigi.Test.expect(false, true, "tok failed")
                    case Filled(tokCol) =>
                      val tokCount: U64 = Stigi.Emit.tokenizeSource(32, sampleDef, 0, 32, 32, tokKind, tokStart, tokEnd, tokLine, tokCol)
                      val ifLineEnd: U64 = Stigi.Emit.findLineEnd(32, tokKind, tokCount, 0)
                      val thenPos: U64 = Stigi.Emit.findTokenInLine(32, sampleDef, 32, tokStart, tokEnd, tokCount, 0, ifLineEnd, 4, "then")
                      val hasSib: Bool = Stigi.Emit.hasSiblingAfterBlock(32, sampleDef, 32, tokKind, tokStart, tokEnd, tokCol, tokCount, ifLineEnd, 0)
                      Array.tryFill{Stigi.Region}(1, zeroWord) match
                        case FillFailed => Stigi.Test.expect(false, true, "u failed")
                        case Filled(unitArr) =>
                          Array.tryFill{Stigi.Region}(1, zeroByte) match
                            case FillFailed => Stigi.Test.expect(false, true, "k failed")
                            case Filled(kindArr) =>
                              Array.tryFill{Stigi.Region}(96, zeroByte) match
                                case FillFailed => Stigi.Test.expect(false, true, "b failed")
                                case Filled(blkKinds) =>
                                  Array.tryFill{Stigi.Region}(96, zeroWord) match
                                    case FillFailed => Stigi.Test.expect(false, true, "b failed")
                                    case Filled(blkTemps) =>
                                      Array.tryFill{Stigi.Region}(96, zeroWord) match
                                        case FillFailed => Stigi.Test.expect(false, true, "b failed")
                                        case Filled(blkPrev) =>
                                          Array.tryFill{Stigi.Region}(256, zeroByte) match
                                            case FillFailed => Stigi.Test.expect(false, true, "o failed")
                                            case Filled(outBuf) =>
                                              blkKinds(0) := 7
                                              blkTemps(0) := 1
                                              blkPrev(0) := 96
                                              val posOne: U64 = if 0 < ifLineEnd then Stigi.Emit.emitSingleLineIfStmt{Stigi.Region}(1, 1, " ", unitArr, unitArr, kindArr, unitArr, 0, 0, 32, sampleDef, 32, tokKind, tokStart, tokEnd, tokCount, 0, ifLineEnd, 1, unitArr, unitArr, unitArr, unitArr, kindArr, 0, 1, unitArr, unitArr, unitArr, 0, 256, outBuf, 0) else 0
                                              val posTwo: U64 = if 2 <= tokCount then Stigi.Emit.emitTailReturn{Stigi.Region}(1, 1, " ", unitArr, unitArr, kindArr, unitArr, 0, 0, 32, sampleDef, 32, tokKind, tokStart, tokEnd, tokCount, 1, 2, 0, false, false, false, 0, 0, 1, unitArr, unitArr, unitArr, unitArr, kindArr, 0, 1, unitArr, unitArr, unitArr, 0, blkKinds, blkTemps, blkPrev, 0, 256, outBuf, posOne) else posOne
                                              val posThree: U64 = Stigi.Emit.emitPatternBindings{Stigi.Region}(32, sampleDef, 32, tokStart, tokEnd, tokCount, 0, 0, 1, unitArr, unitArr, 0, 0, 0, 0, true, 1, 1, unitArr, unitArr, unitArr, unitArr, kindArr, unitArr, 256, outBuf, posTwo)
                                              val good: Bool = thenPos == 4 && !hasSib && posOne > 25 && posTwo > posOne + 20 && posThree == posTwo && !Stigi.Emit.hasForbiddenCrashCall(256, outBuf, posThree)
                                              Stigi.Test.expect(good, true, "statement helpers must emit valid C11 statements")
