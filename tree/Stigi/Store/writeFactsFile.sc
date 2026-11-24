// prompt: write one facts file of the existing output directory from the first bytes of a source array, creating or truncating it, and give the failure with its file when it cannot be written.
def Stigi.Store.writeFactsFile {region: Stigi.Region, Stigi.Mut} (outCap: Stigi.ByteCap, outBuf: Array[U8](outCap), outStart: U64, outEnd: U64, file: Stigi.Store.FactsFile, srcCap: Stigi.EmitCap, srcBuf: Array[U8](srcCap), srcLen: U64) <lessOrEqual(outStart, outEnd), lessOrEqual(outEnd, outCap), lessOrEqual(srcLen, srcCap)>: Stigi.Store.FactsOutcome =
  val Filled(path) = Array.tryFill[U8]{region}(Stigi.Limit.maxPathBytes, 0) else
    case FillFailed => FactsFailed(FactsOutOfMemory)
  Stigi.Store.buildFactsPath(outCap, outBuf, outStart, outEnd, file, path) match
    case PathOverflow => FactsFailed(FileFailed(file, PathTooLong))
    case PathBuilt(pathLen) =>
      Stigi.Posix.writeFileBytes{region}(pathLen, path, srcCap, srcBuf, srcLen) match
        case RunSuccess => FactsWritten
        case RunOutOfMemory => FactsFailed(FactsOutOfMemory)
        case RunCliError | RunFileError | RunDoorRejected | RunEmitError | RunContractError => FactsFailed(FileFailed(file, WriteFailed))
