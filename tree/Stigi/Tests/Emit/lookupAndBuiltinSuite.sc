// prompt: verify symbol lookup, enum case resolution, primary span scanning, and builtin C fragment emission.
val Stigi.Tests.Emit.lookupAndBuiltinSuite {Stigi.Test}: Stigi.Test.Result =
  val zeroByte: U8 = 0
  val zeroWord: U64 = 0
  val srcText: Array[U8](63) = "pub enum Demo.Mode:\n  case Fast\n  case Slow\nU64.shiftLeft(x, 4)"
  val idxText: Array[U8](9) = "Demo.Mode"
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
                      val tokCount: U64 = Stigi.Emit.tokenizeSource(63, srcText, 0, 44, 32, tokKind, tokStart, tokEnd, tokLine, tokCol)
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
                                            case Filled(unusedOne) =>
                                              val EnumTotals(caseTotal, _) = Stigi.Emit.registerEnumUnit(0, 63, srcText, 32, tokStart, tokEnd, tokCount, 4, caseEnum, caseNameBeg, caseNameEnd, caseBase, caseCnt, 0, 4, unusedOne, unusedOne, unusedOne, unusedOne, 0)
                                              Array.tryFill{Stigi.Region}(1, zeroWord) match
                                                case FillFailed => Stigi.Test.expect(false, true, "unit failed")
                                                case Filled(unitStarts) =>
                                                  Array.tryFill{Stigi.Region}(1, zeroWord) match
                                                    case FillFailed => Stigi.Test.expect(false, true, "unit failed")
                                                    case Filled(unitEnds) =>
                                                      unitStarts(0) := 0
                                                      unitEnds(0) := 9
                                                      val unitHit: U64 = Stigi.Emit.findUnitByName(1, 9, idxText, unitStarts, unitEnds, 63, srcText, 9, 18)
                                                      val locHit: U64 = if caseTotal <= 4 then Stigi.Emit.findLocalIndex(63, srcText, 39, 43, 4, caseNameBeg, caseNameEnd, caseTotal) else 9
                                                      val CaseHit(caseFound, _, enumHit, tagHit) = if caseTotal <= 4 then Stigi.Emit.findEnumCase(1, 9, idxText, unitStarts, unitEnds, 0, 5, 63, srcText, 39, 43, 4, caseEnum, caseNameBeg, caseNameEnd, caseTotal) else CaseHit(false, 0, 0, 0)
                                                      val callToks: U64 = Stigi.Emit.tokenizeSource(63, srcText, 44, 63, 32, tokKind, tokStart, tokEnd, tokLine, tokCol)
                                                      val primEnd: U64 = if 0 < callToks then Stigi.Emit.scanPrimaryEnd(63, srcText, 32, tokStart, tokEnd, callToks, 0, callToks) else 0
                                                      val BuiltinInfo(shiftFrame, shiftSub) = Stigi.Emit.classifyBuiltinCall(63, srcText, 44, 57)
                                                      Array.tryFill{Stigi.Region}(128, zeroByte) match
                                                        case FillFailed => Stigi.Test.expect(false, true, "out failed")
                                                        case Filled(outBuf) =>
                                                          val posOne: U64 = Stigi.Emit.emitBuiltinOpen(shiftFrame, shiftSub, 128, outBuf, 0)
                                                          val posTwo: U64 = Stigi.Emit.emitBuiltinComma(shiftFrame, shiftSub, 0, 128, outBuf, posOne)
                                                          val posThree: U64 = Stigi.Emit.emitBuiltinClose(shiftFrame, 128, outBuf, posTwo)
                                                          val allGood: Bool = unitHit == 0 && locHit == 1 && caseFound && enumHit == 0 && tagHit == 1 && primEnd == 6 && shiftFrame == 10 && shiftSub == 1 && posThree == 40
                                                          Stigi.Test.expect(allGood, true, "lookup and builtin emission must pass")
