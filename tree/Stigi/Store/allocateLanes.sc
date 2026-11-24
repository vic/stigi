// prompt: allocate the six zeroed facts lanes of the given size, or say a lane could not be allocated.
def Stigi.Store.allocateLanes {region: Stigi.Region, Stigi.Mut} (laneSize: U64) <below(laneSize, Stigi.Limit.maxEmitBytes)>: Stigi.Store.LanesMade =
  val Filled(unitBytes) = Array.tryFill[U8]{region}(laneSize, 0) else
    case FillFailed => LanesUnavailable
  val Filled(nameBytes) = Array.tryFill[U8]{region}(laneSize, 0) else
    case FillFailed => LanesUnavailable
  val Filled(stigiBytes) = Array.tryFill[U8]{region}(laneSize, 0) else
    case FillFailed => LanesUnavailable
  val Filled(handlerBytes) = Array.tryFill[U8]{region}(laneSize, 0) else
    case FillFailed => LanesUnavailable
  val Filled(negBytes) = Array.tryFill[U8]{region}(laneSize, 0) else
    case FillFailed => LanesUnavailable
  val Filled(edgeBytes) = Array.tryFill[U8]{region}(laneSize, 0) else
    case FillFailed => LanesUnavailable
  LanesReady(Lanes(Lane(laneSize, unitBytes, 0), Lane(laneSize, nameBytes, 0), Lane(laneSize, stigiBytes, 0), Lane(laneSize, handlerBytes, 0), Lane(laneSize, negBytes, 0), Lane(laneSize, edgeBytes, 0)))
