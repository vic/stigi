// prompt: test linear length normalization, commutativity, cancellation, scaling, offsets, and rejections.
def Stigi.Types.Tests.testLinear {region: Stigi.Region, Stigi.Mut} (poolCap: U64) <below(15, poolCap)>: Bool =
  val padTerm: Stigi.Types.Linear.Term = LinearTerm(0, WideInt(0, 0))
  Array.tryFill{region}(poolCap, padTerm) match
    case FillFailed => false
    case Filled(pool) =>
      pool(0) := LinearTerm(1, WideInt(1, 0))
      pool(1) := LinearTerm(2, WideInt(1, 0))
      val formVarOne: Stigi.Types.Linear.Form = LinearForm(WideInt(0, 0), 0, 1)
      val formVarTwo: Stigi.Types.Linear.Form = LinearForm(WideInt(0, 0), 1, 1)
      val formOne: Stigi.Types.Linear.Form = LinearForm(WideInt(1, 0), 0, 0)
      val formTwo: Stigi.Types.Linear.Form = LinearForm(WideInt(2, 0), 0, 0)
      val formThree: Stigi.Types.Linear.Form = LinearForm(WideInt(3, 0), 0, 0)
      val formFive: Stigi.Types.Linear.Form = LinearForm(WideInt(5, 0), 0, 0)
      val formSix: Stigi.Types.Linear.Form = LinearForm(WideInt(6, 0), 0, 0)
      val formZero: Stigi.Types.Linear.Form = LinearForm(WideInt(0, 0), 0, 0)
      val chkComm: Bool = Stigi.Types.Linear.combine(poolCap, pool, formVarOne, formOne, false, 2) match
        case LinearVal(varPlusOne, nextOne) =>
          if nextOne <= poolCap then
            Stigi.Types.Linear.combine(poolCap, pool, formOne, formVarOne, false, nextOne) match
              case LinearVal(onePlusVar, _) => Stigi.Types.Linear.equalForms(poolCap, pool, varPlusOne, onePlusVar)
              case LinearNotLinear => false
              case LinearOverflow => false
              case LinearDivZero => false
              case LinearPoolFull => false
          else
            false
        case LinearNotLinear => false
        case LinearOverflow => false
        case LinearDivZero => false
        case LinearPoolFull => false
      val chkCancel: Bool = Stigi.Types.Linear.combine(poolCap, pool, formVarOne, formVarTwo, false, 4) match
        case LinearVal(varPlusOther, nextTwo) =>
          if nextTwo <= poolCap then
            Stigi.Types.Linear.combine(poolCap, pool, varPlusOther, formVarTwo, true, nextTwo) match
              case LinearVal(cancelled, _) => Stigi.Types.Linear.equalForms(poolCap, pool, cancelled, formVarOne)
              case LinearNotLinear => false
              case LinearOverflow => false
              case LinearDivZero => false
              case LinearPoolFull => false
          else
            false
        case LinearNotLinear => false
        case LinearOverflow => false
        case LinearDivZero => false
        case LinearPoolFull => false
      val chkScale: Bool = Stigi.Types.Linear.combine(poolCap, pool, formVarOne, formThree, false, 8) match
        case LinearVal(varPlusThree, nextThree) =>
          if nextThree <= poolCap then
            Stigi.Types.Linear.multiply(poolCap, pool, formTwo, varPlusThree, nextThree) match
              case LinearVal(lhsScaled, nextFour) =>
                if nextFour <= poolCap then
                  Stigi.Types.Linear.multiply(poolCap, pool, formTwo, formVarOne, nextFour) match
                    case LinearVal(twoVar, nextFive) =>
                      if nextFive <= poolCap then
                        Stigi.Types.Linear.combine(poolCap, pool, twoVar, formSix, false, nextFive) match
                          case LinearVal(rhsScaled, _) => Stigi.Types.Linear.equalForms(poolCap, pool, lhsScaled, rhsScaled)
                          case LinearNotLinear => false
                          case LinearOverflow => false
                          case LinearDivZero => false
                          case LinearPoolFull => false
                      else
                        false
                    case LinearNotLinear => false
                    case LinearOverflow => false
                    case LinearDivZero => false
                    case LinearPoolFull => false
                else
                  false
              case LinearNotLinear => false
              case LinearOverflow => false
              case LinearDivZero => false
              case LinearPoolFull => false
          else
            false
        case LinearNotLinear => false
        case LinearOverflow => false
        case LinearDivZero => false
        case LinearPoolFull => false
      val chkNonLin: Bool = Stigi.Types.Linear.multiply(poolCap, pool, formVarOne, formVarTwo, 2) match
        case LinearNotLinear => true
        case LinearVal(_, _) => false
        case LinearOverflow => false
        case LinearDivZero => false
        case LinearPoolFull => false
      val chkOffset: Bool = Stigi.Types.Linear.combine(poolCap, pool, formVarOne, formFive, false, 2) match
        case LinearVal(varPlusFive, nextSix) =>
          if nextSix <= poolCap then
            Stigi.Types.Linear.combine(poolCap, pool, formVarOne, formTwo, false, nextSix) match
              case LinearVal(varPlusTwo, _) =>
                Stigi.Types.Linear.offsetBetween(poolCap, pool, varPlusFive, varPlusTwo) match
                  case OffsetDelta(delta) => Stigi.Exact.compare(delta, WideInt(3, 0)) is EqualTo
                  case OffsetUnrelated => false
              case LinearNotLinear => false
              case LinearOverflow => false
              case LinearDivZero => false
              case LinearPoolFull => false
          else
            false
        case LinearNotLinear => false
        case LinearOverflow => false
        case LinearDivZero => false
        case LinearPoolFull => false
      val chkDivZero: Bool = Stigi.Types.Linear.divideConst(formFive, formZero, false, 2) match
        case LinearDivZero => true
        case LinearVal(_, _) => false
        case LinearNotLinear => false
        case LinearOverflow => false
        case LinearPoolFull => false
      chkComm && chkCancel && chkScale && chkNonLin && chkOffset && chkDivZero
