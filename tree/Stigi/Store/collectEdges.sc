// prompt: add the row of every dependency edge from an index on to the edge lane, stopping at the first full lane or the first edge outside the image.
def Stigi.Store.collectEdges {Stigi.Mut} (lanes: Stigi.Store.FactsLanes, image: Stigi.Names.Image, edgeTable: Stigi.Names.Edges, edge: U64, remaining: U64) <decreases(remaining)>: Stigi.Store.Collected =
  val Image(pool, entries, count, _) = image
  val Edges(sources, targets, edgeCount, _) = edgeTable
  if remaining < 1 || edgeCount <= edge then LanesFilled(lanes) else
    val source: U64 = sources(edge)
    val target: U64 = targets(edge)
    if source < count && target < count then
      entries(source) match
        case Entry(sourceFirst, sourceEnd, _, _) =>
          entries(target) match
            case Entry(targetFirst, targetEnd, _, _) => lanes match
              case Lanes(unitLane, nameLane, stigiLane, handlerLane, negLane, edgeLane) => Stigi.Store.putEdgeLane(edgeLane, pool, sourceFirst, sourceEnd, targetFirst, targetEnd) match
                case LaneOverflow => CollectStopped(LaneFull(EdgeLane))
                case LaneWritten(edgeNext) => Stigi.Store.collectEdges(Lanes(unitLane, nameLane, stigiLane, handlerLane, negLane, edgeNext), image, edgeTable, edge + 1, remaining - 1)
    else CollectStopped(EdgeOutsideImage(edge))
