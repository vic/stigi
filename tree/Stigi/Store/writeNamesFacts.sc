// prompt: write the seven fact files of a names export into an output directory that must already exist, creating or truncating exactly those files, stopping at the first failure and writing no file when the collection of the lanes stops.
def Stigi.Store.writeNamesFacts {region: Stigi.Region, Stigi.Mut} (outCap: Stigi.ByteCap, outBuf: Array[U8](outCap), outStart: U64, outEnd: U64, laneSize: Stigi.EmitCap, image: Stigi.Names.Image, edgeTable: Stigi.Names.Edges) <lessOrEqual(outStart, outEnd), lessOrEqual(outEnd, outCap)>: Stigi.Store.FactsOutcome =
  val LanesReady(allocated) = Stigi.Store.allocateLanes{region}(laneSize) else
    case LanesUnavailable => FactsFailed(FactsOutOfMemory)
  val LanesFilled(lanes) = Stigi.Store.collectLanes(allocated, image, edgeTable) else
    case CollectStopped(fault) => FactsFailed(FactsStopped(fault))
  val FactsWritten = Stigi.Store.writeLaneFile{region}(outCap, outBuf, outStart, outEnd, lanes, UnitLane) else
    case FactsFailed(failure) => FactsFailed(failure)
  val FactsWritten = Stigi.Store.writeLaneFile{region}(outCap, outBuf, outStart, outEnd, lanes, NameLane) else
    case FactsFailed(failure) => FactsFailed(failure)
  val FactsWritten = Stigi.Store.writeLaneFile{region}(outCap, outBuf, outStart, outEnd, lanes, StigiLane) else
    case FactsFailed(failure) => FactsFailed(failure)
  val FactsWritten = Stigi.Store.writeLaneFile{region}(outCap, outBuf, outStart, outEnd, lanes, HandlerLane) else
    case FactsFailed(failure) => FactsFailed(failure)
  val FactsWritten = Stigi.Store.writeLaneFile{region}(outCap, outBuf, outStart, outEnd, lanes, NegTestLane) else
    case FactsFailed(failure) => FactsFailed(failure)
  val FactsWritten = Stigi.Store.writeLaneFile{region}(outCap, outBuf, outStart, outEnd, lanes, EdgeLane) else
    case FactsFailed(failure) => FactsFailed(failure)
  Stigi.Store.writeModeFile{region}(outCap, outBuf, outStart, outEnd)
