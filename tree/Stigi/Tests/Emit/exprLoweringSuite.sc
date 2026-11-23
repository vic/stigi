// prompt: verify non-recursive L0 expression lowering across enum equality, inline conditionals, builtins, and array reads.
val Stigi.Tests.Emit.exprLoweringSuite {Stigi.Test}: Stigi.Test.Result =
  val zeroByte: U8 = 0
  val zeroWord: U64 = 0
  val srcText: Array[U8](100) = "pub enum Demo.Mode:\n  case Fast\n  case Slow\nif mode == Fast then U64.wrappingAdd(arr(idx), 1) else 0"
  val idxText: Array[U8](9) = "Demo.Mode"
  Array.tryFill{Stigi.Region}(64, zeroByte) match
    case FillFailed => Stigi.Test.expect(false, true, "tok failed")
    case Filled(tokKind) =>
      Array.tryFill{Stigi.Region}(64, zeroWord) match
        case FillFailed => Stigi.Test.expect(false, true, "tok failed")
        case Filled(tokStart) =>
          Array.tryFill{Stigi.Region}(64, zeroWord) match
            case FillFailed => Stigi.Test.expect(false, true, "tok failed")
            case Filled(tokEnd) =>
              Array.tryFill{Stigi.Region}(64, zeroWord) match
                case FillFailed => Stigi.Test.expect(false, true, "tok failed")
                case Filled(tokLine) =>
                  Array.tryFill{Stigi.Region}(64, zeroWord) match
                    case FillFailed => Stigi.Test.expect(false, true, "tok failed")
                    case Filled(tokCol) =>
                      val enumToks: U64 = Stigi.Emit.tokenizeSource(100, srcText, 0, 44, 64, tokKind, tokStart, tokEnd, tokLine, tokCol)
                      Array.tryFill{Stigi.Region}(4, zeroWord) match
                        case FillFailed => Stigi.Test.expect(false, true, "case failed")
                        case Filled(caseEnum) =>
                          Array.tryFill{Stigi.Region}(4, zeroWord) match
                            case FillFailed => Stigi.Test.expect(false, true, "case failed")
                            case Filled(caseNameBeg) =>
                              Array.tryFill{Stigi.Region}(4, zeroWord) match
                                case FillFailed => Stigi.Test.expect(false, true, "case failed")
                                case Filled(caseNameEnd) =>
                                  Array.tryFill{Stigi.Region}(4, zeroWord) match
                                    case FillFailed => Stigi.Test.expect(false, true, "case failed")
                                    case Filled(caseBase) =>
                                      Array.tryFill{Stigi.Region}(4, zeroWord) match
                                        case FillFailed => Stigi.Test.expect(false, true, "case failed")
                                        case Filled(caseCnt) =>
                                          Array.tryFill{Stigi.Region}(4, zeroWord) match
                                            case FillFailed => Stigi.Test.expect(false, true, "field failed")
                                            case Filled(scratchArr) =>
                                              val EnumTotals(caseTotal, _) = Stigi.Emit.registerEnumUnit(0, 100, srcText, 64, tokStart, tokEnd, enumToks, 4, caseEnum, caseNameBeg, caseNameEnd, caseBase, caseCnt, 0, 4, scratchArr, scratchArr, scratchArr, scratchArr, 0)
                                              val exprToks: U64 = Stigi.Emit.tokenizeSource(100, srcText, 44, 100, 64, tokKind, tokStart, tokEnd, tokLine, tokCol)
                                              Array.tryFill{Stigi.Region}(1, zeroWord) match
                                                case FillFailed => Stigi.Test.expect(false, true, "unit failed")
                                                case Filled(unitStarts) =>
                                                  Array.tryFill{Stigi.Region}(1, zeroWord) match
                                                    case FillFailed => Stigi.Test.expect(false, true, "unit failed")
                                                    case Filled(unitEnds) =>
                                                      Array.tryFill{Stigi.Region}(1, zeroByte) match
                                                        case FillFailed => Stigi.Test.expect(false, true, "kind failed")
                                                        case Filled(unitKinds) =>
                                                          Array.tryFill{Stigi.Region}(1, zeroWord) match
                                                            case FillFailed => Stigi.Test.expect(false, true, "ret failed")
                                                            case Filled(unitRet) =>
                                                              unitStarts(0) := 0
                                                              unitEnds(0) := 9
                                                              unitKinds(0) := 1
                                                              unitRet(0) := 1
                                                              Array.tryFill{Stigi.Region}(4, zeroWord) match
                                                                case FillFailed => Stigi.Test.expect(false, true, "loc failed")
                                                                case Filled(locNameBeg) =>
                                                                  Array.tryFill{Stigi.Region}(4, zeroWord) match
                                                                    case FillFailed => Stigi.Test.expect(false, true, "loc failed")
                                                                    case Filled(locNameEnd) =>
                                                                      Array.tryFill{Stigi.Region}(4, zeroWord) match
                                                                        case FillFailed => Stigi.Test.expect(false, true, "loc failed")
                                                                        case Filled(locTypeBeg) =>
                                                                          Array.tryFill{Stigi.Region}(4, zeroWord) match
                                                                            case FillFailed => Stigi.Test.expect(false, true, "loc failed")
                                                                            case Filled(locTypeEnd) =>
                                                                              Array.tryFill{Stigi.Region}(4, zeroByte) match
                                                                                case FillFailed => Stigi.Test.expect(false, true, "loc failed")
                                                                                case Filled(locArr) =>
                                                                                  locNameBeg(0) := 47
                                                                                  locNameEnd(0) := 51
                                                                                  locTypeBeg(0) := 9
                                                                                  locTypeEnd(0) := 18
                                                                                  locArr(0) := 0
                                                                                  locNameBeg(1) := 81
                                                                                  locNameEnd(1) := 84
                                                                                  locTypeBeg(1) := 9
                                                                                  locTypeEnd(1) := 18
                                                                                  locArr(1) := 1
                                                                                  Array.tryFill{Stigi.Region}(512, zeroByte) match
                                                                                    case FillFailed => Stigi.Test.expect(false, true, "out failed")
                                                                                    case Filled(outBuf) =>
                                                                                      val detMode: U64 = if 1 < exprToks && caseTotal <= 4 then Stigi.Emit.detectPrimaryEnumUnit(1, 9, idxText, unitStarts, unitEnds, unitKinds, unitRet, 0, 5, 100, srcText, 64, tokKind, tokStart, tokEnd, 1, 4, locNameBeg, locNameEnd, locTypeBeg, locTypeEnd, locArr, 2, 4, caseEnum, caseNameBeg, caseNameEnd, caseTotal) else 9
                                                                                      val outLen: U64 = if caseTotal <= 4 then Stigi.Emit.emitExprRange{Stigi.Region}(1, 9, idxText, unitStarts, unitEnds, unitKinds, unitRet, 0, 5, 100, srcText, 64, tokKind, tokStart, tokEnd, exprToks, 0, exprToks, 4, locNameBeg, locNameEnd, locTypeBeg, locTypeEnd, locArr, 2, 4, caseEnum, caseNameBeg, caseNameEnd, caseTotal, 512, outBuf, 0) else 0
                                                                                      val good: Bool = detMode == 0 && outLen > 80 && !Stigi.Emit.hasForbiddenCrashCall(512, outBuf, outLen)
                                                                                      Stigi.Test.expect(good, true, "expression lowerer must emit valid C11 expression")
