// prompt: run semi-naive fixpoint evaluation for all rules belonging to one stratum.
def Stigi.Datalog.Engine.runStratum {Stigi.Mut} (relCap: U64, rowCap: U64, tupleSlotCap: U64, ruleCap: U64, litCap: U64, varCap: U64, symCap: U64, relCount: U64, rules: Array[Stigi.Datalog.Check.RuleDef](ruleCap), ruleCount: U64, lits: Array[Stigi.Datalog.Check.Lit](litCap), litCount: U64, strata: Array[U64](relCap), rows: Array[Stigi.Datalog.Store.Tuple](rowCap), tupleSlots: Array[Stigi.Datalog.Store.Slot](tupleSlotCap), relHead: Array[Stigi.Datalog.Store.Slot](relCap), nextInRel: Array[Stigi.Datalog.Store.Slot](rowCap), varBound: Array[Bool](varCap), varVals: Array[Stigi.Datalog.Store.Cell](varCap), ranks: Array[U64](symCap), iterSlot: Array[Stigi.Datalog.Store.Slot](litCap), iterMask: Array[U64](litCap), curStratum: U64, startRows: U64, startDerived: U64, rowBudget: U64) <below(0, tupleSlotCap), below(0, varCap), below(0, symCap), lessOrEqual(relCount, relCap), lessOrEqual(ruleCount, ruleCap), lessOrEqual(litCount, litCap), lessOrEqual(startRows, rowCap)>: Stigi.Datalog.Engine.EvalResult =
  var curRowCount: U64 = startRows
  var curDerived: U64 = startDerived
  var deltaEnd: U64 = startRows
  var tableFull: Bool = false
  var arithErr: Bool = false
  var ruleIdx: U64 = 0
  while ruleIdx < ruleCount && !tableFull && !arithErr do
    @<lessOrEqual(ruleIdx, ruleCount)>
    val nextRule: U64 = ruleIdx + 1
    val curRule: Stigi.Datalog.Check.RuleDef = rules(ruleIdx)
    curRule match
      case RuleEntry(head, _, _) =>
        head match
          case AtomRef(headRel, _, _, _, _, _, _, _, _) =>
            if headRel < relCount && strata(headRel) == curStratum && curRowCount <= rowCap then
              Stigi.Datalog.Engine.evalRuleBody(relCap, rowCap, tupleSlotCap, litCap, varCap, symCap, lits, litCount, rows, tupleSlots, relHead, nextInRel, varBound, varVals, ranks, iterSlot, iterMask, curRule, curRowCount, curDerived, false, 0, 0, deltaEnd) match
                case StepDone(nextRows, nextDerived) =>
                  curRowCount = nextRows
                  curDerived = nextDerived
                case StepFull =>
                  tableFull = true
                case StepArithError =>
                  arithErr = true
            else
              ()
    ruleIdx = nextRule
  if arithErr then
    EvalArithError
  else if tableFull then
    EvalTableFull
  else if curDerived > rowBudget then
    EvalBudgetExceeded(curDerived)
  else
    var budgetHit: Bool = false
    while curRowCount > deltaEnd && !tableFull && !budgetHit && !arithErr do
      val nextDeltaStart: U64 = deltaEnd
      val nextDeltaEnd: U64 = curRowCount
      var semiIdx: U64 = 0
      while semiIdx < ruleCount && !tableFull && !arithErr do
        @<lessOrEqual(semiIdx, ruleCount)>
        val nextSemi: U64 = semiIdx + 1
        val curRule: Stigi.Datalog.Check.RuleDef = rules(semiIdx)
        curRule match
          case RuleEntry(head, litStart, litEnd) =>
            head match
              case AtomRef(headRel, _, _, _, _, _, _, _, _) =>
                if headRel < relCount && strata(headRel) == curStratum && litEnd <= litCount then
                  var deltaIdx: U64 = litStart
                  while deltaIdx < litEnd && !tableFull && !arithErr do
                    @<lessOrEqual(litStart, deltaIdx), lessOrEqual(deltaIdx, litEnd)>
                    val nextDelta: U64 = deltaIdx + 1
                    lits(deltaIdx) match
                      case PosLit(bodyAtom) =>
                        bodyAtom match
                          case AtomRef(bodyRel, _, _, _, _, _, _, _, _) =>
                            if bodyRel < relCount && strata(bodyRel) == curStratum && curRowCount <= rowCap then
                              Stigi.Datalog.Engine.evalRuleBody(relCap, rowCap, tupleSlotCap, litCap, varCap, symCap, lits, litCount, rows, tupleSlots, relHead, nextInRel, varBound, varVals, ranks, iterSlot, iterMask, curRule, curRowCount, curDerived, true, deltaIdx, nextDeltaStart, nextDeltaEnd) match
                                case StepDone(nextRows, nextDerived) =>
                                  curRowCount = nextRows
                                  curDerived = nextDerived
                                case StepFull =>
                                  tableFull = true
                                case StepArithError =>
                                  arithErr = true
                            else
                              ()
                      case NegLit(_) => ()
                      case CmpLit(_, _, _) => ()
                      case ArithLit(_, _, _, _) => ()
                    deltaIdx = nextDelta
                else
                  ()
        semiIdx = nextSemi
      if curDerived > rowBudget then
        budgetHit = true
      else
        ()
      deltaEnd = nextDeltaEnd
    if arithErr then
      EvalArithError
    else if tableFull then
      EvalTableFull
    else if budgetHit then
      EvalBudgetExceeded(curDerived)
    else
      EvalDone(curRowCount, curDerived)
