// prompt: divide or take remainder of two constant linear length forms, rejecting variable operands.
pub def Stigi.Types.Linear.divideConst(leftForm: Stigi.Types.Linear.Form, rightForm: Stigi.Types.Linear.Form, wantRemainder: Bool, outStart: U64) <below(outStart, 65536)>: Stigi.Types.Linear.Outcome =
  leftForm match
    case LinearForm(leftConst, _, leftCount) =>
      rightForm match
        case LinearForm(rightConst, _, rightCount) =>
          if leftCount > 0 || rightCount > 0 then
            LinearNotLinear
          else
            Stigi.Exact.quotientRemainder(leftConst, rightConst) match
              case DivModDivideByZero => LinearDivZero
              case DivModOverflow => LinearOverflow
              case DivModValue(quotVal, remVal) =>
                val chosen: Stigi.Exact.IntWide = if wantRemainder then remVal else quotVal
                LinearVal(LinearForm(chosen, outStart, 0), outStart)
