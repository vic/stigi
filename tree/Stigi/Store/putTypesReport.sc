// prompt: write the one report line of a types export into the report buffer: the unit and row counts, or the typed reason the export failed with the failing unit when known, giving the new end.
def Stigi.Store.putTypesReport {region: Stigi.Region, Stigi.Mut} (report: Array[U8](65536), outcome: Stigi.Names.ExportOutcome): U64 <lessOrEqual(result, 65536)> =
  outcome match
    case Exported(units, _, _, _) =>
      val prefixed: U64 = Stigi.Store.putExportPrefix(report, 0, TypesExport)
      val unitsEnd: U64 = Stigi.Store.putCount{region}(report, prefixed, 6, "units=", units)
      val rowsEnd: U64 = Stigi.Store.putCount{region}(report, unitsEnd, 6, " rows=", 10)
      Stigi.Array.copy[U8](1, "\n", 0, 1, 65536, report, rowsEnd)
    case ExportFailed(why) =>
      val prefixed: U64 = Stigi.Store.putExportPrefix(report, 0, TypesExport)
      Stigi.Store.putReasonWord(report, Stigi.Array.copy[U8](8, "failed: ", 0, 8, 65536, report, prefixed), why)
    case ExportFailedAt(entry, step, why, nameBuf, nameSize) =>
      Stigi.Store.putReasonWord(report, Stigi.Store.putExportFailedAt{region}(report, entry, nameBuf, nameSize, TypesExport, step), why)
