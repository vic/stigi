// prompt: the six facts lanes of one export, each lane knowing its own size and the end of its rows.
enum Stigi.Store.FactsLanes:
  case Lanes(unitLane: Stigi.Store.Lane, nameLane: Stigi.Store.Lane, stigiLane: Stigi.Store.Lane, handlerLane: Stigi.Store.Lane, negLane: Stigi.Store.Lane, edgeLane: Stigi.Store.Lane)
