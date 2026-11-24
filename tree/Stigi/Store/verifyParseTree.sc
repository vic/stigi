// prompt: parse and print back every unit listed in <dir>/index, write the report to standard output, and succeed only when every unit printed back byte for byte.
def Stigi.Store.verifyParseTree {region: Stigi.Region, Stigi.Mut} (dirCap: Stigi.ByteCap, dirBuf: Array[U8](dirCap), dirStart: U64, dirEnd: U64) <lessOrEqual(dirStart, dirEnd), lessOrEqual(dirEnd, dirCap)>: Stigi.RunOutcome =
  val Filled(pathBuf) = Array.tryFill[U8]{region}(4096, 0) else
    case FillFailed => RunOutOfMemory
  val Filled(indexBuf) = Array.tryFill[U8]{region}(262144, 0) else
    case FillFailed => RunOutOfMemory
  val Filled(unitBuf) = Array.tryFill[U8]{region}(65536, 0) else
    case FillFailed => RunOutOfMemory
  val Filled(report) = Array.tryFill[U8]{region}(65536, 0) else
    case FillFailed => RunOutOfMemory
  val Allocated(tables) = Stigi.Syntax.allocate{region}() else
    case AllocationFailed => RunOutOfMemory
  val Found(idxPathLen) = Stigi.Store.buildIndexPath(dirCap, dirBuf, dirStart, dirEnd, pathBuf) else
    case Missing => RunFileError
  if idxPathLen >= 4096 then RunFileError else
    val LoadedBytes(indexLen) = Stigi.Posix.readFileBytes{region}(idxPathLen, pathBuf, 262144, indexBuf, 0) else
      case LoadFailed => RunFileError
      case LoadOverflow => RunFileError
    if indexLen > 262144 then RunFileError else
      val ParseTally(units, matched, bytes, tokens, nodes, reportAt) = Stigi.Store.verifyParseLines{region}(dirCap, dirBuf, dirStart, dirEnd, ParseScratch(pathBuf, indexBuf, unitBuf, report, tables), indexLen, IndexWalk(0, 0), ParseTally(0, 0, 0, 0, 0, 0), indexLen + 1)
      val unitsEnd: U64 = Stigi.Store.putCount{region}(report, reportAt, 6, "units ", units)
      val matchedEnd: U64 = Stigi.Store.putCount{region}(report, unitsEnd, 9, " matched ", matched)
      val bytesEnd: U64 = Stigi.Store.putCount{region}(report, matchedEnd, 7, " bytes ", bytes)
      val tokensEnd: U64 = Stigi.Store.putCount{region}(report, bytesEnd, 8, " tokens ", tokens)
      val nodesEnd: U64 = Stigi.Store.putCount{region}(report, tokensEnd, 7, " nodes ", nodes)
      val reportEnd: U64 = Stigi.Array.copy[U8](1, "\n", 0, 1, 65536, report, nodesEnd)
      Stigi.Posix.writeToDesc{region}(1, 65536, report, reportEnd) match
        case RunSuccess => if 0 < units && matched == units then RunSuccess else RunDoorRejected
        case RunCliError | RunFileError | RunOutOfMemory | RunDoorRejected | RunEmitError | RunContractError => RunFileError
