// prompt: multiply two canonical linear length forms, rejecting nonlinear variable products.
pub def Stigi.Types.Linear.multiply {Stigi.Mut} (poolCap: U64, pool: Array[Stigi.Types.Linear.Term](poolCap), leftForm: Stigi.Types.Linear.Form, rightForm: Stigi.Types.Linear.Form, outStart: U64) <below(0, poolCap), lessOrEqual(outStart, poolCap)>: Stigi.Types.Linear.Outcome =
  leftForm match
    case LinearForm(leftConst, leftStart, leftCount) =>
      rightForm match
        case LinearForm(rightConst, rightStart, rightCount) =>
          if leftCount > 0 && rightCount > 0 then
            LinearNotLinear
          else
            val baseConst: Stigi.Exact.IntWide = if leftCount > 0 then leftConst else rightConst
            val baseStart: U64 = if leftCount > 0 then leftStart else rightStart
            val baseCount: U64 = if leftCount > 0 then leftCount else rightCount
            val scalar: Stigi.Exact.IntWide = if leftCount > 0 then rightConst else leftConst
            Stigi.Exact.multiply(baseConst, scalar) match
              case ExactOverflow => LinearOverflow
              case ExactDivideByZero => LinearDivZero
              case ExactValue(newConst) =>
                if Stigi.Types.Linear.isZeroWide(scalar) || baseCount == 0 then
                  LinearVal(LinearForm(newConst, outStart, 0), outStart)
                else if baseStart < 65536 && baseCount < 65536 then
                  val baseEnd: U64 = baseStart + baseCount
                  if baseEnd <= poolCap then
                    var srcPos: U64 = baseStart
                    var outPos: U64 = outStart
                    var overflowed: Bool = false
                    var poolFull: Bool = false
                    while srcPos < baseEnd && !overflowed && !poolFull do
                      @<lessOrEqual(baseStart, srcPos), lessOrEqual(srcPos, baseEnd), lessOrEqual(outStart, outPos), lessOrEqual(outPos, poolCap)>
                      val nextSrc: U64 = srcPos + 1
                      val nextOut: U64 = pool(srcPos) match
                        case LinearTerm(varNum, coeff) =>
                          Stigi.Exact.multiply(coeff, scalar) match
                            case ExactOverflow =>
                              overflowed = true
                              outPos
                            case ExactDivideByZero =>
                              overflowed = true
                              outPos
                            case ExactValue(scaledCoeff) =>
                              val writtenOut: U64 = Stigi.Types.Linear.appendTerm(poolCap, pool, outPos, LinearTerm(varNum, scaledCoeff))
                              if writtenOut == outPos then
                                poolFull = true
                              else
                                ()
                              writtenOut
                      if outPos <= nextOut && nextOut <= poolCap then outPos = nextOut else poolFull = true
                      srcPos = nextSrc
                    if overflowed then
                      LinearOverflow
                    else if poolFull then
                      LinearPoolFull
                    else
                      val scaledCount: U64 = outPos - outStart
                      LinearVal(LinearForm(newConst, outStart, scaledCount), outPos)
                  else
                    LinearPoolFull
                else
                  LinearPoolFull
