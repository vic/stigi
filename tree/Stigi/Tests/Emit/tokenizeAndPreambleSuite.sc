// prompt: verify columnar L0 tokenization and C runtime preamble and entry generation.
val Stigi.Tests.Emit.tokenizeAndPreambleSuite {Stigi.Test}: Stigi.Test.Result =
  val zeroByte: U8 = 0
  val zeroWord: U64 = 0
  val sampleSrc: Array[U8](42) = "// prompt: ok\npub def Demo.add(x: U64) = x"
  Array.tryFill{Stigi.Region}(32, zeroByte) match
    case FillFailed => Stigi.Test.expect(false, true, "tokKind alloc failed")
    case Filled(tokKind) =>
      Array.tryFill{Stigi.Region}(32, zeroWord) match
        case FillFailed => Stigi.Test.expect(false, true, "tokStart alloc failed")
        case Filled(tokStart) =>
          Array.tryFill{Stigi.Region}(32, zeroWord) match
            case FillFailed => Stigi.Test.expect(false, true, "tokEnd alloc failed")
            case Filled(tokEnd) =>
              Array.tryFill{Stigi.Region}(32, zeroWord) match
                case FillFailed => Stigi.Test.expect(false, true, "tokLine alloc failed")
                case Filled(tokLine) =>
                  Array.tryFill{Stigi.Region}(32, zeroWord) match
                    case FillFailed => Stigi.Test.expect(false, true, "tokCol alloc failed")
                    case Filled(tokCol) =>
                      val tokCount: U64 = Stigi.Emit.tokenizeSource(42, sampleSrc, 0, 42, 32, tokKind, tokStart, tokEnd, tokLine, tokCol)
                      val tokValid: Bool = tokCount == 11 && tokKind(0) == 1 && tokKind(2) == 2 && tokLine(0) == 2 && tokCol(0) == 0
                      Array.tryFill{Stigi.Region}(4096, zeroByte) match
                        case FillFailed => Stigi.Test.expect(false, true, "out alloc failed")
                        case Filled(outBuf) =>
                          val headerEnd: U64 = Stigi.Emit.writeLine(11, "/* stigi */", 4096, outBuf, 0)
                          val preEnd: U64 = Stigi.Emit.writeRuntimePreamble(4096, outBuf, headerEnd)
                          Array.tryFill{Stigi.Region}(1, zeroWord) match
                            case FillFailed => Stigi.Test.expect(false, true, "ord alloc failed")
                            case Filled(order) =>
                              Array.tryFill{Stigi.Region}(1, zeroWord) match
                                case FillFailed => Stigi.Test.expect(false, true, "ns alloc failed")
                                case Filled(nameStarts) =>
                                  Array.tryFill{Stigi.Region}(1, zeroWord) match
                                    case FillFailed => Stigi.Test.expect(false, true, "ne alloc failed")
                                    case Filled(nameEnds) =>
                                      Array.tryFill{Stigi.Region}(1, zeroByte) match
                                        case FillFailed => Stigi.Test.expect(false, true, "kc alloc failed")
                                        case Filled(kindCodes) =>
                                          order(0) := 0
                                          nameStarts(0) := 0
                                          nameEnds(0) := 10
                                          kindCodes(0) := 11
                                          val mainEnd: U64 = Stigi.Emit.writeRuntimeEntry(1, order, 10, "Demo.check", nameStarts, nameEnds, kindCodes, true, 4096, outBuf, preEnd)
                                          val preValid: Bool = preEnd > 1000 && mainEnd > preEnd && !Stigi.Emit.hasForbiddenCrashCall(4096, outBuf, mainEnd)
                                          val allPassed: Bool = tokValid && preValid
                                          Stigi.Test.expect(allPassed, true, "tokenizer and C runtime preamble and entry must pass")
