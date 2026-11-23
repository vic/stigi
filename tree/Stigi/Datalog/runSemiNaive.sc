// prompt: run hash-indexed semi-naive Datalog fixpoint over positive joins and stratified negation with row budget.
pub def Stigi.Datalog.runSemiNaive {region: Stigi.Region, Stigi.Mut} (rowCap: Stigi.NodeCap, rows: Array[Stigi.Datalog.TupleRow](rowCap), initCount: U64, ruleCap: Stigi.UnitCap, rules: Array[Stigi.Datalog.RuleSpec](ruleCap), ruleCount: U64, rejectRel: U64) <lessOrEqual(initCount, rowCap), below(0, rowCap), lessOrEqual(ruleCount, ruleCap)>: Stigi.Datalog.EngineResult =
  val hashCap: Stigi.NodeCap = rowCap
  val Filled(bucketUsed) = Array.tryFill[Bool]{region}(hashCap, false) else
    case FillFailed => EngineOutOfMemory
  val Filled(bucketRow) = Array.tryFill[U64]{region}(hashCap, 0) else
    case FillFailed => EngineOutOfMemory
  var totalRows: U64 = 0
  var seedIdx: U64 = 0
  while seedIdx < initCount && totalRows < rowCap do
    @<lessOrEqual(seedIdx, initCount), lessOrEqual(totalRows, rowCap)>
    val seedRow: Stigi.Datalog.TupleRow = rows(seedIdx)
    if Stigi.Datalog.insertUnique(rowCap, rows, totalRows, hashCap, bucketUsed, bucketRow, seedRow) then
      totalRows = totalRows + 1
    else ()
    seedIdx = seedIdx + 1
  var deltaStart: U64 = 0
  var deltaEnd: U64 = totalRows
  var budgetHit: Bool = false
  var iterGuard: U64 = 0
  while deltaStart < deltaEnd && !budgetHit && iterGuard < 256 do
    @<lessOrEqual(deltaStart, deltaEnd), lessOrEqual(deltaEnd, totalRows), lessOrEqual(totalRows, rowCap), lessOrEqual(iterGuard, 256)>
    var ruleIdx: U64 = 0
    while ruleIdx < ruleCount && !budgetHit do
      @<lessOrEqual(ruleIdx, ruleCount), lessOrEqual(deltaEnd, totalRows), lessOrEqual(totalRows, rowCap)>
      val RuleSpec(headRel, leftRel, rightRel, shape) = rules(ruleIdx)
      shape match
        case JoinCopy =>
          var leftIdx: U64 = deltaStart
          while leftIdx < deltaEnd && !budgetHit do
            @<lessOrEqual(leftIdx, deltaEnd), lessOrEqual(deltaEnd, totalRows), lessOrEqual(totalRows, rowCap)>
            val TupleRow(rowRel, colZero, colOne, colTwo) = rows(leftIdx)
            if rowRel == leftRel then
              if totalRows < rowCap then
                if Stigi.Datalog.insertUnique(rowCap, rows, totalRows, hashCap, bucketUsed, bucketRow, TupleRow(headRel, colZero, colOne, colTwo)) then
                  totalRows = totalRows + 1
                else ()
              else
                budgetHit = true
            else ()
            leftIdx = leftIdx + 1
        case JoinChain =>
          var leftIdx: U64 = deltaStart
          while leftIdx < deltaEnd && !budgetHit do
            @<lessOrEqual(leftIdx, deltaEnd), lessOrEqual(deltaEnd, totalRows), lessOrEqual(totalRows, rowCap)>
            val TupleRow(leftRowRel, leftZero, leftOne, leftTwo) = rows(leftIdx)
            if leftRowRel == leftRel then
              var rightIdx: U64 = 0
              while rightIdx < deltaEnd && !budgetHit do
                @<lessOrEqual(rightIdx, deltaEnd), lessOrEqual(deltaEnd, totalRows), lessOrEqual(totalRows, rowCap)>
                val TupleRow(rightRowRel, rightZero, rightOne, rightTwo) = rows(rightIdx)
                if rightRowRel == rightRel && leftOne == rightZero && leftTwo == rightTwo then
                  if totalRows < rowCap then
                    if Stigi.Datalog.insertUnique(rowCap, rows, totalRows, hashCap, bucketUsed, bucketRow, TupleRow(headRel, leftZero, rightOne, leftTwo)) then
                      totalRows = totalRows + 1
                    else ()
                  else
                    budgetHit = true
                else ()
                rightIdx = rightIdx + 1
            else ()
            leftIdx = leftIdx + 1
        case JoinAntiMatch => ()
      ruleIdx = ruleIdx + 1
    deltaStart = deltaEnd
    deltaEnd = totalRows
    iterGuard = iterGuard + 1
  if budgetHit then EngineBudgetExceeded else
    val posRows: U64 = totalRows
    var antiRuleIdx: U64 = 0
    while antiRuleIdx < ruleCount && !budgetHit do
      @<lessOrEqual(antiRuleIdx, ruleCount), lessOrEqual(totalRows, rowCap)>
      val RuleSpec(headRel, leftRel, rightRel, shape) = rules(antiRuleIdx)
      shape match
        case JoinAntiMatch =>
          var scanIdx: U64 = 0
          while scanIdx < posRows && !budgetHit do
            @<lessOrEqual(scanIdx, posRows), lessOrEqual(totalRows, rowCap)>
            val TupleRow(rowRel, colZero, colOne, colTwo) = rows(scanIdx)
            if rowRel == leftRel then
              val clearedProbe: Stigi.Datalog.TupleRow = TupleRow(rightRel, colZero, colOne, colTwo)
              if !Stigi.Datalog.containsRow(rowCap, rows, posRows, hashCap, bucketUsed, bucketRow, clearedProbe) then
                if totalRows < rowCap then
                  if Stigi.Datalog.insertUnique(rowCap, rows, totalRows, hashCap, bucketUsed, bucketRow, TupleRow(headRel, colZero, colOne, colTwo)) then
                    totalRows = totalRows + 1
                  else ()
                else
                  budgetHit = true
              else ()
            else ()
            scanIdx = scanIdx + 1
        case JoinCopy => ()
        case JoinChain => ()
      antiRuleIdx = antiRuleIdx + 1
    if budgetHit then EngineBudgetExceeded else
      var rejectCount: U64 = 0
      var countIdx: U64 = 0
      while countIdx < totalRows do
        @<lessOrEqual(countIdx, totalRows), lessOrEqual(rejectCount, countIdx)>
        val TupleRow(rowRel, _, _, _) = rows(countIdx)
        if rowRel == rejectRel then rejectCount = rejectCount + 1 else ()
        countIdx = countIdx + 1
      EngineConverged(totalRows, rejectCount)
