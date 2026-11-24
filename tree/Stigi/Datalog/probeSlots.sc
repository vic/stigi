// prompt: probe the row index from a slot for a row equal to a candidate, among the first stored rows.
def Stigi.Datalog.probeSlots(rowCap: Stigi.NodeCap, rows: Array[Stigi.Datalog.TupleRow](rowCap), rowCount: U64, slots: Array[Stigi.Datalog.Slot](rowCap), cand: Stigi.Datalog.TupleRow, slot: U64, remaining: U64) <lessOrEqual(rowCount, rowCap), below(slot, rowCap), decreases(remaining)>: Stigi.Datalog.Probe =
  if remaining < 1 then ProbeFull else
    slots(slot) match
      case Empty => FreeAt(slot)
      case Holds(rowIdx) =>
        if rowIdx < rowCount && Stigi.Datalog.sameRow(rows(rowIdx), cand) then FoundAt(rowIdx) else Stigi.Datalog.probeSlots(rowCap, rows, rowCount, slots, cand, (slot + 1) % rowCap, remaining - 1)
