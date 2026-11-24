// prompt: read /proc/self/cmdline via Stigi.File and dispatch compiler CLI actions.
pub def Stigi.Store.runCli {region: Stigi.Region, clock: Stigi.Clock, Stigi.Mut} (): Stigi.RunOutcome =
  val Filled(procPath) = Array.tryFill[U8]{region}(4096, 0) else
    case FillFailed => RunOutOfMemory
  val Filled(cmdBuf) = Array.tryFill[U8]{region}(16384, 0) else
    case FillFailed => RunOutOfMemory
  val procLit: Array[U8](18) = "/proc/self/cmdline"
  Stigi.Array.copy(18, procLit, 0, 18, 4096, procPath, 0)
  val LoadedBytes(cmdLen) = Stigi.Posix.readFileBytes{region}(18, procPath, 16384, cmdBuf, 0) else
    case LoadFailed => RunFileError
    case LoadOverflow => RunCliError
  if cmdLen > 16384 then RunCliError else
    Stigi.Store.parseCmdline(16384, cmdBuf, cmdLen) match
      case CliRunContracts => Stigi.Posix.verifyContracts{region, clock}()
      case CliCheckTree(dirBeg, dirFin) =>
        if dirFin <= 16384 then Stigi.Store.verifyTree{region}(16384, cmdBuf, dirBeg, dirFin) else RunCliError
      case CliVerifyParse(dirBeg, dirFin) =>
        if dirFin <= 16384 then Stigi.Store.verifyParseTree{region}(16384, cmdBuf, dirBeg, dirFin) else RunCliError
      case CliExportNames(dirBeg, dirFin) =>
        if dirFin <= 16384 then Stigi.Store.exportNamesTree{region}(16384, cmdBuf, dirBeg, dirFin) else RunCliError
      case CliExportNamesTo(dirBeg, dirFin, outBeg, outFin) =>
        if dirFin <= 16384 && outFin <= 16384 then Stigi.Store.exportNamesFiles{region}(16384, cmdBuf, dirBeg, dirFin, outBeg, outFin) else RunCliError
      case CliLoadEmit(dirBeg, dirFin, outBeg, outFin) =>
        if dirFin <= 16384 && outFin <= 16384 then
          val Filled(outPath) = Array.tryFill[U8]{region}(4096, 0) else
            case FillFailed => RunOutOfMemory
          val outLen: U64 = Stigi.Array.copy(16384, cmdBuf, outBeg, outFin, 4096, outPath, 0)
          if outLen < 4096 then
            @<below(outLen, 4096)>
            outPath(outLen) := '\0'
            Stigi.Emit.compileTree{region}(16384, cmdBuf, dirBeg, dirFin, outLen, outPath)
          else RunCliError
        else RunCliError
      case CliInvalid => RunCliError
