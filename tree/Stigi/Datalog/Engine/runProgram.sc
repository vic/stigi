// prompt: check safety, stratify, and run stratified semi-naive fixpoint evaluation on a Datalog program.
pub def Stigi.Datalog.Engine.runProgram {Stigi.Mut} (relCap: U64, rowCap: U64, tupleSlotCap: U64, ruleCap: U64, litCap: U64, varCap: U64, symCap: U64, relArities: Array[U64](relCap), relCount: U64, rules: Array[Stigi.Datalog.Check.RuleDef](ruleCap), ruleCount: U64, lits: Array[Stigi.Datalog.Check.Lit](litCap), litCount: U64, bound: Array[Bool](varCap), strata: Array[U64](relCap), rows: Array[Stigi.Datalog.Store.Tuple](rowCap), tupleSlots: Array[Stigi.Datalog.Store.Slot](tupleSlotCap), relHead: Array[Stigi.Datalog.Store.Slot](relCap), nextInRel: Array[Stigi.Datalog.Store.Slot](rowCap), varBound: Array[Bool](varCap), varVals: Array[Stigi.Datalog.Store.Cell](varCap), ranks: Array[U64](symCap), iterSlot: Array[Stigi.Datalog.Store.Slot](litCap), iterMask: Array[U64](litCap), initRows: U64, rowBudget: U64) <below(0, tupleSlotCap), below(0, varCap), below(0, symCap), lessOrEqual(relCount, relCap), lessOrEqual(ruleCount, ruleCap), lessOrEqual(litCount, litCap), lessOrEqual(initRows, rowCap)>: Stigi.Datalog.Engine.EvalResult =
  val checkOut: Stigi.Datalog.Check.Outcome = Stigi.Datalog.Check.stratifyProgram(relCap, relArities, relCount, ruleCap, rules, ruleCount, litCap, lits, litCount, varCap, bound, strata)
  checkOut match
    case UnresolvedRelation(ruleIdx, relNum) => EvalRefused(UnresolvedRelation(ruleIdx, relNum))
    case ArityMismatch(ruleIdx, relNum) => EvalRefused(ArityMismatch(ruleIdx, relNum))
    case RuleUnsafe(ruleIdx, varNum) => EvalRefused(RuleUnsafe(ruleIdx, varNum))
    case NegationCycle(relNum) => EvalRefused(NegationCycle(relNum))
    case CheckPassed(maxStratum) =>
      var curStratum: U64 = 0
      var curRowCount: U64 = initRows
      var curDerived: U64 = 0
      var stopped: Bool = false
      var finalRes: Stigi.Datalog.Engine.EvalResult = EvalDone(initRows, 0)
      while curStratum <= maxStratum && curStratum < relCap && !stopped do
        val nextStratum: U64 = curStratum + 1
        if curRowCount <= rowCap then
          val stratRes: Stigi.Datalog.Engine.EvalResult = Stigi.Datalog.Engine.runStratum(relCap, rowCap, tupleSlotCap, ruleCap, litCap, varCap, symCap, relCount, rules, ruleCount, lits, litCount, strata, rows, tupleSlots, relHead, nextInRel, varBound, varVals, ranks, iterSlot, iterMask, curStratum, curRowCount, curDerived, rowBudget)
          stratRes match
            case EvalDone(nextRows, nextDerived) =>
              curRowCount = nextRows
              curDerived = nextDerived
              finalRes = EvalDone(nextRows, nextDerived)
            case EvalRefused(reason) =>
              finalRes = EvalRefused(reason)
              stopped = true
            case EvalBudgetExceeded(derCount) =>
              finalRes = EvalBudgetExceeded(derCount)
              stopped = true
            case EvalTableFull =>
              finalRes = EvalTableFull
              stopped = true
            case EvalArithError =>
              finalRes = EvalArithError
              stopped = true
        else
          finalRes = EvalTableFull
          stopped = true
        curStratum = nextStratum
      finalRes
