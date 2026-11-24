// prompt: allocate the four bounded lanes for lengthTerm, linearForm, termOffset, and sameLength exports.
def Stigi.Store.allocateLengthLanes {region: Stigi.Region} (laneSize: Stigi.EmitCap): Stigi.Store.LengthLanesMade =
  val Filled(lengthBytes) = Array.tryFill[U8]{region}(laneSize, 0) else
    case FillFailed => LengthLanesUnavailable
  val Filled(linearBytes) = Array.tryFill[U8]{region}(laneSize, 0) else
    case FillFailed => LengthLanesUnavailable
  val Filled(offsetBytes) = Array.tryFill[U8]{region}(laneSize, 0) else
    case FillFailed => LengthLanesUnavailable
  val Filled(sameBytes) = Array.tryFill[U8]{region}(laneSize, 0) else
    case FillFailed => LengthLanesUnavailable
  LengthLanesReady(LengthLanes(Lane(laneSize, lengthBytes, 0), Lane(laneSize, linearBytes, 0), Lane(laneSize, offsetBytes, 0), Lane(laneSize, sameBytes, 0), 0))
