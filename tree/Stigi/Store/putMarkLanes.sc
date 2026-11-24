// prompt: add the under Stigi row of one unit when its name starts with Stigi and the handler or negtest row its kind owes, give the lanes back or say which lane is full.
def Stigi.Store.putMarkLanes {Stigi.Mut} (lanes: Stigi.Store.FactsLanes, poolCap: Stigi.ByteCap, pool: Array[U8](poolCap), first: U64, end: U64, kind: Stigi.Names.UnitKind) <lessOrEqual(first, end), lessOrEqual(end, poolCap), lessOrEqual(poolCap, Stigi.Limit.maxIndexBytes)>: Stigi.Store.Collected =
  lanes match
    case Lanes(unitLane, nameLane, stigiLane, handlerLane, negLane, edgeLane) =>
      val under: Stigi.Store.LaneWrite = if first + 6 <= end && Stigi.Lex.sliceEquals(poolCap, pool, first, first + 6, 6, "Stigi.") then Stigi.Store.putSingleLane(stigiLane, poolCap, pool, first, end) else LaneWritten(stigiLane)
      under match
        case LaneOverflow => CollectStopped(LaneFull(StigiLane))
        case LaneWritten(stigiNext) => kind match
          case HandlerKind => Stigi.Store.putSingleLane(handlerLane, poolCap, pool, first, end) match
            case LaneOverflow => CollectStopped(LaneFull(HandlerLane))
            case LaneWritten(handlerNext) => LanesFilled(Lanes(unitLane, nameLane, stigiNext, handlerNext, negLane, edgeLane))
          case NegTestKind => Stigi.Store.putSingleLane(negLane, poolCap, pool, first, end) match
            case LaneOverflow => CollectStopped(LaneFull(NegTestLane))
            case LaneWritten(negNext) => LanesFilled(Lanes(unitLane, nameLane, stigiNext, handlerLane, negNext, edgeLane))
          case DefKind | ValKind | EnumKind | TypeKind | CapabilityKind | TraitKind | GivenKind | ExternKind | RelationKind | RuleKind | TestKind => LanesFilled(Lanes(unitLane, nameLane, stigiNext, handlerLane, negLane, edgeLane))
