// prompt: verify a lane that fits exactly is accepted, one byte less stops with a full unit lane and leaves the name lane holding the rows before it, a handler lane too small stops with a full handler lane, and every row writer takes a row that fills the room after the end exactly and refuses one that fits the lane size but not that room.
val Stigi.Store.Tests.laneFitSuite {Stigi.Test}: Stigi.Test.Result =
  val passed: Bool =
    val Sampled(image, edges) = Stigi.Names.Tests.threeUnits{Stigi.Region}(2, 0) else
      case SampleUnavailable => false
    val fits: Bool = Stigi.Store.allocateLanes{Stigi.Region}(36) match
      case LanesReady(lanes) => Stigi.Store.collectLanes(lanes, image, edges) is LanesFilled(_)
      case LanesUnavailable => false
    val stops: Bool = Stigi.Store.allocateLanes{Stigi.Region}(35) match
      case LanesReady(lanes) =>
        val full: Bool = Stigi.Store.collectLanes(lanes, image, edges) is CollectStopped(LaneFull(UnitLane))
        val inside: Bool = lanes match
          case Lanes(_, nameLane, _, _, _, _) => nameLane match
            case Lane(size, bytes, _) => if 24 <= size then Stigi.Store.Tests.laneIs(Lane(size, bytes, 24), 24, "A.d\tA.d\nStigi.h\tStigi.h\n") else false
        full && inside
      case LanesUnavailable => false
    val handlerFull: Bool = Stigi.Store.allocateLanes{Stigi.Region}(64) match
      case LanesReady(lanes) => lanes match
        case Lanes(unitLane, nameLane, stigiLane, _, negLane, edgeLane) =>
          val Filled(tiny) = Array.tryFill[U8]{Stigi.Region}(7, 0) else
            case FillFailed => false
          Stigi.Store.collectLanes(Lanes(unitLane, nameLane, stigiLane, Lane(7, tiny, 0), negLane, edgeLane), image, edges) is CollectStopped(LaneFull(HandlerLane))
      case LanesUnavailable => false
    val crowded: Bool =
      val Filled(room) = Array.tryFill[U8]{Stigi.Region}(10, 0) else
        case FillFailed => false
      val Filled(pool) = Array.tryFill[U8]{Stigi.Region}(Stigi.Limit.maxIndexBytes, 0) else
        case FillFailed => false
      val refused: Bool = Stigi.Store.putSingleLane(Lane(10, room, 5), 7, "Stigi.h", 0, 7) is LaneOverflow && Stigi.Store.putNameLane(Lane(10, room, 5), pool, 0, 2) is LaneOverflow && Stigi.Store.putEdgeLane(Lane(10, room, 5), pool, 0, 2, 0, 2) is LaneOverflow && Stigi.Store.putUnitLane(Lane(10, room, 5), 2, "ab", 0, 2, DefKind) is LaneOverflow
      val taken: Bool = Stigi.Store.putSingleLane(Lane(10, room, 7), 2, "ab", 0, 2) is LaneWritten(_) && Stigi.Store.putNameLane(Lane(10, room, 4), pool, 0, 2) is LaneWritten(_) && Stigi.Store.putEdgeLane(Lane(10, room, 4), pool, 0, 2, 0, 2) is LaneWritten(_) && Stigi.Store.putUnitLane(Lane(10, room, 3), 2, "ab", 0, 2, DefKind) is LaneWritten(_)
      refused && taken
    fits && stops && handlerFull && crowded
  Stigi.Test.expect(passed, true, "an exact fit is accepted, one byte less stops with a full unit lane, a handler lane too small stops with a full handler lane and every row writer takes a row that fills the room exactly and refuses one past it")
