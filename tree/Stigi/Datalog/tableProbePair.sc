// prompt: probe an open-addressing hash set for a binary pair returning one if present and zero otherwise.
def Stigi.Datalog.tableProbePair(slotCount: U64, rowCap: U64, usedSlots: Array[U8](slotCount), slotRows: Array[U64](slotCount), leftColumn: Array[U64](rowCap), rightColumn: Array[U64](rowCap), targetLeft: U64, targetRight: U64) <below(0, slotCount), below(slotCount, 0x100000000), below(0, rowCap)>: U64 =
  val startSlot: U64 = Stigi.Datalog.hashPair(targetLeft, targetRight) % slotCount
  var cursor: U64 = startSlot
  var steps: U64 = 0
  var found: U64 = 0
  while steps < slotCount do
    @<below(cursor, slotCount), lessOrEqual(steps, slotCount), lessOrEqual(found, 1), decreases(slotCount - steps)>
    val occupied: U8 = usedSlots(cursor)
    if occupied == 0 then
      steps = slotCount
    else
      val rowIndex: U64 = slotRows(cursor)
      if rowIndex < rowCap then
        if leftColumn(rowIndex) == targetLeft && rightColumn(rowIndex) == targetRight then
          found = 1
          steps = slotCount
        else
          val nextStep: U64 = steps + 1
          steps = nextStep
          val nextCursor: U64 = cursor + 1
          cursor = if nextCursor < slotCount then nextCursor else 0
      else
        steps = slotCount
  found
