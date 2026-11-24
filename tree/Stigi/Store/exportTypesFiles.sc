// prompt: export the types tables of a valid tree in a directory into the fact files of an existing output directory, sharing one syntax table allocation across exportTree and length collection.
def Stigi.Store.exportTypesFiles {region: Stigi.Region, Stigi.Mut} (dirCap: Stigi.ByteCap, dirBuf: Array[U8](dirCap), dirStart: U64, dirEnd: U64, outStart: U64, outEnd: U64) <lessOrEqual(dirStart, dirEnd), lessOrEqual(dirEnd, dirCap), lessOrEqual(outStart, outEnd), lessOrEqual(outEnd, dirCap)>: Stigi.RunOutcome =
  val Filled(report) = Array.tryFill[U8]{region}(65536, 0) else
    case FillFailed => RunOutOfMemory
  val Allocated(synTables) = Stigi.Syntax.allocate{region}() else
    case AllocationFailed => RunOutOfMemory
  val outcome: Stigi.Names.ExportOutcome = Stigi.Names.exportTreeWithTables{region}(dirCap, dirBuf, dirStart, dirEnd, synTables)
  outcome match
    case Exported(_, _, image, _) =>
      Stigi.Store.writeTypesFacts{region}(TypesTarget(dirCap, dirBuf, dirStart, dirEnd, outStart, outEnd), image, synTables) match
        case TypesWritten(rowCount) =>
          Stigi.Posix.writeToDesc{region}(1, 65536, report, Stigi.Store.putTypesReport{region}(report, outcome, rowCount)) match
            case RunSuccess => RunSuccess
            case RunCliError | RunFileError | RunOutOfMemory | RunDoorRejected | RunEmitError | RunContractError => RunFileError
        case TypesWriteFailed(failure) =>
          Stigi.Posix.writeToDesc{region}(2, 65536, report, Stigi.Store.putExportFailure{region}(report, 0, TypesExport, failure)) match
            case RunSuccess =>
              failure match
                case FactsOutOfMemory => RunOutOfMemory
                case FactsStopped(_) | FileFailed(_, _) => RunFileError
            case RunCliError | RunFileError | RunOutOfMemory | RunDoorRejected | RunEmitError | RunContractError => RunFileError
    case ExportFailed(_) | ExportFailedAt(_, _, _, _, _) =>
      Stigi.Posix.writeToDesc{region}(1, 65536, report, Stigi.Store.putTypesReport{region}(report, outcome, 0)) match
        case RunSuccess => RunDoorRejected
        case RunCliError | RunFileError | RunOutOfMemory | RunDoorRejected | RunEmitError | RunContractError => RunFileError
