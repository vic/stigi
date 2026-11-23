// prompt: read /proc/self/cmdline via Stigi.File and dispatch compiler CLI actions.
pub def Stigi.Store.runCli {region: Stigi.Region, clock: Stigi.Clock, Stigi.Mut} (): Stigi.RunOutcome =
  val Filled(procPath) = Array.tryFill[U8]{region}(4096, 0) else
    case FillFailed => RunOutOfMemory
  val Filled(cmdBuf) = Array.tryFill[U8]{region}(16384, 0) else
    case FillFailed => RunOutOfMemory
  val procLit: Array[U8](18) = "/proc/self/cmdline"
  var idx: U64 = 0
  while idx < 18 do
    @<lessOrEqual(idx, 18)>
    procPath(idx) := procLit(idx)
    idx = idx + 1
  val LoadedBytes(cmdLen) = Stigi.Posix.readFileBytes{region}(18, procPath, 16384, cmdBuf, 0) else
    case LoadFailed => RunFileError
    case LoadOverflow => RunCliError
  if cmdLen > 16384 then RunCliError else
    Stigi.Store.parseCmdline(16384, cmdBuf, cmdLen) match
      case CliRunContracts => Stigi.Posix.verifyContracts{region, clock}()
      case CliCheckTree(dirBeg, dirFin) =>
        if dirFin <= 16384 then Stigi.Store.verifyTree{region}(16384, cmdBuf, dirBeg, dirFin) else RunCliError
      case CliLoadEmit(dirBeg, dirFin, outBeg, outFin) =>
        if dirFin <= 16384 && outFin <= 16384 then
          val Filled(outPath) = Array.tryFill[U8]{region}(4096, 0) else
            case FillFailed => RunOutOfMemory
          var outLen: U64 = 0
          var scanPos: U64 = outBeg
          while scanPos < outFin && outLen < 4095 do
            @<lessOrEqual(outBeg, scanPos), lessOrEqual(scanPos, outFin), lessOrEqual(outLen, 4095)>
            outPath(outLen) := cmdBuf(scanPos)
            outLen = outLen + 1
            scanPos = scanPos + 1
          outPath(outLen) := 0
          Stigi.Emit.compileTree{region}(16384, cmdBuf, dirBeg, dirFin, outLen, outPath)
        else RunCliError
      case CliInvalid => RunCliError
