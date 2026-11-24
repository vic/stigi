// prompt: collect lengthTerm, linearForm, sameLength, and termOffset rows across all non-NegTest units in the tree.
def Stigi.Store.collectAllLengths {region: Stigi.Region, Stigi.Mut} (target: Stigi.Store.TypesTarget, image: Stigi.Names.Image, synTables: Stigi.Syntax.Tables, laneSize: Stigi.EmitCap): Stigi.Store.LengthCollect =
  val LengthScratchReady(scratch) = Stigi.Store.allocateLengthScratch{region}(512, synTables) else
    case LengthScratchUnavailable => LengthsStopped(FactsOutOfMemory)
  val LengthLanesReady(initLanes) = Stigi.Store.allocateLengthLanes{region}(laneSize) else
    case LengthLanesUnavailable => LengthsStopped(FactsOutOfMemory)
  Stigi.Store.collectValConstants{region}(target, scratch, image)
  val Image(_, entries, unitCount, _) = image
  var current: Stigi.Store.LengthLanes = initLanes
  var failed: Bool = false
  var fault: Stigi.Store.FactsFailure = FactsOutOfMemory
  var unitIdx: U64 = 0
  while unitIdx < unitCount && !failed do
    @<lessOrEqual(unitIdx, unitCount)>
    val Entry(_, _, kind, _) = entries(unitIdx)
    if !(kind is NegTestKind) then
      Stigi.Store.loadUnitForLength{region}(target, scratch, image, unitIdx) match
        case ParseRejected(_, _) | LexRejected(_) => ()
        case Parsed(rootSyn, _, _) =>
          Stigi.Store.walkUnitLengths{region}(synTables, scratch, image, unitIdx, rootSyn, current) match
            case LengthsStopped(why) =>
              failed = true
              fault = why
            case LengthsCollected(nextLanes) => current = nextLanes
    else ()
    unitIdx = unitIdx + 1
  if failed then LengthsStopped(fault) else LengthsCollected(current)
