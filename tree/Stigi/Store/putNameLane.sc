// prompt: add the unit name row of one unit to the name lane and give the lane back, or say the lane is full.
def Stigi.Store.putNameLane {Stigi.Mut} (lane: Stigi.Store.Lane, pool: Array[U8](Stigi.Limit.maxIndexBytes), first: U64, end: U64) <lessOrEqual(first, end), lessOrEqual(end, Stigi.Limit.maxIndexBytes)>: Stigi.Store.LaneWrite =
  lane match
    case Lane(size, bytes, at) =>
      if Stigi.Store.edgeRowSize(first, end, first, end) <= size - at then LaneWritten(Lane(size, bytes, Stigi.Store.putEdgeRow(size, bytes, at, pool, PoolRun(first, end), PoolRun(first, end)))) else LaneOverflow
