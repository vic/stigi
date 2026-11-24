// prompt: closed outcome of adding one row to one lane: the lane with the row, or the lane had no room.
enum Stigi.Store.LaneWrite:
  case LaneWritten(lane: Stigi.Store.Lane)
  case LaneOverflow
