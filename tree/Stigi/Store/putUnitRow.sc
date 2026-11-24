// prompt: write one row of the unit relation, the unit name from the name pool, a tab, the kind word and a newline, at a position, giving the new end.
def Stigi.Store.putUnitRow {Stigi.Mut} (reportCap: Stigi.EmitCap, report: Array[U8](reportCap), at: U64, poolCap: Stigi.ByteCap, pool: Array[U8](poolCap), first: U64, end: U64, kind: Stigi.Names.UnitKind) <lessOrEqual(at, reportCap), lessOrEqual(first, end), lessOrEqual(end, poolCap)>: U64 <lessOrEqual(result, reportCap)> =
  val named: U64 = Stigi.Array.copy[U8](poolCap, pool, first, end, reportCap, report, at)
  val tabbed: U64 = Stigi.Array.copy[U8](1, "\t", 0, 1, reportCap, report, named)
  Stigi.Array.copy[U8](1, "\n", 0, 1, reportCap, report, Stigi.Store.putKindWord(reportCap, report, tabbed, kind))
