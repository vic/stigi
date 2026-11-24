// prompt: write the one report line of a names export into the report buffer: the unit and edge counts, or the typed reason the export failed with the failing unit when known, giving the new end.
def Stigi.Store.putNamesReport {region: Stigi.Region, Stigi.Mut} (report: Array[U8](65536), outcome: Stigi.Names.ExportOutcome): U64 <lessOrEqual(result, 65536)> =
  outcome match
    case Exported(units, edges, _, _) =>
      val unitsEnd: U64 = Stigi.Store.putCount{region}(report, 0, 13, "names: units=", units)
      val edgesEnd: U64 = Stigi.Store.putCount{region}(report, unitsEnd, 7, " edges=", edges)
      Stigi.Array.copy[U8](1, "\n", 0, 1, 65536, report, edgesEnd)
    case ExportFailed(why) => Stigi.Store.putReasonWord(report, Stigi.Array.copy[U8](15, "names: failed: ", 0, 15, 65536, report, 0), why)
    case ExportFailedAt(entry, step, why, nameBuf, nameSize) => Stigi.Store.putReasonWord(report, Stigi.Store.putFailedAt{region}(report, entry, nameBuf, nameSize, step), why)
