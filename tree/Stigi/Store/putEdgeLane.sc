// prompt: add one edge row to the edge lane and give the lane back, or say the lane is full.
def Stigi.Store.putEdgeLane {Stigi.Mut} (lane: Stigi.Store.Lane, pool: Array[U8](Stigi.Limit.maxIndexBytes), fromFirst: U64, fromEnd: U64, toFirst: U64, toEnd: U64) <lessOrEqual(fromFirst, fromEnd), lessOrEqual(fromEnd, Stigi.Limit.maxIndexBytes), lessOrEqual(toFirst, toEnd), lessOrEqual(toEnd, Stigi.Limit.maxIndexBytes)>: Stigi.Store.LaneWrite =
  lane match
    case Lane(size, bytes, at) =>
      if Stigi.Store.edgeRowSize(fromFirst, fromEnd, toFirst, toEnd) <= size - at then LaneWritten(Lane(size, bytes, Stigi.Store.putEdgeRow(size, bytes, at, pool, PoolRun(fromFirst, fromEnd), PoolRun(toFirst, toEnd)))) else LaneOverflow
