// prompt: run semi-naive binary transitive closure and optional stratified negation filter with row budget enforcement.
pub def Stigi.Datalog.run {region: Stigi.Region, Stigi.Mut} (inputCount: U64, inputLeft: Array[U64](inputCount), inputRight: Array[U64](inputCount), blockedCount: U64, blockedLeft: Array[U64](blockedCount), blockedRight: Array[U64](blockedCount), budgetLimit: U64) <below(inputCount, 4096), below(blockedCount, 4096), below(0, budgetLimit), lessOrEqual(budgetLimit, Stigi.Datalog.rowBudget)>: Stigi.Datalog.RunOutcome =
  val tableCap: U64 = if budgetLimit < 4096 then budgetLimit else 4096
  val slotCount: U64 = 8192
  val zeroByte: U8 = 0
  val zeroWord: U64 = 0
  Array.tryFill{region}(slotCount, zeroByte) match
    case FillFailed => EngineRejected(ReasonOutOfMemory)
    case Filled(usedSlots) =>
      Array.tryFill{region}(slotCount, zeroWord) match
        case FillFailed => EngineRejected(ReasonOutOfMemory)
        case Filled(slotRows) =>
          Array.tryFill{region}(tableCap, zeroWord) match
            case FillFailed => EngineRejected(ReasonOutOfMemory)
            case Filled(leftCol) =>
              Array.tryFill{region}(tableCap, zeroWord) match
                case FillFailed => EngineRejected(ReasonOutOfMemory)
                case Filled(rightCol) =>
                  Array.tryFill{region}(tableCap, zeroWord) match
                    case FillFailed => EngineRejected(ReasonOutOfMemory)
                    case Filled(ruleCol) =>
                      Array.tryFill{region}(tableCap, zeroWord) match
                        case FillFailed => EngineRejected(ReasonOutOfMemory)
                        case Filled(roundCol) =>
                          var rowCount: U64 = 0
                          var probes: U64 = 0
                          var budgetHit: U64 = 0
                          var seedIndex: U64 = 0
                          while seedIndex < inputCount do
                            @<lessOrEqual(seedIndex, inputCount), lessOrEqual(rowCount, tableCap), lessOrEqual(budgetHit, 1), decreases(inputCount - seedIndex)>
                            if rowCount < tableCap then
                              val nextCount: U64 = Stigi.Datalog.tableInsertPair(slotCount, tableCap, rowCount, usedSlots, slotRows, leftCol, rightCol, ruleCol, roundCol, inputLeft(seedIndex), inputRight(seedIndex), 0, 0)
                              rowCount = nextCount
                              probes = U64.wrappingAdd(probes, 1)
                            else
                              budgetHit = 1
                            seedIndex = seedIndex + 1
                          var deltaStart: U64 = 0
                          var roundCount: U64 = 0
                          var maxRounds: U64 = tableCap
                          while deltaStart < rowCount && roundCount < maxRounds && budgetHit == 0 do
                            @<lessOrEqual(deltaStart, rowCount), lessOrEqual(rowCount, tableCap), lessOrEqual(roundCount, maxRounds), lessOrEqual(budgetHit, 1), decreases(maxRounds - roundCount)>
                            val deltaEnd: U64 = rowCount
                            var deltaIndex: U64 = deltaStart
                            while deltaIndex < deltaEnd && budgetHit == 0 do
                              @<lessOrEqual(deltaIndex, deltaEnd), lessOrEqual(deltaEnd, rowCount), lessOrEqual(rowCount, tableCap), lessOrEqual(budgetHit, 1), decreases(deltaEnd - deltaIndex)>
                              val midLeft: U64 = leftCol(deltaIndex)
                              val midJoint: U64 = rightCol(deltaIndex)
                              var edgeIndex: U64 = 0
                              while edgeIndex < inputCount && budgetHit == 0 do
                                @<lessOrEqual(edgeIndex, inputCount), lessOrEqual(deltaEnd, rowCount), lessOrEqual(rowCount, tableCap), lessOrEqual(budgetHit, 1), decreases(inputCount - edgeIndex)>
                                if inputLeft(edgeIndex) == midJoint then
                                  val targetRight: U64 = inputRight(edgeIndex)
                                  val already: U64 = Stigi.Datalog.tableProbePair(slotCount, tableCap, usedSlots, slotRows, leftCol, rightCol, midLeft, targetRight)
                                  probes = U64.wrappingAdd(probes, 1)
                                  if already == 0 then
                                    if rowCount < tableCap then
                                      val grownCount: U64 = Stigi.Datalog.tableInsertPair(slotCount, tableCap, rowCount, usedSlots, slotRows, leftCol, rightCol, ruleCol, roundCol, midLeft, targetRight, 1, roundCount + 1)
                                      rowCount = grownCount
                                    else
                                      budgetHit = 1
                                  else
                                    ()
                                else
                                  ()
                                edgeIndex = edgeIndex + 1
                              deltaIndex = deltaIndex + 1
                            deltaStart = deltaEnd
                            roundCount = roundCount + 1
                          if budgetHit == 1 then
                            EngineRejected(ReasonBudgetExceeded)
                          else
                            var unblockedRows: U64 = 0
                            var checkIndex: U64 = 0
                            while checkIndex < rowCount do
                              @<lessOrEqual(checkIndex, rowCount), lessOrEqual(rowCount, tableCap), lessOrEqual(unblockedRows, checkIndex), decreases(rowCount - checkIndex)>
                              val curLeft: U64 = leftCol(checkIndex)
                              val curRight: U64 = rightCol(checkIndex)
                              var isBlocked: U64 = 0
                              var blkIndex: U64 = 0
                              while blkIndex < blockedCount do
                                @<lessOrEqual(blkIndex, blockedCount), lessOrEqual(isBlocked, 1), decreases(blockedCount - blkIndex)>
                                if blockedLeft(blkIndex) == curLeft && blockedRight(blkIndex) == curRight then
                                  isBlocked = 1
                                else
                                  ()
                                blkIndex = blkIndex + 1
                              if isBlocked == 0 then
                                unblockedRows = unblockedRows + 1
                              else
                                ()
                              checkIndex = checkIndex + 1
                            EngineComplete(unblockedRows, roundCount, probes, 0)
