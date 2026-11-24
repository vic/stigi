// prompt: closed file of the names or types facts export, a lane file, the image mode file, or a types table file.
enum Stigi.Store.FactsFile:
  case LaneFile(lane: Stigi.Store.FactsLane)
  case ModeFile
  case TypeRangeFile
  case TypeWidthFile
