// prompt: export the names of the tree in a directory into the fact files of an output directory that already exists, write the summary line to standard output only when every file was written, and write the failure line to standard error otherwise.
def Stigi.Store.exportNamesFiles {region: Stigi.Region, Stigi.Mut} (dirCap: Stigi.ByteCap, dirBuf: Array[U8](dirCap), dirStart: U64, dirEnd: U64, outStart: U64, outEnd: U64) <lessOrEqual(dirStart, dirEnd), lessOrEqual(dirEnd, dirCap), lessOrEqual(outStart, outEnd), lessOrEqual(outEnd, dirCap)>: Stigi.RunOutcome =
  val Filled(report) = Array.tryFill[U8]{region}(65536, 0) else
    case FillFailed => RunOutOfMemory
  val outcome: Stigi.Names.ExportOutcome = Stigi.Names.exportTree{region}(dirCap, dirBuf, dirStart, dirEnd)
  outcome match
    case Exported(_, _, image, edgeTable) =>
      Stigi.Store.writeNamesFacts{region}(dirCap, dirBuf, outStart, outEnd, Stigi.Store.laneBytes, image, edgeTable) match
        case FactsWritten =>
          Stigi.Posix.writeToDesc{region}(1, 65536, report, Stigi.Store.putNamesReport{region}(report, outcome)) match
            case RunSuccess => RunSuccess
            case RunCliError | RunFileError | RunOutOfMemory | RunDoorRejected | RunEmitError | RunContractError => RunFileError
        case FactsFailed(failure) =>
          Stigi.Posix.writeToDesc{region}(2, 65536, report, Stigi.Store.putFactsFailure{region}(report, 0, failure)) match
            case RunSuccess =>
              failure match
                case FactsOutOfMemory => RunOutOfMemory
                case FactsStopped(_) | FileFailed(_, _) => RunFileError
            case RunCliError | RunFileError | RunOutOfMemory | RunDoorRejected | RunEmitError | RunContractError => RunFileError
    case ExportFailed(_) | ExportFailedAt(_, _, _, _, _) =>
      Stigi.Posix.writeToDesc{region}(1, 65536, report, Stigi.Store.putNamesReport{region}(report, outcome)) match
        case RunSuccess => RunDoorRejected
        case RunCliError | RunFileError | RunOutOfMemory | RunDoorRejected | RunEmitError | RunContractError => RunFileError
