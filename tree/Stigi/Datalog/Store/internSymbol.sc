// prompt: intern a symbol slice into an open-addressed hash table.
def Stigi.Datalog.Store.internSymbol {Stigi.Mut} (symCap: U64, poolCap: U64, slotCap: U64, srcCap: U64, pool: Array[U8](poolCap), starts: Array[U64](symCap), ends: Array[U64](symCap), slots: Array[Stigi.Datalog.Store.Slot](slotCap), src: Array[U8](srcCap), first: U64, endPos: U64, symCount: U64, poolEnd: U64) <below(0, slotCap), lessOrEqual(first, endPos), lessOrEqual(endPos, srcCap), lessOrEqual(symCount, symCap), lessOrEqual(poolEnd, poolCap)>: Stigi.Datalog.Store.SymOutcome =
  val startSlot: U64 = Stigi.Datalog.Store.hashBytes(srcCap, src, first, endPos) % slotCap
  var probeStep: U64 = 0
  var doneFlag: Bool = false
  var foundSlot: U64 = 0
  var foundExisting: Bool = false
  var existingSym: U64 = 0
  while !doneFlag && probeStep < slotCap do
    val nextStep: U64 = probeStep + 1
    val slotPos: U64 = U64.wrappingAdd(startSlot, probeStep) % slotCap
    slots(slotPos) match
      case StoreVacant =>
        foundSlot = slotPos
        doneFlag = true
      case StoreOccupied(candSym) =>
        if candSym < symCount then
          val candStart: U64 = starts(candSym)
          val candEnd: U64 = ends(candSym)
          if candStart <= candEnd && candEnd <= poolCap then
            if Stigi.Datalog.Store.sameBytes(poolCap, pool, candStart, candEnd, srcCap, src, first, endPos) then
              foundExisting = true
              existingSym = candSym
              doneFlag = true
            else
              ()
          else
            ()
        else
          ()
    probeStep = nextStep
  if foundExisting then
    Interned(existingSym, symCount, poolEnd)
  else if !doneFlag then
    SymTableFull
  else if symCount >= symCap then
    SymTableFull
  else
    val needLen: U64 = endPos - first
    val availPool: U64 = poolCap - poolEnd
    if needLen > availPool then
      SymTableFull
    else
      var srcPos: U64 = first
      var dstPos: U64 = poolEnd
      while srcPos < endPos && dstPos < poolCap do
        val nextSrc: U64 = srcPos + 1
        val nextDst: U64 = dstPos + 1
        pool(dstPos) := src(srcPos)
        srcPos = nextSrc
        dstPos = nextDst
      starts(symCount) := poolEnd
      ends(symCount) := dstPos
      if foundSlot < slotCap then
        slots(foundSlot) := StoreOccupied(symCount)
      else
        ()
      val newCount: U64 = symCount + 1
      Interned(symCount, newCount, dstPos)
