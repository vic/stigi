// prompt: count the linked rows in one relation chain of the Datalog tuple store.
pub def Stigi.Datalog.Engine.countRelationRows(relCap: U64, rowCap: U64, relHead: Array[Stigi.Datalog.Store.Slot](relCap), nextInRel: Array[Stigi.Datalog.Store.Slot](rowCap), relNum: U64) <below(relNum, relCap)>: U64 =
  var curSlot: Stigi.Datalog.Store.Slot = relHead(relNum)
  var chainCount: U64 = 0
  var keepGoing: Bool = true
  while keepGoing && chainCount < rowCap do
    @<lessOrEqual(chainCount, rowCap)>
    curSlot match
      case StoreVacant =>
        keepGoing = false
      case StoreOccupied(rowIdx) =>
        val nextCount: U64 = chainCount + 1
        chainCount = nextCount
        if rowIdx < rowCap then
          curSlot = nextInRel(rowIdx)
        else
          keepGoing = false
  chainCount
