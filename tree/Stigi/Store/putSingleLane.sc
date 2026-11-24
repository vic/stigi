// prompt: add one name row to a one column lane and give the lane back, or say the lane is full.
def Stigi.Store.putSingleLane {Stigi.Mut} (lane: Stigi.Store.Lane, poolCap: Stigi.ByteCap, pool: Array[U8](poolCap), first: U64, end: U64) <lessOrEqual(first, end), lessOrEqual(end, poolCap), lessOrEqual(poolCap, Stigi.Limit.maxIndexBytes)>: Stigi.Store.LaneWrite =
  lane match
    case Lane(size, bytes, at) =>
      if Stigi.Store.nameRowSize(first, end) <= size - at then LaneWritten(Lane(size, bytes, Stigi.Store.putNameRow(size, bytes, at, poolCap, pool, first, end))) else LaneOverflow
