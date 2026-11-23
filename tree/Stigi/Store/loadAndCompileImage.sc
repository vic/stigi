// prompt: load a stored Stigi tree index from disk and run self-hosted compilation or verification.
pub def Stigi.Store.loadAndCompileImage {region: Stigi.Region, Stigi.Mut} (cmdCap: U64, cmdBuf: Array[U8](cmdCap), loadStart: U64, loadEnd: U64, emitStart: U64, emitEnd: U64) <lessOrEqual(loadStart, loadEnd), lessOrEqual(loadEnd, cmdCap), lessOrEqual(emitStart, emitEnd), lessOrEqual(emitEnd, cmdCap), below(cmdCap, 0x100000000)>: I32 =
  val zeroByte: U8 = 0
  val zeroWord: U64 = 0
  Array.tryFill{region}(512, zeroByte) match
    case FillFailed => 3
    case Filled(pathBuf) =>
      Array.tryFill{region}(65536, zeroByte) match
        case FillFailed => 3
        case Filled(indexBuf) =>
          Array.tryFill{region}(65536, zeroByte) match
            case FillFailed => 3
            case Filled(chunkBuf) =>
              Array.tryFill{region}(2, zeroWord) match
                case FillFailed => 3
                case Filled(readStateBox) =>
                  Array.tryFill{region}(1024, zeroWord) match
                    case FillFailed => 3
                    case Filled(rawNameStarts) =>
                      Array.tryFill{region}(1024, zeroWord) match
                        case FillFailed => 3
                        case Filled(rawNameEnds) =>
                          Array.tryFill{region}(1024, zeroByte) match
                            case FillFailed => 3
                            case Filled(rawKindCodes) =>
                              Array.tryFill{region}(1024, zeroWord) match
                                case FillFailed => 3
                                case Filled(rawDigestLow) =>
                                  Array.tryFill{region}(1024, zeroWord) match
                                    case FillFailed => 3
                                    case Filled(rawDigestMid) =>
                                      Array.tryFill{region}(1024, zeroWord) match
                                        case FillFailed => 3
                                        case Filled(rawDigestHigh) =>
                                          Array.tryFill{region}(1024, zeroWord) match
                                            case FillFailed => 3
                                            case Filled(rawDigestTop) =>
                                              val indexPathLen: U64 = Stigi.Store.buildSubPath(cmdCap, cmdBuf, loadStart, loadEnd, 5, "index", 512, pathBuf)
                                              if indexPathLen < 512 then
                                                Stigi.Posix.openReadWithBuf(512, pathBuf, 0, indexPathLen, pathBuf) match
                                                  case OpenFailed => 4
                                                  case FileOpened(indexDesc) =>
                                                    val indexReadRes: Stigi.Posix.ReadOutcome = Stigi.Posix.readAllWithChunk(indexDesc, chunkBuf, readStateBox, 65536, indexBuf, 0)
                                                    val closedIndex: Bool = Stigi.Posix.closeDescriptor(indexDesc)
                                                    if !closedIndex then
                                                      5
                                                    else
                                                      indexReadRes match
                                                        case ReadFailed => 6
                                                        case ReadBytes(indexLen) =>
                                                          if indexLen <= 65536 then
                                                            Stigi.Store.parseIndex(65536, indexBuf, indexLen, 1024, rawNameStarts, rawNameEnds, rawKindCodes, rawDigestLow, rawDigestMid, rawDigestHigh, rawDigestTop) match
                                                              case IndexCorrupt(_) => 7
                                                              case IndexParsed(unitCount) =>
                                                                if unitCount <= 1024 then
                                                                  Stigi.Store.compileParsedImage{region}(cmdCap, cmdBuf, loadStart, loadEnd, emitStart, emitEnd, indexBuf, pathBuf, chunkBuf, readStateBox, unitCount, rawNameStarts, rawNameEnds, rawKindCodes, rawDigestLow, rawDigestMid, rawDigestHigh, rawDigestTop)
                                                                else
                                                                  8
                                                          else
                                                            9
                                              else
                                                10
