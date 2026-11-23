// prompt: insert a tuple row into the hash index if absent and return true when newly added.
def Stigi.Datalog.insertUnique {Stigi.Mut} (rowCap: Stigi.NodeCap, rows: Array[Stigi.Datalog.TupleRow](rowCap), rowCount: U64, hashCap: Stigi.NodeCap, bucketUsed: Array[Bool](hashCap), bucketRow: Array[U64](hashCap), cand: Stigi.Datalog.TupleRow) <below(rowCount, rowCap), below(0, hashCap)>: Bool =
  val startSlot: U64 = Stigi.Datalog.tupleKey(cand, hashCap)
  var step: U64 = 0
  var inserted: Bool = false
  var done: Bool = false
  while step < hashCap && !done do
    @<lessOrEqual(step, hashCap)>
    val slot: U64 = (startSlot + step) % hashCap
    if !bucketUsed(slot) then
      rows(rowCount) := cand
      bucketUsed(slot) := true
      bucketRow(slot) := rowCount
      inserted = true
      done = true
    else
      val idx: U64 = bucketRow(slot)
      if idx < rowCount && Stigi.Datalog.sameRow(rows(idx), cand) then
        done = true
      else ()
    step = step + 1
  inserted
