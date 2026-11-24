// prompt: write one row of a two column relation, the name of the first pool run, a tab, the name of the second pool run and a newline, at a position, giving the new end.
def Stigi.Store.putEdgeRow {Stigi.Mut} (reportCap: Stigi.EmitCap, report: Array[U8](reportCap), at: U64, pool: Array[U8](Stigi.Limit.maxIndexBytes), from: Stigi.Names.PoolRun, to: Stigi.Names.PoolRun) <lessOrEqual(at, reportCap)>: U64 <lessOrEqual(result, reportCap)> =
  val PoolRun(fromFirst, fromEnd) = from
  val PoolRun(toFirst, toEnd) = to
  val named: U64 = Stigi.Array.copy[U8](Stigi.Limit.maxIndexBytes, pool, fromFirst, fromEnd, reportCap, report, at)
  val tabbed: U64 = Stigi.Array.copy[U8](1, "\t", 0, 1, reportCap, report, named)
  val target: U64 = Stigi.Array.copy[U8](Stigi.Limit.maxIndexBytes, pool, toFirst, toEnd, reportCap, report, tabbed)
  Stigi.Array.copy[U8](1, "\n", 0, 1, reportCap, report, target)
