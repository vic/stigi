// prompt: load all unit source files for a parsed index into srcArena and optionally emit deterministic C11.
def Stigi.Store.compileParsedImage {region: Stigi.Region, Stigi.Mut} (cmdCap: U64, cmdBuf: Array[U8](cmdCap), loadStart: U64, loadEnd: U64, emitStart: U64, emitEnd: U64, indexBuf: Array[U8](65536), pathBuf: Array[U8](512), chunkBuf: Array[U8](65536), readStateBox: Array[U64](2), unitCount: U64, rawNameStarts: Array[U64](1024), rawNameEnds: Array[U64](1024), rawKindCodes: Array[U8](1024), rawDigestLow: Array[U64](1024), rawDigestMid: Array[U64](1024), rawDigestHigh: Array[U64](1024), rawDigestTop: Array[U64](1024)) <lessOrEqual(loadStart, loadEnd), lessOrEqual(loadEnd, cmdCap), lessOrEqual(emitStart, emitEnd), lessOrEqual(emitEnd, cmdCap), below(cmdCap, 0x100000000), lessOrEqual(unitCount, 1024)>: I32 =
  val zeroByte: U8 = 0
  val zeroWord: U64 = 0
  Array.tryFill{region}(unitCount, zeroWord) match
    case FillFailed => 11
    case Filled(nameStarts) =>
      Array.tryFill{region}(unitCount, zeroWord) match
        case FillFailed => 11
        case Filled(nameEnds) =>
          Array.tryFill{region}(unitCount, zeroByte) match
            case FillFailed => 11
            case Filled(kindCodes) =>
              Array.tryFill{region}(unitCount, zeroWord) match
                case FillFailed => 11
                case Filled(digestLow) =>
                  Array.tryFill{region}(unitCount, zeroWord) match
                    case FillFailed => 11
                    case Filled(digestMid) =>
                      Array.tryFill{region}(unitCount, zeroWord) match
                        case FillFailed => 11
                        case Filled(digestHigh) =>
                          Array.tryFill{region}(unitCount, zeroWord) match
                            case FillFailed => 11
                            case Filled(digestTop) =>
                              Array.tryFill{region}(unitCount, zeroWord) match
                                case FillFailed => 11
                                case Filled(srcStarts) =>
                                  Array.tryFill{region}(unitCount, zeroWord) match
                                    case FillFailed => 11
                                    case Filled(srcEnds) =>
                                      Array.tryFill{region}(4194304, zeroByte) match
                                        case FillFailed => 12
                                        case Filled(srcArena) =>
                                          Array.tryFill{region}(2, zeroWord) match
                                            case FillFailed => 12
                                            case Filled(loadBox) =>
                                              var copyIdx: U64 = 0
                                              while copyIdx < unitCount do
                                                @<lessOrEqual(copyIdx, unitCount), decreases(unitCount - copyIdx)>
                                                nameStarts(copyIdx) := rawNameStarts(copyIdx)
                                                nameEnds(copyIdx) := rawNameEnds(copyIdx)
                                                kindCodes(copyIdx) := rawKindCodes(copyIdx)
                                                digestLow(copyIdx) := rawDigestLow(copyIdx)
                                                digestMid(copyIdx) := rawDigestMid(copyIdx)
                                                digestHigh(copyIdx) := rawDigestHigh(copyIdx)
                                                digestTop(copyIdx) := rawDigestTop(copyIdx)
                                                copyIdx = copyIdx + 1
                                              var unitIdx: U64 = 0
                                              while unitIdx < unitCount && loadBox(1) == 0 do
                                                @<lessOrEqual(unitIdx, unitCount), decreases(unitCount - unitIdx)>
                                                val nameBeg: U64 = nameStarts(unitIdx)
                                                val nameFin: U64 = nameEnds(unitIdx)
                                                val curOffset: U64 = loadBox(0)
                                                if nameBeg <= nameFin && nameFin <= 65536 && curOffset <= 4194304 then
                                                  val pathLen: U64 = Stigi.Store.buildUnitFilePath(cmdCap, cmdBuf, loadStart, loadEnd, 65536, indexBuf, nameBeg, nameFin, 512, pathBuf)
                                                  if pathLen < 512 then
                                                    pathBuf(pathLen) := 0
                                                    Stigi.Posix.openReadWithBuf(512, pathBuf, 0, pathLen, pathBuf) match
                                                      case OpenFailed =>
                                                        loadBox(1) := 13
                                                      case FileOpened(unitDesc) =>
                                                        Stigi.Posix.readAllWithChunk(unitDesc, chunkBuf, readStateBox, 4194304, srcArena, curOffset) match
                                                          case ReadFailed =>
                                                            loadBox(1) := 14
                                                          case ReadBytes(gotLen) =>
                                                            if gotLen <= 4194304 then
                                                              val endOffset: U64 = curOffset + gotLen
                                                              if endOffset <= 4194304 then
                                                                srcStarts(unitIdx) := curOffset
                                                                srcEnds(unitIdx) := endOffset
                                                                loadBox(0) := endOffset
                                                              else
                                                                loadBox(1) := 15
                                                            else
                                                              loadBox(1) := 15
                                                        val closedUnit: Bool = Stigi.Posix.closeDescriptor(unitDesc)
                                                        if !closedUnit then loadBox(1) := 16 else ()
                                                  else
                                                    loadBox(1) := 17
                                                else
                                                  loadBox(1) := 18
                                                unitIdx = unitIdx + 1
                                              if loadBox(1) != 0 then
                                                20
                                              else if emitStart < emitEnd then
                                                Array.tryFill{region}(8388608, zeroByte) match
                                                  case FillFailed => 21
                                                  case Filled(outBuf) =>
                                                    val outLen: U64 = Stigi.Emit.compileImage{region}(unitCount, 65536, indexBuf, nameStarts, nameEnds, kindCodes, digestLow, digestMid, digestHigh, digestTop, 4194304, srcArena, srcStarts, srcEnds, 8388608, outBuf)
                                                    val crashFree: Bool = !Stigi.Emit.hasForbiddenCrashCall(8388608, outBuf, outLen)
                                                    if outLen > 0 && crashFree then
                                                      Stigi.Posix.openWriteTruncWithBuf(cmdCap, cmdBuf, emitStart, emitEnd, pathBuf) match
                                                        case OpenFailed => 22
                                                        case FileOpened(outDesc) =>
                                                          val writeGood: Bool = Stigi.Posix.writeAllFromSlice{region}(outDesc, 8388608, outBuf, 0, outLen) match
                                                            case WriteFailed => false
                                                            case WroteBytes(wroteLen) => wroteLen == outLen
                                                          val closedOut: Bool = Stigi.Posix.closeDescriptor(outDesc)
                                                          if writeGood && closedOut then
                                                            val doneLine: Array[U8](11) = "stigi done\n"
                                                            Stigi.Posix.writeAllFromSlice{region}(1, 11, doneLine, 0, 11) match
                                                              case WriteFailed => 24
                                                              case WroteBytes(_) => 0
                                                          else
                                                            23
                                                    else
                                                      25
                                              else
                                                val doneLine: Array[U8](11) = "stigi done\n"
                                                Stigi.Posix.writeAllFromSlice{region}(1, 11, doneLine, 0, 11) match
                                                  case WriteFailed => 26
                                                  case WroteBytes(_) => 0
