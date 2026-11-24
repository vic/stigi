// prompt: write the prefix of a failure line into the report buffer: the unit index, the unit name and the step that failed, giving the new end.
def Stigi.Store.putFailedAt {region: Stigi.Region, Stigi.Mut} (report: Array[U8](65536), entry: U64, nameBuf: Array[U8](Stigi.Limit.maxUnitBytes), nameSize: U64, step: Stigi.Names.ExportStep) <lessOrEqual(nameSize, Stigi.Limit.maxUnitBytes)>: U64 <lessOrEqual(result, 65536)> =
  val head: U64 = Stigi.Store.putCount{region}(report, 0, 22, "names: failed at unit ", entry)
  val open: U64 = Stigi.Array.copy[U8](2, " (", 0, 2, 65536, report, head)
  val named: U64 = Stigi.Array.copy[U8](Stigi.Limit.maxUnitBytes, nameBuf, 0, nameSize, 65536, report, open)
  step match
    case IndexStep => Stigi.Array.copy[U8](16, ") during index: ", 0, 16, 65536, report, named)
    case WalkStep => Stigi.Array.copy[U8](15, ") during walk: ", 0, 15, 65536, report, named)
