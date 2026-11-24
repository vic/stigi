// prompt: write an unsigned number in decimal digits into the report text at a position, giving the new end.
def Stigi.Store.putNumber {region: Stigi.Region, Stigi.Mut} (report: Array[U8](65536), at: U64, value: U64) <lessOrEqual(at, 65536)>: U64 <lessOrEqual(result, 65536)> =
  Stigi.Store.putUnsigned{region}(65536, report, at, value)
