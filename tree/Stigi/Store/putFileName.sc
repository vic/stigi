// prompt: write the file name of a facts file into a buffer at a position, giving the new end.
def Stigi.Store.putFileName {Stigi.Mut} (reportCap: Stigi.EmitCap, report: Array[U8](reportCap), at: U64, file: Stigi.Store.FactsFile) <lessOrEqual(at, reportCap)>: U64 <lessOrEqual(result, reportCap)> =
  file match
    case LaneFile(UnitLane) => Stigi.Array.copy[U8](10, "unit.facts", 0, 10, reportCap, report, at)
    case LaneFile(NameLane) => Stigi.Array.copy[U8](14, "unitName.facts", 0, 14, reportCap, report, at)
    case LaneFile(StigiLane) => Stigi.Array.copy[U8](16, "underStigi.facts", 0, 16, reportCap, report, at)
    case LaneFile(HandlerLane) => Stigi.Array.copy[U8](17, "handlerUnit.facts", 0, 17, reportCap, report, at)
    case LaneFile(NegTestLane) => Stigi.Array.copy[U8](13, "negtest.facts", 0, 13, reportCap, report, at)
    case LaneFile(EdgeLane) => Stigi.Array.copy[U8](10, "edge.facts", 0, 10, reportCap, report, at)
    case ModeFile => Stigi.Array.copy[U8](15, "imageMode.facts", 0, 15, reportCap, report, at)
