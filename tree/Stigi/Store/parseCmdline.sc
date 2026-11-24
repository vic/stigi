// prompt: parse NUL-separated /proc/self/cmdline bytes into a closed Stigi.Store.CliAction.
def Stigi.Store.parseCmdline(bufCap: Stigi.ByteCap, buf: Array[U8](bufCap), bufLen: U64) <lessOrEqual(bufLen, bufCap)>: Stigi.Store.CliAction =
  val ArgAt(_, _, afterProgram) = Stigi.Store.nextArg(bufCap, buf, bufLen, 0) else
    case ArgsEnd => CliRunContracts
  val ArgAt(flagStart, flagEnd, afterFlag) = Stigi.Store.nextArg(bufCap, buf, bufLen, afterProgram) else
    case ArgsEnd => CliRunContracts
  val ArgAt(dirStart, dirEnd, afterDir) = Stigi.Store.nextArg(bufCap, buf, bufLen, afterFlag) else
    case ArgsEnd => CliInvalid
  if dirStart == dirEnd || bufCap < flagEnd then CliInvalid else
    if Stigi.Lex.sliceEquals(bufCap, buf, flagStart, flagEnd, 7, "--check") then
      if afterDir < bufLen then CliInvalid else CliCheckTree(dirStart, dirEnd)
    else if Stigi.Lex.sliceEquals(bufCap, buf, flagStart, flagEnd, 14, "--verify-parse") then
      if afterDir < bufLen then CliInvalid else CliVerifyParse(dirStart, dirEnd)
    else if Stigi.Lex.sliceEquals(bufCap, buf, flagStart, flagEnd, 14, "--export-names") then
      if afterDir < bufLen then
        val ArgAt(namesOutStart, namesOutEnd, afterNamesOut) = Stigi.Store.nextArg(bufCap, buf, bufLen, afterDir) else
          case ArgsEnd => CliInvalid
        if afterNamesOut < bufLen || namesOutStart == namesOutEnd then CliInvalid else CliExportNamesTo(dirStart, dirEnd, namesOutStart, namesOutEnd)
      else CliExportNames(dirStart, dirEnd)
    else if Stigi.Lex.sliceEquals(bufCap, buf, flagStart, flagEnd, 6, "--load") then
      val ArgAt(emitStart, emitEnd, afterEmit) = Stigi.Store.nextArg(bufCap, buf, bufLen, afterDir) else
        case ArgsEnd => CliInvalid
      val ArgAt(outStart, outEnd, afterOut) = Stigi.Store.nextArg(bufCap, buf, bufLen, afterEmit) else
        case ArgsEnd => CliInvalid
      if afterOut < bufLen || outStart == outEnd || bufCap < emitEnd then CliInvalid else
        if Stigi.Lex.sliceEquals(bufCap, buf, emitStart, emitEnd, 6, "--emit") then CliLoadEmit(dirStart, dirEnd, outStart, outEnd) else CliInvalid
    else CliInvalid
