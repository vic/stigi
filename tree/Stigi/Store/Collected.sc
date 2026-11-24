// prompt: closed outcome of filling the facts lanes: every lane took its rows and the filled lanes are given back, or the collection stopped with a typed fault.
enum Stigi.Store.Collected:
  case LanesFilled(lanes: Stigi.Store.FactsLanes)
  case CollectStopped(fault: Stigi.Store.CollectFault)
