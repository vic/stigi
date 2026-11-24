// prompt: store a row after the first stored rows when no equal row is stored, giving the new row count.
def Stigi.Datalog.insertRow {Stigi.Mut} (rowCap: Stigi.NodeCap, rows: Array[Stigi.Datalog.TupleRow](rowCap), rowCount: U64, slots: Array[Stigi.Datalog.Slot](rowCap), cand: Stigi.Datalog.TupleRow) <below(rowCount, rowCap), below(0, rowCap)>: U64 <lessOrEqual(rowCount, result)> =
  Stigi.Datalog.probeSlots(rowCap, rows, rowCount, slots, cand, Stigi.Datalog.tupleKey(cand, rowCap), rowCap) match
    case FreeAt(slot) =>
      if slot < rowCap then
        rows(rowCount) := cand
        slots(slot) := Holds(rowCount)
        rowCount + 1
      else rowCount
    case FoundAt(_) | ProbeFull => rowCount
