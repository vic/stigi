// prompt: index the seed rows of a scan into distinct stored rows, giving their count.
def Stigi.Datalog.seedRows {Stigi.Mut} (rowCap: Stigi.NodeCap, rows: Array[Stigi.Datalog.TupleRow](rowCap), slots: Array[Stigi.Datalog.Slot](rowCap), scan: Stigi.Datalog.Scan, remaining: U64) <below(0, rowCap), decreases(remaining)>: U64 =
  val Scan(idx, stop, total) = scan
  if remaining < 1 || stop <= idx || rowCap <= idx || rowCap <= total then total else
    Stigi.Datalog.seedRows(rowCap, rows, slots, Scan(idx + 1, stop, Stigi.Datalog.insertRow(rowCap, rows, total, slots, rows(idx))), remaining - 1)
