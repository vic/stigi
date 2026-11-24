// prompt: walk the unit entries once and the edges once, adding every row to its lane and stopping at the first full lane or the first edge outside the image.
def Stigi.Store.collectLanes {Stigi.Mut} (lanes: Stigi.Store.FactsLanes, image: Stigi.Names.Image, edgeTable: Stigi.Names.Edges): Stigi.Store.Collected =
  val Image(_, _, count, _) = image
  val Edges(_, _, edgeCount, _) = edgeTable
  @<lessOrEqual(count, Stigi.Limit.maxUnits)>
  Stigi.Store.collectEntries(lanes, image, 0, count) match
    case CollectStopped(fault) => CollectStopped(fault)
    case LanesFilled(next) => Stigi.Store.collectEdges(next, image, edgeTable, 0, edgeCount)
