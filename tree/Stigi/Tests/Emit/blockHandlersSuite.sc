// prompt: verify block stack push, close-to-column freeing, tryFill block lowering, and enum match block lowering.
val Stigi.Tests.Emit.blockHandlersSuite {Stigi.Test}: Stigi.Test.Result =
  val zeroByte: U8 = 0
  val zeroWord: U64 = 0
  val srcText: Array[U8](110) = "pub enum Demo.Mode:\n  case Fast\n  case Slow\nArray.tryFill{region}(8, zeroByte) match\nmode match\n  case Fast =>"
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
                      val enumToks: U64 = Stigi.Emit.tokenizeSource(110, srcText, 0, 44, 64, tokKind, tokStart, tokEnd, tokLine, tokCol)
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
                                            case FillFailed => Stigi.Test.expect(false, true, "scratch failed")
                                            case Filled(scratchArr) =>
                                              Array.tryFill{Stigi.Region}(4, zeroByte) match
                                                case FillFailed => Stigi.Test.expect(false, true, "loc arr failed")
                                                case Filled(locArr) =>
                                                  val EnumTotals(caseTotal, _) = Stigi.Emit.registerEnumUnit(0, 110, srcText, 64, tokStart, tokEnd, enumToks, 4, caseEnum, caseNameBeg, caseNameEnd, caseBase, caseCnt, 0, 4, scratchArr, scratchArr, scratchArr, scratchArr, 0)
                                                  val bodyToks: U64 = Stigi.Emit.tokenizeSource(110, srcText, 44, 110, 64, tokKind, tokStart, tokEnd, tokLine, tokCol)
                                                  val lineOneEnd: U64 = Stigi.Emit.findLineEnd(64, tokKind, bodyToks, 0)
                                                  val lineTwoStart: U64 = if lineOneEnd + 1 <= bodyToks then lineOneEnd + 1 else bodyToks
                                                  val lineTwoEnd: U64 = Stigi.Emit.findLineEnd(64, tokKind, bodyToks, lineTwoStart)
                                                  Array.tryFill{Stigi.Region}(32, zeroByte) match
                                                    case FillFailed => Stigi.Test.expect(false, true, "blk failed")
                                                    case Filled(blkKinds) =>
                                                      Array.tryFill{Stigi.Region}(32, zeroWord) match
                                                        case FillFailed => Stigi.Test.expect(false, true, "blk failed")
                                                        case Filled(blkCols) =>
                                                          Array.tryFill{Stigi.Region}(32, zeroByte) match
                                                            case FillFailed => Stigi.Test.expect(false, true, "blk failed")
                                                            case Filled(blkFlags) =>
                                                              Array.tryFill{Stigi.Region}(32, zeroWord) match
                                                                case FillFailed => Stigi.Test.expect(false, true, "blk failed")
                                                                case Filled(blkSaved) =>
                                                                  Array.tryFill{Stigi.Region}(32, zeroWord) match
                                                                    case FillFailed => Stigi.Test.expect(false, true, "blk failed")
                                                                    case Filled(blkTemps) =>
                                                                      Array.tryFill{Stigi.Region}(32, zeroWord) match
                                                                        case FillFailed => Stigi.Test.expect(false, true, "blk failed")
                                                                        case Filled(blkAuxBeg) =>
                                                                          Array.tryFill{Stigi.Region}(32, zeroWord) match
                                                                            case FillFailed => Stigi.Test.expect(false, true, "blk failed")
                                                                            case Filled(blkAuxEnd) =>
                                                                              Array.tryFill{Stigi.Region}(32, zeroWord) match
                                                                                case FillFailed => Stigi.Test.expect(false, true, "blk failed")
                                                                                case Filled(blkPrev) =>
                                                                                  Array.tryFill{Stigi.Region}(1, zeroWord) match
                                                                                    case FillFailed => Stigi.Test.expect(false, true, "box failed")
                                                                                    case Filled(blkTopBox) =>
                                                                                      Array.tryFill{Stigi.Region}(1, zeroWord) match
                                                                                        case FillFailed => Stigi.Test.expect(false, true, "box failed")
                                                                                        case Filled(tempBox) =>
                                                                                          Array.tryFill{Stigi.Region}(1, zeroWord) match
                                                                                            case FillFailed => Stigi.Test.expect(false, true, "box failed")
                                                                                            case Filled(locBox) =>
                                                                                              Array.tryFill{Stigi.Region}(4, zeroWord) match
                                                                                                case FillFailed => Stigi.Test.expect(false, true, "tgt failed")
                                                                                                case Filled(tgtBox) =>
                                                                                                  Array.tryFill{Stigi.Region}(512, zeroByte) match
                                                                                                    case FillFailed => Stigi.Test.expect(false, true, "out failed")
                                                                                                    case Filled(outBuf) =>
                                                                                                      blkTopBox(0) := 32
                                                                                                      Array.tryFill{Stigi.Region}(1, zeroWord) match
                                                                                                        case FillFailed => Stigi.Test.expect(false, true, "u failed")
                                                                                                        case Filled(unitStarts) =>
                                                                                                          Array.tryFill{Stigi.Region}(1, zeroWord) match
                                                                                                            case FillFailed => Stigi.Test.expect(false, true, "u failed")
                                                                                                            case Filled(unitEnds) =>
                                                                                                              Array.tryFill{Stigi.Region}(1, zeroByte) match
                                                                                                                case FillFailed => Stigi.Test.expect(false, true, "u failed")
                                                                                                                case Filled(unitKinds) =>
                                                                                                                  unitStarts(0) := 0
                                                                                                                  unitEnds(0) := 9
                                                                                                                  unitKinds(0) := 1
                                                                                                                  val posOne: U64 = if 0 < lineOneEnd && caseTotal <= 4 then Stigi.Emit.startTryFillBlock{Stigi.Region}(1, 9, idxText, unitStarts, unitEnds, unitKinds, unitStarts, 0, 5, 110, srcText, 64, tokKind, tokStart, tokEnd, bodyToks, 0, lineOneEnd, 0, 1, 4, scratchArr, scratchArr, scratchArr, scratchArr, locArr, locBox, 4, caseEnum, caseNameBeg, caseNameEnd, caseTotal, blkKinds, blkCols, blkFlags, blkSaved, blkTemps, blkAuxBeg, blkAuxEnd, blkPrev, blkTopBox, tempBox, 512, outBuf, 0) else 0
                                                                                                                  val posTwo: U64 = if lineTwoStart + 1 < lineTwoEnd && caseTotal <= 4 then Stigi.Emit.startMatchBlock{Stigi.Region}(1, 9, idxText, unitStarts, unitEnds, unitKinds, unitStarts, 0, 5, 110, srcText, 64, tokKind, tokStart, tokEnd, bodyToks, lineTwoStart, lineTwoStart + 1, lineTwoEnd, 2, 1, 4, scratchArr, scratchArr, scratchArr, scratchArr, locArr, locBox, 4, caseEnum, caseNameBeg, caseNameEnd, caseTotal, blkKinds, blkCols, blkFlags, blkSaved, blkTemps, blkAuxBeg, blkAuxEnd, blkPrev, blkTopBox, tempBox, 512, outBuf, posOne) else posOne
                                                                                                                  val posThree: U64 = Stigi.Emit.closeBlocksToCol{Stigi.Region}(blkKinds, blkCols, blkSaved, blkTemps, blkPrev, blkTopBox, locBox, tgtBox, 0, false, true, 512, outBuf, posTwo)
                                                                                                                  val good: Bool = posOne > 120 && posTwo > posOne + 30 && posThree > posTwo && blkTopBox(0) == 32
                                                                                                                  Stigi.Test.expect(good, true, "block handlers must lower tryFill and match cleanly")
