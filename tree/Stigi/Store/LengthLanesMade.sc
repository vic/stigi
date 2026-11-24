// prompt: closed outcome of allocating the four bounded length fact lanes.
enum Stigi.Store.LengthLanesMade:
  case LengthLanesReady(lanes: Stigi.Store.LengthLanes)
  case LengthLanesUnavailable
