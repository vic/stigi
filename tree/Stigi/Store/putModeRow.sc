// prompt: write the single row of the image mode relation at a position, giving the new end.
def Stigi.Store.putModeRow {Stigi.Mut} (reportCap: Stigi.EmitCap, report: Array[U8](reportCap), at: U64) <lessOrEqual(at, reportCap)>: U64 <lessOrEqual(result, reportCap)> =
  Stigi.Array.copy[U8](7, "Kernel\n", 0, 7, reportCap, report, at)
