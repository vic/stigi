// prompt: compile an entire stored Stigi L0 image into deterministic portable ISO C11.
pub def Stigi.Emit.compileImage {region: Stigi.Region, Stigi.Mut} (unitCount: U64, indexCap: U64, indexBytes: Array[U8](indexCap), nameStarts: Array[U64](unitCount), nameEnds: Array[U64](unitCount), kindCodes: Array[U8](unitCount), digestLow: Array[U64](unitCount), digestMid: Array[U64](unitCount), digestHigh: Array[U64](unitCount), digestTop: Array[U64](unitCount), srcCap: U64, srcBytes: Array[U8](srcCap), unitSrcStart: Array[U64](unitCount), unitSrcEnd: Array[U64](unitCount), outCap: U64, output: Array[U8](outCap)) <below(unitCount, 4096), below(indexCap, 0x100000000), below(srcCap, 0x100000000), below(outCap, 0x100000000)>: U64 <lessOrEqual(result, outCap)> =
  val zeroByte: U8 = 0
  val zeroWord: U64 = 0
  Array.tryFill{region}(unitCount, zeroWord) match
    case FillFailed => 0
    case Filled(order) =>
      Stigi.Emit.sortUnitsByDigest(unitCount, digestLow, digestMid, digestHigh, digestTop, order)
      Array.tryFill{region}(unitCount, zeroWord) match
        case FillFailed => 0
        case Filled(unitRetEnum) =>
          Array.tryFill{region}(8192, zeroByte) match
            case FillFailed => 0
            case Filled(tokKind) =>
              Array.tryFill{region}(8192, zeroWord) match
                case FillFailed => 0
                case Filled(tokStart) =>
                  Array.tryFill{region}(8192, zeroWord) match
                    case FillFailed => 0
                    case Filled(tokEnd) =>
                      Array.tryFill{region}(8192, zeroWord) match
                        case FillFailed => 0
                        case Filled(tokLine) =>
                          Array.tryFill{region}(8192, zeroWord) match
                            case FillFailed => 0
                            case Filled(tokCol) =>
                              Array.tryFill{region}(1024, zeroWord) match
                                case FillFailed => 0
                                case Filled(caseEnumUnit) =>
                                  Array.tryFill{region}(1024, zeroWord) match
                                    case FillFailed => 0
                                    case Filled(caseNameStart) =>
                                      Array.tryFill{region}(1024, zeroWord) match
                                        case FillFailed => 0
                                        case Filled(caseNameEnd) =>
                                          Array.tryFill{region}(1024, zeroWord) match
                                            case FillFailed => 0
                                            case Filled(caseFieldBase) =>
                                              Array.tryFill{region}(1024, zeroWord) match
                                                case FillFailed => 0
                                                case Filled(caseFieldCount) =>
                                                  Array.tryFill{region}(2048, zeroWord) match
                                                    case FillFailed => 0
                                                    case Filled(fieldNameStart) =>
                                                      Array.tryFill{region}(2048, zeroWord) match
                                                        case FillFailed => 0
                                                        case Filled(fieldNameEnd) =>
                                                          Array.tryFill{region}(2048, zeroWord) match
                                                            case FillFailed => 0
                                                            case Filled(fieldTypeStart) =>
                                                              Array.tryFill{region}(2048, zeroWord) match
                                                                case FillFailed => 0
                                                                case Filled(fieldTypeEnd) =>
                                                                  Array.tryFill{region}(128, zeroWord) match
                                                                    case FillFailed => 0
                                                                    case Filled(localNameStart) =>
                                                                      Array.tryFill{region}(128, zeroWord) match
                                                                        case FillFailed => 0
                                                                        case Filled(localNameEnd) =>
                                                                          Array.tryFill{region}(128, zeroWord) match
                                                                            case FillFailed => 0
                                                                            case Filled(localTypeStart) =>
                                                                              Array.tryFill{region}(128, zeroWord) match
                                                                                case FillFailed => 0
                                                                                case Filled(localTypeEnd) =>
                                                                                  Array.tryFill{region}(128, zeroByte) match
                                                                                    case FillFailed => 0
                                                                                    case Filled(localIsArray) =>
                                                                                      Stigi.Emit.runCompilePasses{region}(unitCount, order, indexCap, indexBytes, nameStarts, nameEnds, kindCodes, unitRetEnum, srcCap, srcBytes, unitSrcStart, unitSrcEnd, tokKind, tokStart, tokEnd, tokLine, tokCol, caseEnumUnit, caseNameStart, caseNameEnd, caseFieldBase, caseFieldCount, fieldNameStart, fieldNameEnd, fieldTypeStart, fieldTypeEnd, localNameStart, localNameEnd, localTypeStart, localTypeEnd, localIsArray, outCap, output, 0)
