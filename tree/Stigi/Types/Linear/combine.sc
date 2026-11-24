// prompt: add or subtract two canonical linear length forms via sorted two-pointer merge.
pub def Stigi.Types.Linear.combine {Stigi.Mut} (poolCap: U64, pool: Array[Stigi.Types.Linear.Term](poolCap), leftForm: Stigi.Types.Linear.Form, rightForm: Stigi.Types.Linear.Form, negateRight: Bool, outStart: U64) <below(0, poolCap), lessOrEqual(outStart, poolCap)>: Stigi.Types.Linear.Outcome =
  leftForm match
    case LinearForm(leftConst, leftStart, leftCount) =>
      rightForm match
        case LinearForm(rightConst, rightStart, rightCount) =>
          val constOut: Stigi.Exact.Outcome = if negateRight then Stigi.Exact.subtract(leftConst, rightConst) else Stigi.Exact.add(leftConst, rightConst)
          constOut match
            case ExactOverflow => LinearOverflow
            case ExactDivideByZero => LinearDivZero
            case ExactValue(mergedConst) =>
              if leftStart < 65536 && leftCount < 65536 && rightStart < 65536 && rightCount < 65536 then
                val leftEnd: U64 = leftStart + leftCount
                val rightEnd: U64 = rightStart + rightCount
                if leftEnd <= poolCap && rightEnd <= poolCap then
                  var leftPos: U64 = leftStart
                  var rightPos: U64 = rightStart
                  var outPos: U64 = outStart
                  var overflowed: Bool = false
                  var poolFull: Bool = false
                  while (leftPos < leftEnd || rightPos < rightEnd) && !overflowed && !poolFull do
                    @<lessOrEqual(leftStart, leftPos), lessOrEqual(leftPos, leftEnd), lessOrEqual(rightStart, rightPos), lessOrEqual(rightPos, rightEnd), lessOrEqual(outStart, outPos), lessOrEqual(outPos, poolCap)>
                    val stepKind: U64 = if leftPos < leftEnd && rightPos < rightEnd then
                      pool(leftPos) match
                        case LinearTerm(leftVar, _) =>
                          pool(rightPos) match
                            case LinearTerm(rightVar, _) =>
                              if leftVar < rightVar then 0 else if rightVar < leftVar then 1 else 2
                    else if leftPos < leftEnd then
                      0
                    else
                      1
                    if stepKind == 0 && leftPos < leftEnd then
                      val nextLeft: U64 = leftPos + 1
                      val nextOut: U64 = Stigi.Types.Linear.appendTerm(poolCap, pool, outPos, pool(leftPos))
                      if nextOut == outPos then
                        poolFull = true
                      else
                        outPos = nextOut
                      leftPos = nextLeft
                    else if stepKind == 1 && rightPos < rightEnd then
                      val nextRight: U64 = rightPos + 1
                      val rightTerm: Stigi.Types.Linear.Term = pool(rightPos)
                      val nextOut: U64 = rightTerm match
                        case LinearTerm(rightVar, rightCoeff) =>
                          val signedCoeff: Stigi.Exact.Outcome = if negateRight then Stigi.Exact.subtract(WideInt(0, 0), rightCoeff) else ExactValue(rightCoeff)
                          signedCoeff match
                            case ExactValue(finalCoeff) =>
                              if !Stigi.Types.Linear.isZeroWide(finalCoeff) then
                                val writtenOut: U64 = Stigi.Types.Linear.appendTerm(poolCap, pool, outPos, LinearTerm(rightVar, finalCoeff))
                                if writtenOut == outPos then
                                  poolFull = true
                                else
                                  ()
                                writtenOut
                              else
                                outPos
                            case ExactOverflow =>
                              overflowed = true
                              outPos
                            case ExactDivideByZero =>
                              overflowed = true
                              outPos
                      if outPos <= nextOut && nextOut <= poolCap then outPos = nextOut else poolFull = true
                      rightPos = nextRight
                    else if leftPos < leftEnd && rightPos < rightEnd then
                      val nextLeft: U64 = leftPos + 1
                      val nextRight: U64 = rightPos + 1
                      val leftTerm: Stigi.Types.Linear.Term = pool(leftPos)
                      val rightTerm: Stigi.Types.Linear.Term = pool(rightPos)
                      val nextOut: U64 = leftTerm match
                        case LinearTerm(varNum, leftCoeff) =>
                          rightTerm match
                            case LinearTerm(_, rightCoeff) =>
                              val sumCoeff: Stigi.Exact.Outcome = if negateRight then Stigi.Exact.subtract(leftCoeff, rightCoeff) else Stigi.Exact.add(leftCoeff, rightCoeff)
                              sumCoeff match
                                case ExactValue(finalCoeff) =>
                                  if !Stigi.Types.Linear.isZeroWide(finalCoeff) then
                                    val writtenOut: U64 = Stigi.Types.Linear.appendTerm(poolCap, pool, outPos, LinearTerm(varNum, finalCoeff))
                                    if writtenOut == outPos then
                                      poolFull = true
                                    else
                                      ()
                                    writtenOut
                                  else
                                    outPos
                                case ExactOverflow =>
                                  overflowed = true
                                  outPos
                                case ExactDivideByZero =>
                                  overflowed = true
                                  outPos
                      if outPos <= nextOut && nextOut <= poolCap then outPos = nextOut else poolFull = true
                      leftPos = nextLeft
                      rightPos = nextRight
                    else
                      poolFull = true
                  if overflowed then
                    LinearOverflow
                  else if poolFull then
                    LinearPoolFull
                  else
                    val mergedCount: U64 = outPos - outStart
                    LinearVal(LinearForm(mergedConst, outStart, mergedCount), outPos)
                else
                  LinearPoolFull
              else
                LinearPoolFull
