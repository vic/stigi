// prompt: add the unit row of one unit to the unit lane and give the lane back, or say the lane is full.
def Stigi.Store.putUnitLane {Stigi.Mut} (lane: Stigi.Store.Lane, poolCap: Stigi.ByteCap, pool: Array[U8](poolCap), first: U64, end: U64, kind: Stigi.Names.UnitKind) <lessOrEqual(first, end), lessOrEqual(end, poolCap), lessOrEqual(poolCap, Stigi.Limit.maxIndexBytes)>: Stigi.Store.LaneWrite =
  lane match
    case Lane(size, bytes, at) =>
      if Stigi.Store.unitRowSize(first, end, kind) <= size - at then LaneWritten(Lane(size, bytes, Stigi.Store.putUnitRow(size, bytes, at, poolCap, pool, first, end, kind))) else LaneOverflow
