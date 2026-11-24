// prompt: test structural canonical equality of two normalized linear length forms.
pub def Stigi.Types.Linear.equalForms(poolCap: U64, pool: Array[Stigi.Types.Linear.Term](poolCap), leftForm: Stigi.Types.Linear.Form, rightForm: Stigi.Types.Linear.Form) <below(0, poolCap)>: Bool =
  leftForm match
    case LinearForm(leftConst, leftStart, leftCount) =>
      rightForm match
        case LinearForm(rightConst, rightStart, rightCount) =>
          val sameConst: Bool = Stigi.Exact.compare(leftConst, rightConst) is EqualTo
          if !sameConst || leftCount != rightCount then
            false
          else if leftStart < 65536 && rightStart < 65536 && leftCount < 65536 then
            val leftEnd: U64 = leftStart + leftCount
            val rightEnd: U64 = rightStart + leftCount
            if leftEnd <= poolCap && rightEnd <= poolCap then
              var idx: U64 = 0
              var same: Bool = true
              while idx < leftCount && same do
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
                            same = false
                          else
                            ()
                else
                  same = false
                idx = nextIdx
              same
            else
              false
          else
            false
