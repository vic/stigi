// prompt: add the rows one unit gives to the five unit lanes, its unit, name, under Stigi, handler and negtest rows, stopping at the first full lane.
def Stigi.Store.putEntryLanes {Stigi.Mut} (lanes: Stigi.Store.FactsLanes, pool: Array[U8](Stigi.Limit.maxIndexBytes), first: U64, end: U64, kind: Stigi.Names.UnitKind) <lessOrEqual(first, end), lessOrEqual(end, Stigi.Limit.maxIndexBytes)>: Stigi.Store.Collected =
  lanes match
    case Lanes(unitLane, nameLane, stigiLane, handlerLane, negLane, edgeLane) =>
      Stigi.Store.putUnitLane(unitLane, Stigi.Limit.maxIndexBytes, pool, first, end, kind) match
        case LaneOverflow => CollectStopped(LaneFull(UnitLane))
        case LaneWritten(unitNext) => Stigi.Store.putNameLane(nameLane, pool, first, end) match
          case LaneOverflow => CollectStopped(LaneFull(NameLane))
          case LaneWritten(nameNext) => Stigi.Store.putMarkLanes(Lanes(unitNext, nameNext, stigiLane, handlerLane, negLane, edgeLane), Stigi.Limit.maxIndexBytes, pool, first, end, kind)
