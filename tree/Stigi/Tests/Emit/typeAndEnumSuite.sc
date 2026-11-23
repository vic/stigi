// prompt: verify enum registration, balanced delimiter stepping, type emission, and topological enum ordering.
val Stigi.Tests.Emit.typeAndEnumSuite {Stigi.Test}: Stigi.Test.Result =
  val zeroByte: U8 = 0
  val zeroWord: U64 = 0
  val srcText: Array[U8](86) = "pub enum Demo.Outer:\n  case Wrap(inner: Demo.Inner)\npub enum Demo.Inner:\n  case Empty\n"
  val idxText: Array[U8](20) = "Demo.OuterDemo.Inner"
  Array.tryFill{Stigi.Region}(64, zeroByte) match
    case FillFailed => Stigi.Test.expect(false, true, "tok kind failed")
    case Filled(tokKind) =>
      Array.tryFill{Stigi.Region}(64, zeroWord) match
        case FillFailed => Stigi.Test.expect(false, true, "tok start failed")
        case Filled(tokStart) =>
          Array.tryFill{Stigi.Region}(64, zeroWord) match
            case FillFailed => Stigi.Test.expect(false, true, "tok end failed")
            case Filled(tokEnd) =>
              Array.tryFill{Stigi.Region}(64, zeroWord) match
                case FillFailed => Stigi.Test.expect(false, true, "tok line failed")
                case Filled(tokLine) =>
                  Array.tryFill{Stigi.Region}(64, zeroWord) match
                    case FillFailed => Stigi.Test.expect(false, true, "tok col failed")
                    case Filled(tokCol) =>
                      val tokCount: U64 = Stigi.Emit.tokenizeSource(86, srcText, 0, 52, 64, tokKind, tokStart, tokEnd, tokLine, tokCol)
                      val stepMatch: Bool = if 7 < tokCount then Stigi.Emit.stepPastBalanced(86, srcText, 64, tokStart, tokEnd, tokCount, 7, 40, 41) == 12 else false
                      val tokMatch: Bool = if 6 < tokCount then Stigi.Emit.tokenEquals(86, srcText, 64, tokStart, tokEnd, 6, 4, "Wrap") else false
                      val sliceMatch: Bool = Stigi.Emit.slicesEqual(20, idxText, 0, 10, 86, srcText, 9, 19)
                      Array.tryFill{Stigi.Region}(8, zeroWord) match
                        case FillFailed => Stigi.Test.expect(false, true, "case enum failed")
                        case Filled(caseEnum) =>
                          Array.tryFill{Stigi.Region}(8, zeroWord) match
                            case FillFailed => Stigi.Test.expect(false, true, "case name failed")
                            case Filled(caseNameStart) =>
                              Array.tryFill{Stigi.Region}(8, zeroWord) match
                                case FillFailed => Stigi.Test.expect(false, true, "case end failed")
                                case Filled(caseNameEnd) =>
                                  Array.tryFill{Stigi.Region}(8, zeroWord) match
                                    case FillFailed => Stigi.Test.expect(false, true, "case base failed")
                                    case Filled(caseBase) =>
                                      Array.tryFill{Stigi.Region}(8, zeroWord) match
                                        case FillFailed => Stigi.Test.expect(false, true, "case cnt failed")
                                        case Filled(caseCnt) =>
                                          Array.tryFill{Stigi.Region}(8, zeroWord) match
                                            case FillFailed => Stigi.Test.expect(false, true, "field failed")
                                            case Filled(fieldNameBeg) =>
                                              Array.tryFill{Stigi.Region}(8, zeroWord) match
                                                case FillFailed => Stigi.Test.expect(false, true, "field failed")
                                                case Filled(fieldNameEnd) =>
                                                  Array.tryFill{Stigi.Region}(8, zeroWord) match
                                                    case FillFailed => Stigi.Test.expect(false, true, "field failed")
                                                    case Filled(fieldTypeBeg) =>
                                                      Array.tryFill{Stigi.Region}(8, zeroWord) match
                                                        case FillFailed => Stigi.Test.expect(false, true, "field failed")
                                                        case Filled(fieldTypeEnd) =>
                                                          val EnumTotals(caseOne, fieldOne) = Stigi.Emit.registerEnumUnit(0, 86, srcText, 64, tokStart, tokEnd, tokCount, 8, caseEnum, caseNameStart, caseNameEnd, caseBase, caseCnt, 0, 8, fieldNameBeg, fieldNameEnd, fieldTypeBeg, fieldTypeEnd, 0)
                                                          val tokTwo: U64 = Stigi.Emit.tokenizeSource(86, srcText, 52, 86, 64, tokKind, tokStart, tokEnd, tokLine, tokCol)
                                                          val countsTwo: Stigi.Emit.RegCounts = if caseOne <= 8 && fieldOne <= 8 then Stigi.Emit.registerEnumUnit(1, 86, srcText, 64, tokStart, tokEnd, tokTwo, 8, caseEnum, caseNameStart, caseNameEnd, caseBase, caseCnt, caseOne, 8, fieldNameBeg, fieldNameEnd, fieldTypeBeg, fieldTypeEnd, fieldOne) else EnumTotals(0, 0)
                                                          val EnumTotals(caseTwo, fieldTwo) = countsTwo
                                                          Array.tryFill{Stigi.Region}(1024, zeroByte) match
                                                            case FillFailed => Stigi.Test.expect(false, true, "out failed")
                                                            case Filled(outBuf) =>
                                                              val typeLen: U64 = if 10 < tokCount then Stigi.Emit.emitTypeName(86, srcText, 64, tokStart, tokEnd, tokCount, 10, 1024, outBuf, 0) else 0
                                                              Array.tryFill{Stigi.Region}(2, zeroWord) match
                                                                case FillFailed => Stigi.Test.expect(false, true, "order failed")
                                                                case Filled(order) =>
                                                                  Array.tryFill{Stigi.Region}(2, zeroWord) match
                                                                    case FillFailed => Stigi.Test.expect(false, true, "name start failed")
                                                                    case Filled(nameStarts) =>
                                                                      Array.tryFill{Stigi.Region}(2, zeroWord) match
                                                                        case FillFailed => Stigi.Test.expect(false, true, "name end failed")
                                                                        case Filled(nameEnds) =>
                                                                          Array.tryFill{Stigi.Region}(2, zeroByte) match
                                                                            case FillFailed => Stigi.Test.expect(false, true, "kind codes failed")
                                                                            case Filled(kindCodes) =>
                                                                              order(0) := 0
                                                                              order(1) := 1
                                                                              nameStarts(0) := 0
                                                                              nameEnds(0) := 10
                                                                              nameStarts(1) := 10
                                                                              nameEnds(1) := 20
                                                                              kindCodes(0) := 1
                                                                              kindCodes(1) := 1
                                                                              val emittedLen: U64 = if caseTwo <= 8 && fieldTwo <= 8 then Stigi.Emit.emitAllEnumsOrdered{Stigi.Region}(2, order, 20, idxText, nameStarts, nameEnds, kindCodes, 86, srcText, 8, caseEnum, caseBase, caseCnt, caseTwo, 8, fieldTypeBeg, fieldTypeEnd, fieldTwo, 1024, outBuf, 0) else 0
                                                                              val good: Bool = stepMatch && tokMatch && sliceMatch && caseTwo == 2 && fieldTwo == 1 && typeLen == 16 && emittedLen > 100
                                                                              Stigi.Test.expect(good, true, "enum registry and ordered emitter must pass")
