// prompt: write one labeled count of the report summary line at a position, giving the new end.
def Stigi.Store.putCount {region: Stigi.Region, Stigi.Mut} (report: Array[U8](65536), at: U64, size: U64, label: Array[U8](size), value: U64) <lessOrEqual(at, 65536), lessOrEqual(size, 65536)>: U64 <lessOrEqual(result, 65536)> =
  Stigi.Store.putNumber{region}(report, Stigi.Array.copy[U8](size, label, 0, size, 65536, report, at), value)
