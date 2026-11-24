// prompt: closed file of the names facts export, a lane file or the image mode file.
enum Stigi.Store.FactsFile:
  case LaneFile(lane: Stigi.Store.FactsLane)
  case ModeFile
