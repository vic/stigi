// prompt: write the rows of one lane of the facts lanes up to the end of that lane to its file of the existing output directory.
def Stigi.Store.writeLaneFile {region: Stigi.Region, Stigi.Mut} (outCap: Stigi.ByteCap, outBuf: Array[U8](outCap), outStart: U64, outEnd: U64, lanes: Stigi.Store.FactsLanes, which: Stigi.Store.FactsLane) <lessOrEqual(outStart, outEnd), lessOrEqual(outEnd, outCap)>: Stigi.Store.FactsOutcome =
  lanes match
    case Lanes(unitLane, nameLane, stigiLane, handlerLane, negLane, edgeLane) =>
      val chosen: Stigi.Store.Lane = which match
        case UnitLane => unitLane
        case NameLane => nameLane
        case StigiLane => stigiLane
        case HandlerLane => handlerLane
        case NegTestLane => negLane
        case EdgeLane => edgeLane
      chosen match
        case Lane(size, bytes, end) => Stigi.Store.writeFactsFile{region}(outCap, outBuf, outStart, outEnd, LaneFile(which), size, bytes, end)
