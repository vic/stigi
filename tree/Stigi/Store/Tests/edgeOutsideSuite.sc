// prompt: verify an edge that points outside the image stops the collection with the number of that edge after the first edge row was written.
val Stigi.Store.Tests.edgeOutsideSuite {Stigi.Test}: Stigi.Test.Result =
  val passed: Bool =
    val Sampled(image, edges) = Stigi.Names.Tests.threeUnits{Stigi.Region}(1, 7) else
      case SampleUnavailable => false
    Stigi.Store.allocateLanes{Stigi.Region}(64) match
      case LanesReady(lanes) =>
        val stopped: Bool = Stigi.Store.collectLanes(lanes, image, edges) is CollectStopped(EdgeOutsideImage(1))
        val written: Bool = lanes match
          case Lanes(_, _, _, _, _, edgeLane) => edgeLane match
            case Lane(size, bytes, _) => if 12 <= size then Stigi.Store.Tests.laneIs(Lane(size, bytes, 12), 12, "A.d\tStigi.h\n") else false
        stopped && written
      case LanesUnavailable => false
  Stigi.Test.expect(passed, true, "an edge outside the image stops the collection at its number after the first edge")
