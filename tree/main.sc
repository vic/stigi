// prompt: self-hosted Stigi compiler entry point reading Linux process cmdline and driving load, check, emit, and S39 contracts.
pub def main {region: Stigi.Region, Stigi.Mut} () <lessOrEqual(0, 1)>: I32 =
  val zeroByte: U8 = 0
  val zeroWord: U64 = 0
  val cmdPath: Array[U8](18) = "/proc/self/cmdline"
  Array.tryFill{region}(512, zeroByte) match
    case FillFailed => 1
    case Filled(pathBuf) =>
      Array.tryFill{region}(4096, zeroByte) match
        case FillFailed => 1
        case Filled(cmdBuf) =>
          Array.tryFill{region}(65536, zeroByte) match
            case FillFailed => 1
            case Filled(chunkBuf) =>
              Array.tryFill{region}(2, zeroWord) match
                case FillFailed => 1
                case Filled(readStateBox) =>
                  Stigi.Posix.openReadWithBuf(18, cmdPath, 0, 18, pathBuf) match
                    case OpenFailed => 2
                    case FileOpened(cmdDesc) =>
                      val cmdRead: Stigi.Posix.ReadOutcome = Stigi.Posix.readAllWithChunk(cmdDesc, chunkBuf, readStateBox, 4096, cmdBuf, 0)
                      val closedCmd: Bool = Stigi.Posix.closeDescriptor(cmdDesc)
                      if !closedCmd then
                        2
                      else
                        cmdRead match
                          case ReadFailed => 2
                          case ReadBytes(cmdLen) =>
                            if cmdLen <= 4096 then
                              Stigi.Store.parseCmdline(4096, cmdBuf, cmdLen) match
                                case CliArgs(loadStart, loadEnd, emitStart, emitEnd, _, _, _) =>
                                  if loadStart < loadEnd && loadEnd <= 4096 && emitStart <= emitEnd && emitEnd <= 4096 then
                                    Stigi.Store.loadAndCompileImage{region}(4096, cmdBuf, loadStart, loadEnd, emitStart, emitEnd)
                                  else if Stigi.Posix.verifyLiveContracts{region}() then
                                    0
                                  else
                                    1
                            else
                              2
