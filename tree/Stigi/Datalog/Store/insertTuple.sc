// prompt: insert a tuple into an open-addressed dedup store and link it in its relation chain.
pub def Stigi.Datalog.Store.insertTuple {Stigi.Mut} (relCap: U64, rowCap: U64, tupleSlotCap: U64, rows: Array[Stigi.Datalog.Store.Tuple](rowCap), slots: Array[Stigi.Datalog.Store.Slot](tupleSlotCap), relHead: Array[Stigi.Datalog.Store.Slot](relCap), nextInRel: Array[Stigi.Datalog.Store.Slot](rowCap), row: Stigi.Datalog.Store.Tuple, rowCount: U64) <below(0, tupleSlotCap), lessOrEqual(rowCount, rowCap)>: Stigi.Datalog.Store.TupleOutcome =
  val startSlot: U64 = Stigi.Datalog.Store.hashTuple(row) % tupleSlotCap
  var probeStep: U64 = 0
  var doneFlag: Bool = false
  var foundSlot: U64 = 0
  var foundDup: Bool = false
  var dupIndex: U64 = 0
  while !doneFlag && probeStep < tupleSlotCap do
    val nextStep: U64 = probeStep + 1
    val slotPos: U64 = U64.wrappingAdd(startSlot, probeStep) % tupleSlotCap
    slots(slotPos) match
      case StoreVacant =>
        foundSlot = slotPos
        doneFlag = true
      case StoreOccupied(candRow) =>
        if candRow < rowCount then
          if Stigi.Datalog.Store.sameTuple(rows(candRow), row) then
            foundDup = true
            dupIndex = candRow
            doneFlag = true
          else
            ()
        else
          ()
    probeStep = nextStep
  if foundDup then
    TupleDuplicate(dupIndex)
  else if !doneFlag then
    TupleTableFull
  else if rowCount >= rowCap then
    TupleTableFull
  else
    rows(rowCount) := row
    if foundSlot < tupleSlotCap then
      slots(foundSlot) := StoreOccupied(rowCount)
    else
      ()
    row match
      case StoreRow(relNum, _, _, _, _, _, _, _, _) =>
        if relNum < relCap then
          val prevHead: Stigi.Datalog.Store.Slot = relHead(relNum)
          nextInRel(rowCount) := prevHead
          relHead(relNum) := StoreOccupied(rowCount)
        else
          ()
    val newCount: U64 = rowCount + 1
    TupleAdded(rowCount, newCount)
