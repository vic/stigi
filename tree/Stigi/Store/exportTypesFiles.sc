// prompt: export the types tables of a valid tree in a directory into the fact files of an existing output directory, writing the summary line to stdout on success or failure to stdout or stderr.
def Stigi.Store.exportTypesFiles {region: Stigi.Region, Stigi.Mut} (dirCap: Stigi.ByteCap, dirBuf: Array[U8](dirCap), dirStart: U64, dirEnd: U64, outStart: U64, outEnd: U64) <lessOrEqual(dirStart, dirEnd), lessOrEqual(dirEnd, dirCap), lessOrEqual(outStart, outEnd), lessOrEqual(outEnd, dirCap)>: Stigi.RunOutcome =
  val Filled(report) = Array.tryFill[U8]{region}(65536, 0) else
    case FillFailed => RunOutOfMemory
  val outcome: Stigi.Names.ExportOutcome = Stigi.Names.exportTree{region}(dirCap, dirBuf, dirStart, dirEnd)
  outcome match
    case Exported(_, _, _, _) =>
      Stigi.Store.writeTypesFacts{region}(dirCap, dirBuf, outStart, outEnd) match
        case FactsWritten =>
          Stigi.Posix.writeToDesc{region}(1, 65536, report, Stigi.Store.putTypesReport{region}(report, outcome)) match
            case RunSuccess => RunSuccess
            case RunCliError | RunFileError | RunOutOfMemory | RunDoorRejected | RunEmitError | RunContractError => RunFileError
        case FactsFailed(failure) =>
          Stigi.Posix.writeToDesc{region}(2, 65536, report, Stigi.Store.putExportFailure{region}(report, 0, TypesExport, failure)) match
            case RunSuccess =>
              failure match
                case FactsOutOfMemory => RunOutOfMemory
                case FactsStopped(_) | FileFailed(_, _) => RunFileError
            case RunCliError | RunFileError | RunOutOfMemory | RunDoorRejected | RunEmitError | RunContractError => RunFileError
    case ExportFailed(_) | ExportFailedAt(_, _, _, _, _) =>
      Stigi.Posix.writeToDesc{region}(1, 65536, report, Stigi.Store.putTypesReport{region}(report, outcome)) match
        case RunSuccess => RunDoorRejected
        case RunCliError | RunFileError | RunOutOfMemory | RunDoorRejected | RunEmitError | RunContractError => RunFileError
