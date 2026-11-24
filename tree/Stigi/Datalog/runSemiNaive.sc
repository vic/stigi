// prompt: run hash-indexed semi-naive Datalog fixpoint over positive joins and stratified negation with row budget.
pub def Stigi.Datalog.runSemiNaive {region: Stigi.Region, Stigi.Mut} (rowCap: Stigi.NodeCap, rows: Array[Stigi.Datalog.TupleRow](rowCap), initCount: U64, ruleCap: Stigi.UnitCap, rules: Array[Stigi.Datalog.RuleSpec](ruleCap), ruleCount: U64, rejectRel: U64) <lessOrEqual(initCount, rowCap), below(0, rowCap), lessOrEqual(ruleCount, ruleCap)>: Stigi.Datalog.EngineResult =
  val Filled(slots) = Array.tryFill[Stigi.Datalog.Slot]{region}(rowCap, Empty) else
    case FillFailed => EngineOutOfMemory
  val seeded: U64 = Stigi.Datalog.seedRows(rowCap, rows, slots, Scan(0, initCount, 0), initCount)
  val Derived(positive) = Stigi.Datalog.fixpoint(rowCap, rows, slots, ruleCap, rules, ruleCount, Scan(0, seeded, seeded), 256) else
    case DeriveFull => EngineBudgetExceeded
  val Derived(totalRows) = Stigi.Datalog.applyRules(rowCap, rows, slots, ruleCap, rules, ruleCount, Negative, RulePass(0, 0, positive, positive), ruleCount) else
    case DeriveFull => EngineBudgetExceeded
  var rejectCount: U64 = 0
  var countIdx: U64 = 0
  while countIdx < totalRows && countIdx < rowCap do
    @<lessOrEqual(countIdx, totalRows), lessOrEqual(rejectCount, countIdx)>
    val TupleRow(rowRel, _, _, _) = rows(countIdx)
    if rowRel == rejectRel then rejectCount = rejectCount + 1 else ()
    countIdx = countIdx + 1
  EngineConverged(totalRows, rejectCount)
