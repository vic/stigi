// prompt: probe the open-addressed tuple store for membership of one tuple.
pub def Stigi.Datalog.Store.hasTuple(rowCap: U64, tupleSlotCap: U64, rows: Array[Stigi.Datalog.Store.Tuple](rowCap), slots: Array[Stigi.Datalog.Store.Slot](tupleSlotCap), row: Stigi.Datalog.Store.Tuple, rowCount: U64) <below(0, tupleSlotCap), lessOrEqual(rowCount, rowCap)>: Bool =
  val startSlot: U64 = Stigi.Datalog.Store.hashTuple(row) % tupleSlotCap
  var probeStep: U64 = 0
  var doneFlag: Bool = false
  var foundFlag: Bool = false
  while !doneFlag && probeStep < tupleSlotCap do
    val nextStep: U64 = probeStep + 1
    val slotPos: U64 = U64.wrappingAdd(startSlot, probeStep) % tupleSlotCap
    slots(slotPos) match
      case StoreVacant =>
        doneFlag = true
      case StoreOccupied(candRow) =>
        if candRow < rowCount then
          if Stigi.Datalog.Store.sameTuple(rows(candRow), row) then
            foundFlag = true
            doneFlag = true
          else
            ()
        else
          ()
    probeStep = nextStep
  foundFlag
