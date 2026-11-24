// prompt: export the names of the tree in a directory, write one summary line to standard output, and succeed only when every unit was indexed and walked.
def Stigi.Store.exportNamesTree {region: Stigi.Region, Stigi.Mut} (dirCap: Stigi.ByteCap, dirBuf: Array[U8](dirCap), dirStart: U64, dirEnd: U64) <lessOrEqual(dirStart, dirEnd), lessOrEqual(dirEnd, dirCap)>: Stigi.RunOutcome =
  val Filled(report) = Array.tryFill[U8]{region}(65536, 0) else
    case FillFailed => RunOutOfMemory
  val outcome: Stigi.Names.ExportOutcome = Stigi.Names.exportTree{region}(dirCap, dirBuf, dirStart, dirEnd)
  Stigi.Posix.writeToDesc{region}(1, 65536, report, Stigi.Store.putNamesReport{region}(report, outcome)) match
    case RunSuccess =>
      outcome match
        case Exported(_, _, _, _) => RunSuccess
        case ExportFailed(_) | ExportFailedAt(_, _, _, _, _) => RunDoorRejected
    case RunCliError | RunFileError | RunOutOfMemory | RunDoorRejected | RunEmitError | RunContractError => RunFileError
