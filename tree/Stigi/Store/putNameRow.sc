// prompt: write one row of a one column relation, the name of a pool run and a newline, at a position, giving the new end.
def Stigi.Store.putNameRow {Stigi.Mut} (reportCap: Stigi.EmitCap, report: Array[U8](reportCap), at: U64, poolCap: Stigi.ByteCap, pool: Array[U8](poolCap), first: U64, end: U64) <lessOrEqual(at, reportCap), lessOrEqual(first, end), lessOrEqual(end, poolCap)>: U64 <lessOrEqual(result, reportCap)> =
  Stigi.Array.copy[U8](1, "\n", 0, 1, reportCap, report, Stigi.Array.copy[U8](poolCap, pool, first, end, reportCap, report, at))
