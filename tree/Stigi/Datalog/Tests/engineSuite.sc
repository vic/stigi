// prompt: verify 128-bit exact arithmetic carry and stratified semi-naive Datalog rejection.
val Stigi.Datalog.Tests.engineSuite {Stigi.Test}: Stigi.Test.Result =
  Stigi.Datalog.Laws.tupleSlotRange(TupleRow(1, 2, 3, 4), 64)
  val maxWord: U64 = 0xffffffffffffffff
  val IntWide(sumLow, sumHigh) = Stigi.Datalog.wideAdd(IntWide(maxWord, 0), IntWide(1, 0), 100)
  val wideValid: Bool = sumLow == 0 && sumHigh == 1
  val engineValid: Bool =
    val Filled(rows) = Array.tryFill[Stigi.Datalog.TupleRow]{Stigi.Region}(64, TupleRow(0, 0, 0, 0)) else
      case FillFailed => false
    val Filled(rules) = Array.tryFill[Stigi.Datalog.RuleSpec]{Stigi.Region}(4, RuleSpec(0, 0, 0, JoinCopy)) else
      case FillFailed => false
    rows(0) := TupleRow(1, 10, 20, 0)
    rows(1) := TupleRow(2, 20, 30, 0)
    rows(2) := TupleRow(3, 10, 30, 0)
    rows(3) := TupleRow(3, 10, 99, 0)
    rules(0) := RuleSpec(1, 1, 2, JoinChain)
    rules(1) := RuleSpec(99, 3, 1, JoinAntiMatch)
    Stigi.Datalog.runSemiNaive{Stigi.Region}(64, rows, 4, 4, rules, 2, 99) match
      case EngineConverged(totalRows, rejectCount) => totalRows == 6 && rejectCount == 1
      case EngineBudgetExceeded => false
      case EngineOutOfMemory => false
  Stigi.Test.expect(wideValid && engineValid, true, "Datalog 128-bit carry and stratified anti-join must be exact")
