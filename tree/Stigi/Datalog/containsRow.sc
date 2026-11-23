// prompt: check whether a tuple row is already present in the open-addressed hash index.
def Stigi.Datalog.containsRow(rowCap: Stigi.NodeCap, rows: Array[Stigi.Datalog.TupleRow](rowCap), rowCount: U64, hashCap: Stigi.NodeCap, bucketUsed: Array[Bool](hashCap), bucketRow: Array[U64](hashCap), cand: Stigi.Datalog.TupleRow) <lessOrEqual(rowCount, rowCap), below(0, hashCap)>: Bool =
  val startSlot: U64 = Stigi.Datalog.tupleKey(cand, hashCap)
  var step: U64 = 0
  var found: Bool = false
  var done: Bool = false
  while step < hashCap && !done do
    @<lessOrEqual(step, hashCap)>
    val slot: U64 = (startSlot + step) % hashCap
    if !bucketUsed(slot) then
      done = true
    else
      val idx: U64 = bucketRow(slot)
      if idx < rowCount && Stigi.Datalog.sameRow(rows(idx), cand) then
        found = true
        done = true
      else ()
    step = step + 1
  found
