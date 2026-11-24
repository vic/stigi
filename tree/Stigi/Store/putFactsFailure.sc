// prompt: write the single failure line of a names facts export into the report buffer at a position, giving the new end.
def Stigi.Store.putFactsFailure {region: Stigi.Region, Stigi.Mut} (report: Array[U8](65536), at: U64, failure: Stigi.Store.FactsFailure) <lessOrEqual(at, 65536)>: U64 <lessOrEqual(result, 65536)> =
  Stigi.Store.putExportFailure{region}(report, at, NamesExport, failure)
