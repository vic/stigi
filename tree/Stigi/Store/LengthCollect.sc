// prompt: closed outcome of collecting length facts across a unit or tree.
enum Stigi.Store.LengthCollect:
  case LengthsCollected(lanes: Stigi.Store.LengthLanes)
  case LengthsStopped(failure: Stigi.Store.FactsFailure)
