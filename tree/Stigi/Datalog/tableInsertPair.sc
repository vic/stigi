// prompt: insert a binary pair with provenance into columnar arrays and open-addressing hash set if absent.
def Stigi.Datalog.tableInsertPair {Stigi.Mut} (slotCount: U64, rowCap: U64, rowCount: U64, usedSlots: Array[U8](slotCount), slotRows: Array[U64](slotCount), leftColumn: Array[U64](rowCap), rightColumn: Array[U64](rowCap), ruleColumn: Array[U64](rowCap), roundColumn: Array[U64](rowCap), valueLeft: U64, valueRight: U64, ruleIndex: U64, roundNumber: U64) <below(0, slotCount), below(slotCount, 0x100000000), below(rowCount, rowCap)>: U64 <lessOrEqual(rowCount, result), lessOrEqual(result, rowCap)> =
  val startSlot: U64 = Stigi.Datalog.hashPair(valueLeft, valueRight) % slotCount
  var cursor: U64 = startSlot
  var steps: U64 = 0
  var inserted: U64 = 0
  while steps < slotCount do
    @<below(cursor, slotCount), lessOrEqual(steps, slotCount), lessOrEqual(inserted, 1), decreases(slotCount - steps)>
    val occupied: U8 = usedSlots(cursor)
    if occupied == 0 then
      leftColumn(rowCount) := valueLeft
      rightColumn(rowCount) := valueRight
      ruleColumn(rowCount) := ruleIndex
      roundColumn(rowCount) := roundNumber
      slotRows(cursor) := rowCount
      usedSlots(cursor) := 1
      inserted = 1
      steps = slotCount
    else
      val rowIndex: U64 = slotRows(cursor)
      if rowIndex < rowCap && leftColumn(rowIndex) == valueLeft && rightColumn(rowIndex) == valueRight then
        steps = slotCount
      else
        val nextStep: U64 = steps + 1
        steps = nextStep
        val nextCursor: U64 = cursor + 1
        cursor = if nextCursor < slotCount then nextCursor else 0
  if inserted == 1 then rowCount + 1 else rowCount
