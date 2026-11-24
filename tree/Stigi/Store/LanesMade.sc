// prompt: closed outcome of allocating the facts lanes: every lane was allocated, or one could not be.
enum Stigi.Store.LanesMade:
  case LanesReady(lanes: Stigi.Store.FactsLanes)
  case LanesUnavailable
