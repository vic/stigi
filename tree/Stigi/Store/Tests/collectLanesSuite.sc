// prompt: verify the lane collector gives every lane the exact rows and the exact end of three units and two edges.
val Stigi.Store.Tests.collectLanesSuite {Stigi.Test}: Stigi.Test.Result =
  val code: U64 =
    val Sampled(image, edges) = Stigi.Names.Tests.threeUnits{Stigi.Region}(2, 0) else
      case SampleUnavailable => 99
    Stigi.Store.allocateLanes{Stigi.Region}(64) match
      case LanesUnavailable => 99
      case LanesReady(lanes) => Stigi.Store.collectLanes(lanes, image, edges) match
        case CollectStopped(_) => 1
        case LanesFilled(filled) => filled match
          case Lanes(unitLane, nameLane, stigiLane, handlerLane, negLane, edgeLane) =>
            if Stigi.Store.Tests.laneIs(unitLane, 36, "A.d\tDef\nStigi.h\tHandler\nA.n\tNegTest\n") then
              if Stigi.Store.Tests.laneIs(nameLane, 32, "A.d\tA.d\nStigi.h\tStigi.h\nA.n\tA.n\n") then
                if Stigi.Store.Tests.laneIs(stigiLane, 8, "Stigi.h\n") && Stigi.Store.Tests.laneIs(handlerLane, 8, "Stigi.h\n") then
                  if Stigi.Store.Tests.laneIs(negLane, 4, "A.n\n") && Stigi.Store.Tests.laneIs(edgeLane, 20, "A.d\tStigi.h\nA.n\tA.d\n") then 0 else 5
                else 4
              else 3
            else 2
  if code == 0 then Stigi.Test.expect(true, true, "every lane holds the exact rows and the exact end") else if code == 1 then Stigi.Test.expect(false, true, "clause the lanes filled") else if code == 2 then Stigi.Test.expect(false, true, "clause the unit lane") else if code == 3 then Stigi.Test.expect(false, true, "clause the name lane") else if code == 4 then Stigi.Test.expect(false, true, "clause the stigi and handler lanes") else if code == 5 then Stigi.Test.expect(false, true, "clause the negtest and edge lanes") else Stigi.Test.expect(false, true, "clause setup")
