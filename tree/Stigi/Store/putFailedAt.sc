// prompt: write the prefix of a failure line into the report buffer: the unit index, the unit name and the step that failed, giving the new end.
def Stigi.Store.putFailedAt {region: Stigi.Region, Stigi.Mut} (report: Array[U8](65536), entry: U64, nameBuf: Array[U8](Stigi.Limit.maxUnitBytes), nameSize: U64, step: Stigi.Names.ExportStep) <lessOrEqual(nameSize, Stigi.Limit.maxUnitBytes)>: U64 <lessOrEqual(result, 65536)> =
  Stigi.Store.putExportFailedAt{region}(report, entry, nameBuf, nameSize, NamesExport, step)
