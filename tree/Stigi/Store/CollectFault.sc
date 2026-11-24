// prompt: closed fault that stops the collection of the facts lanes.
enum Stigi.Store.CollectFault:
  case LaneFull(lane: Stigi.Store.FactsLane)
  case EdgeOutsideImage(edge: U64)
