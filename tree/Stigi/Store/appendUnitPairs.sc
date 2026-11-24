// prompt: append sameLength and strictly oriented termOffset rows for all valid pairs of length terms in one unit.
def Stigi.Store.appendUnitPairs {region: Stigi.Region, Stigi.Mut} (scratch: Stigi.Store.LengthScratch, unitSlice: Stigi.Store.NameSlice, termCount: U64, lanes: Stigi.Store.LengthLanes) <lessOrEqual(termCount, 256)>: Stigi.Store.LengthCollect =
  scratch match
    case LengthScratch(_, _, _, _, _, _, pool, _, _, _, _, unitForms, unitValid, _, _, _) =>
      var current: Stigi.Store.LengthLanes = lanes
      var failed: Bool = false
      var fault: Stigi.Store.FactsFailure = FactsOutOfMemory
      var leftIdx: U64 = 0
      while leftIdx < termCount && !failed do
        @<lessOrEqual(leftIdx, termCount)>
        if unitValid(leftIdx) then
          val leftForm: Stigi.Types.Linear.Form = unitForms(leftIdx)
          var rightIdx: U64 = 0
          while rightIdx < termCount && !failed do
            @<lessOrEqual(rightIdx, termCount)>
            if leftIdx != rightIdx && unitValid(rightIdx) then
              val rightForm: Stigi.Types.Linear.Form = unitForms(rightIdx)
              if Stigi.Types.sameLength(1024, pool, leftForm, rightForm) then
                current match
                  case LengthLanes(lengthLane, linearLane, offsetLane, sameLane, rowCount) =>
                    if Stigi.Limit.maxEmitBytes <= rowCount then
                      failed = true
                      fault = FactsStopped(TypeLaneFull(SameLengthFile))
                    else
                      Stigi.Store.putSameLengthLane{region}(sameLane, unitSlice, LengthPair(leftIdx, rightIdx)) match
                        case LaneWritten(nextSame) => current = LengthLanes(lengthLane, linearLane, offsetLane, nextSame, rowCount + 1)
                        case LaneOverflow =>
                          failed = true
                          fault = FactsStopped(TypeLaneFull(SameLengthFile))
              else
                Stigi.Types.termOffsetOf(1024, pool, leftForm, rightForm) match
                  case WithoutTermOffset => ()
                  case TermOffsetPlus(constant) =>
                    current match
                      case LengthLanes(lengthLane, linearLane, offsetLane, sameLane, rowCount) =>
                        if Stigi.Limit.maxEmitBytes <= rowCount then
                          failed = true
                          fault = FactsStopped(TypeLaneFull(TermOffsetFile))
                        else
                          Stigi.Store.putTermOffsetLane{region}(offsetLane, unitSlice, LengthPair(leftIdx, rightIdx), constant) match
                            case LaneWritten(nextOff) => current = LengthLanes(lengthLane, linearLane, nextOff, sameLane, rowCount + 1)
                            case LaneOverflow =>
                              failed = true
                              fault = FactsStopped(TypeLaneFull(TermOffsetFile))
            else ()
            rightIdx = rightIdx + 1
        else ()
        leftIdx = leftIdx + 1
      if failed then LengthsStopped(fault) else LengthsCollected(current)
