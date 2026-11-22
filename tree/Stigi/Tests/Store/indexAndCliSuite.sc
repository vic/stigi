// prompt: verify stored index parsing, corrupt rejection, hex digest decoding, path construction, and CLI cmdline parsing.
val Stigi.Tests.Store.indexAndCliSuite {Stigi.Test}: Stigi.Test.Result =
  val zeroByte: U8 = 0
  val zeroWord: U64 = 0
  val hexSample: U64 = Stigi.Store.decodeHexWord(16, "0123456789abcdef", 0)
  val hexValid: Bool = hexSample == 0x0123456789abcdef
  val enumKindSample: Stigi.Store.UnitKind = Stigi.Store.parseUnitKind(4, "Enum", 0, 4)
  val kindChecks: Bool = Stigi.Store.unitKindCode(enumKindSample) == 1 && Stigi.Store.unitKindCode(Stigi.Store.parseUnitKind(4, "Test", 0, 4)) == 11 && Stigi.Store.unitKindCode(Stigi.Store.parseUnitKind(7, "NegTest", 0, 7)) == 12 && Stigi.Store.unitKindCode(Stigi.Store.parseUnitKind(5, "Lemma", 0, 5)) == 4 && Stigi.Store.unitKindCode(Stigi.Store.parseUnitKind(6, "Extern", 0, 6)) == 8
  Array.tryFill{Stigi.Region}(128, zeroByte) match
    case FillFailed => Stigi.Test.expect(false, true, "index buf alloc failed")
    case Filled(idxBuf) =>
      var pos: U64 = Stigi.Emit.writeLiteral(10, "Demo.Alpha", 128, idxBuf, 0)
      pos = Stigi.Emit.writeByte(128, idxBuf, pos, 9)
      val digAbc: Stigi.Hash.Digest = Stigi.Hash.hashBytes{Stigi.Region}(3, "abc", 3)
      pos = Stigi.Emit.writeHexDigest{Stigi.Region}(digAbc, 128, idxBuf, pos)
      pos = Stigi.Emit.writeByte(128, idxBuf, pos, 9)
      pos = Stigi.Emit.writeLiteral(3, "Def", 128, idxBuf, pos)
      pos = Stigi.Emit.writeByte(128, idxBuf, pos, 10)
      Array.tryFill{Stigi.Region}(4, zeroWord) match
        case FillFailed => Stigi.Test.expect(false, true, "starts alloc failed")
        case Filled(nameStarts) =>
          Array.tryFill{Stigi.Region}(4, zeroWord) match
            case FillFailed => Stigi.Test.expect(false, true, "ends alloc failed")
            case Filled(nameEnds) =>
              Array.tryFill{Stigi.Region}(4, zeroByte) match
                case FillFailed => Stigi.Test.expect(false, true, "kinds alloc failed")
                case Filled(kindCodes) =>
                  Array.tryFill{Stigi.Region}(4, zeroWord) match
                    case FillFailed => Stigi.Test.expect(false, true, "d0 alloc failed")
                    case Filled(digestLow) =>
                      Array.tryFill{Stigi.Region}(4, zeroWord) match
                        case FillFailed => Stigi.Test.expect(false, true, "d1 alloc failed")
                        case Filled(digestMid) =>
                          Array.tryFill{Stigi.Region}(4, zeroWord) match
                            case FillFailed => Stigi.Test.expect(false, true, "d2 alloc failed")
                            case Filled(digestHigh) =>
                              Array.tryFill{Stigi.Region}(4, zeroWord) match
                                case FillFailed => Stigi.Test.expect(false, true, "d3 alloc failed")
                                case Filled(digestTop) =>
                                  val parsedValid: Bool = Stigi.Store.parseIndex(128, idxBuf, pos, 4, nameStarts, nameEnds, kindCodes, digestLow, digestMid, digestHigh, digestTop) match
                                    case IndexCorrupt(_) => false
                                    case IndexParsed(unitCount) =>
                                      Stigi.Store.unitCountBounded(unitCount, 4, 4)
                                      unitCount == 1 && nameStarts(0) == 0 && nameEnds(0) == 10 && kindCodes(0) == 3
                                  val badRejected: Bool = Stigi.Store.parseIndex(4, "bad\n", 4, 4, nameStarts, nameEnds, kindCodes, digestLow, digestMid, digestHigh, digestTop) match
                                    case IndexParsed(_) => false
                                    case IndexCorrupt(lineNum) => lineNum == 1
                                  Array.tryFill{Stigi.Region}(128, zeroByte) match
                                    case FillFailed => Stigi.Test.expect(false, true, "path alloc failed")
                                    case Filled(pathBuf) =>
                                      val subLen: U64 = Stigi.Store.buildSubPath(4, "tree", 0, 4, 5, "index", 128, pathBuf)
                                      val subValid: Bool = Stigi.Parse.sliceEquals(128, pathBuf, 0, subLen, 10, "tree/index")
                                      val unitLen: U64 = Stigi.Store.buildUnitFilePath(4, "tree", 0, 4, 10, "Demo.Alpha", 0, 10, 128, pathBuf)
                                      val unitPathValid: Bool = Stigi.Parse.sliceEquals(128, pathBuf, 0, unitLen, 18, "tree/Demo/Alpha.sc")
                                      var cmdPos: U64 = Stigi.Emit.writeLiteral(5, "stigi", 128, pathBuf, 0)
                                      cmdPos = Stigi.Emit.writeByte(128, pathBuf, cmdPos, 0)
                                      cmdPos = Stigi.Emit.writeLiteral(6, "--load", 128, pathBuf, cmdPos)
                                      cmdPos = Stigi.Emit.writeByte(128, pathBuf, cmdPos, 0)
                                      cmdPos = Stigi.Emit.writeLiteral(4, "tree", 128, pathBuf, cmdPos)
                                      cmdPos = Stigi.Emit.writeByte(128, pathBuf, cmdPos, 0)
                                      cmdPos = Stigi.Emit.writeLiteral(6, "--emit", 128, pathBuf, cmdPos)
                                      cmdPos = Stigi.Emit.writeByte(128, pathBuf, cmdPos, 0)
                                      cmdPos = Stigi.Emit.writeLiteral(5, "out.c", 128, pathBuf, cmdPos)
                                      cmdPos = Stigi.Emit.writeByte(128, pathBuf, cmdPos, 0)
                                      cmdPos = Stigi.Emit.writeLiteral(7, "--tests", 128, pathBuf, cmdPos)
                                      val cliValid: Bool = Stigi.Store.parseCmdline(128, pathBuf, cmdPos) match
                                        case CliArgs(loadStart, loadEnd, emitStart, emitEnd, checkOnly, showTests, showLaws) =>
                                          Stigi.Parse.sliceEquals(128, pathBuf, loadStart, loadEnd, 4, "tree") && Stigi.Parse.sliceEquals(128, pathBuf, emitStart, emitEnd, 5, "out.c") && !checkOnly && showTests && !showLaws
                                      val allPassed: Bool = hexValid && kindChecks && parsedValid && badRejected && subValid && unitPathValid && cliValid
                                      Stigi.Test.expect(allPassed, true, "store index parser, path builder, and cmdline parser must pass")
