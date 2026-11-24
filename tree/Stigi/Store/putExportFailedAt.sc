// prompt: write the prefix of a failure line into the report buffer: the export prefix, unit index, unit name and the step that failed, giving the new end.
def Stigi.Store.putExportFailedAt {region: Stigi.Region, Stigi.Mut} (report: Array[U8](65536), entry: U64, nameBuf: Array[U8](Stigi.Limit.maxUnitBytes), nameSize: U64, kind: Stigi.Store.ExportKind, step: Stigi.Names.ExportStep) <lessOrEqual(nameSize, Stigi.Limit.maxUnitBytes)>: U64 <lessOrEqual(result, 65536)> =
  val prefixed: U64 = Stigi.Store.putExportPrefix(report, 0, kind)
  val head: U64 = Stigi.Store.putCount{region}(report, prefixed, 15, "failed at unit ", entry)
  val open: U64 = Stigi.Array.copy[U8](2, " (", 0, 2, 65536, report, head)
  val named: U64 = Stigi.Array.copy[U8](Stigi.Limit.maxUnitBytes, nameBuf, 0, nameSize, 65536, report, open)
  step match
    case IndexStep => Stigi.Array.copy[U8](16, ") during index: ", 0, 16, 65536, report, named)
    case WalkStep => Stigi.Array.copy[U8](15, ") during walk: ", 0, 15, 65536, report, named)
