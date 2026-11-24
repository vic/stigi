// prompt: derive one row while the row budget has room.
def Stigi.Datalog.emitRow {Stigi.Mut} (rowCap: Stigi.NodeCap, rows: Array[Stigi.Datalog.TupleRow](rowCap), slots: Array[Stigi.Datalog.Slot](rowCap), total: U64, row: Stigi.Datalog.TupleRow) <below(0, rowCap)>: Stigi.Datalog.Derive =
  if total < rowCap then Derived(Stigi.Datalog.insertRow(rowCap, rows, total, slots, row)) else DeriveFull
