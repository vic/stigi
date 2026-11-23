// prompt: verify def signature scanning, parameter collection, C header emission, and top-level val emission.
val Stigi.Tests.Emit.headerAndValSuite {Stigi.Test}: Stigi.Test.Result =
  val zeroByte: U8 = 0
  val zeroWord: U64 = 0
  val defSrc: Array[U8](76) = "pub def Demo.run(len: U64, buf: Array[U8](len)) <below(len, 64)>: U64 = len\n"
  val valSrc: Array[U8](31) = "pub val Demo.limit: U64 = 1000\n"
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
                      val defToks: U64 = Stigi.Emit.tokenizeSource(76, defSrc, 0, 76, 64, tokKind, tokStart, tokEnd, tokLine, tokCol)
                      val DefSignature(paramListStart, afterParams, returnTok, hasReturnTok, bodyTok) = Stigi.Emit.scanDefSignature(76, defSrc, 64, tokStart, tokEnd, defToks)
                      Array.tryFill{Stigi.Region}(16, zeroWord) match
                        case FillFailed => Stigi.Test.expect(false, true, "local failed")
                        case Filled(localNameBeg) =>
                          Array.tryFill{Stigi.Region}(16, zeroWord) match
                            case FillFailed => Stigi.Test.expect(false, true, "local failed")
                            case Filled(localNameEnd) =>
                              Array.tryFill{Stigi.Region}(16, zeroWord) match
                                case FillFailed => Stigi.Test.expect(false, true, "local failed")
                                case Filled(localTypeBeg) =>
                                  Array.tryFill{Stigi.Region}(16, zeroWord) match
                                    case FillFailed => Stigi.Test.expect(false, true, "local failed")
                                    case Filled(localTypeEnd) =>
                                      Array.tryFill{Stigi.Region}(16, zeroByte) match
                                        case FillFailed => Stigi.Test.expect(false, true, "local failed")
                                        case Filled(localArr) =>
                                          Array.tryFill{Stigi.Region}(512, zeroByte) match
                                            case FillFailed => Stigi.Test.expect(false, true, "out failed")
                                            case Filled(outBuf) =>
                                              val paramCount: U64 = if paramListStart <= afterParams && afterParams <= defToks then Stigi.Emit.collectFunctionParams(76, defSrc, 64, tokStart, tokEnd, defToks, paramListStart, afterParams, 16, localNameBeg, localNameEnd, localTypeBeg, localTypeEnd, localArr) else 0
                                              val defHdrLen: U64 = if paramListStart <= afterParams && afterParams <= defToks then Stigi.Emit.emitFunctionHeader(8, "Demo.run", 0, 8, 76, defSrc, 64, tokStart, tokEnd, defToks, paramListStart, afterParams, returnTok, hasReturnTok, true, 512, outBuf, 0) else 0
                                              val defGood: Bool = hasReturnTok && bodyTok < defToks && paramCount == 2 && localArr(0) == 0 && localArr(1) == 1 && defHdrLen > 40
                                              val valToks: U64 = Stigi.Emit.tokenizeSource(31, valSrc, 0, 31, 64, tokKind, tokStart, tokEnd, tokLine, tokCol)
                                              val DefSignature(valParamStart, valAfterParam, valRetTok, valHasRet, valBodyTok) = Stigi.Emit.scanDefSignature(31, valSrc, 64, tokStart, tokEnd, valToks)
                                              val valOutLen: U64 = Stigi.Emit.emitTopLevelVal(10, "Demo.limit", 0, 10, 31, valSrc, 64, tokKind, tokStart, tokEnd, valToks, valRetTok, valHasRet, valBodyTok, 512, outBuf, defHdrLen)
                                              val valGood: Bool = valParamStart == valAfterParam && valHasRet && valOutLen > defHdrLen + 20
                                              val allGood: Bool = defGood && valGood
                                              Stigi.Test.expect(allGood, true, "function header and top-level val emitter must pass")
