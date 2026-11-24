// prompt: walk one parsed unit syntax tree from rootSyn, collecting lengthTerm, linearForm, sameLength, and termOffset rows.
def Stigi.Store.walkUnitLengths {region: Stigi.Region, Stigi.Mut} (synTables: Stigi.Syntax.Tables, scratch: Stigi.Store.LengthScratch, image: Stigi.Names.Image, unitIdx: U64, rootSyn: Stigi.Syntax.NodeAt, lanes: Stigi.Store.LengthLanes) <below(unitIdx, Stigi.Limit.maxUnits)>: Stigi.Store.LengthCollect =
  val Image(namePool, entries, count, _) = image
  if count <= unitIdx then LengthsStopped(FactsOutOfMemory) else
    val Entry(nameFirst, nameEnd, _, _) = entries(unitIdx)
    val unitSlice: Stigi.Store.NameSlice = NameSlice(namePool, nameFirst, nameEnd)
    scratch match
      case LengthScratch(_, _, _, _, _, work, _, _, _, _, _, _, _, _, _, _) =>
        work(0) := VisitNode(rootSyn)
        var state: Stigi.Store.LengthUnitState = LengthUnitState(scratch, 0, 0, 0, 1, lanes)
        var keepGoing: Bool = true
        var failed: Bool = false
        var fault: Stigi.Store.FactsFailure = FactsOutOfMemory
        while !failed && keepGoing do
          state match
            case LengthUnitState(curScratch, localCount, termCount, poolEnd, workTop, curLanes) =>
              if 0 < workTop then
                val nextTop: U64 = workTop - 1
                val popped: Stigi.Store.LengthUnitState = LengthUnitState(curScratch, localCount, termCount, poolEnd, nextTop, curLanes)
                work(nextTop) match
                  case RestoreBinders(mark) =>
                    val restoredLocals: U64 = if mark <= localCount then mark else localCount
                    state = LengthUnitState(curScratch, restoredLocals, termCount, poolEnd, nextTop, curLanes)
                  case VisitNode(node) =>
                    if node < Stigi.Limit.maxUnitNodes then
                      Stigi.Store.stepUnitNode{region}(synTables, image, unitIdx, popped, node) match
                        case StepUnitDone(nextState) => state = nextState
                        case StepUnitStopped(why) =>
                          failed = true
                          fault = why
                    else
                      failed = true
                      fault = FactsOutOfMemory
              else keepGoing = false
        if failed then LengthsStopped(fault) else
          state match
            case LengthUnitState(finalScratch, _, finalTerms, _, _, finalLanes) =>
              Stigi.Store.appendUnitPairs{region}(finalScratch, unitSlice, finalTerms, finalLanes)
