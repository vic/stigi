// prompt: compute the exact constant offset between two linear length forms sharing identical variable terms.
pub def Stigi.Types.Linear.offsetBetween(poolCap: U64, pool: Array[Stigi.Types.Linear.Term](poolCap), leftForm: Stigi.Types.Linear.Form, rightForm: Stigi.Types.Linear.Form) <below(0, poolCap)>: Stigi.Types.Linear.OffsetResult =
  leftForm match
    case LinearForm(leftConst, leftStart, leftCount) =>
      rightForm match
        case LinearForm(rightConst, rightStart, rightCount) =>
          if leftCount != rightCount then
            OffsetUnrelated
          else if leftStart < 65536 && rightStart < 65536 && leftCount < 65536 then
            val leftEnd: U64 = leftStart + leftCount
            val rightEnd: U64 = rightStart + leftCount
            if leftEnd <= poolCap && rightEnd <= poolCap then
              var idx: U64 = 0
              var sameTerms: Bool = true
              while idx < leftCount && sameTerms do
                @<lessOrEqual(idx, leftCount)>
                val nextIdx: U64 = idx + 1
                val leftPos: U64 = leftStart + idx
                val rightPos: U64 = rightStart + idx
                if leftPos < poolCap && rightPos < poolCap then
                  pool(leftPos) match
                    case LinearTerm(leftVar, leftCoeff) =>
                      pool(rightPos) match
                        case LinearTerm(rightVar, rightCoeff) =>
                          val sameCoeff: Bool = Stigi.Exact.compare(leftCoeff, rightCoeff) is EqualTo
                          if leftVar != rightVar || !sameCoeff then
                            sameTerms = false
                          else
                            ()
                else
                  sameTerms = false
                idx = nextIdx
              if !sameTerms then
                OffsetUnrelated
              else
                Stigi.Exact.subtract(leftConst, rightConst) match
                  case ExactValue(delta) => OffsetDelta(delta)
                  case ExactOverflow => OffsetUnrelated
                  case ExactDivideByZero => OffsetUnrelated
            else
              OffsetUnrelated
          else
            OffsetUnrelated
