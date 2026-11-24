// prompt: closed fault of collecting facts lanes for names or types exports.
enum Stigi.Store.CollectFault:
  case LaneFull(lane: Stigi.Store.FactsLane)
  case EdgeOutsideImage(edge: U64)
  case TypeLaneFull(file: Stigi.Store.FactsFile)
