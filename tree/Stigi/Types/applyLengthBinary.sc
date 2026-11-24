// prompt: apply one Checked-IR exact binary operator between two normalized linear forms.
def Stigi.Types.applyLengthBinary {Stigi.Mut} (poolCap: U64, pool: Array[Stigi.Types.Linear.Term](poolCap), arithKind: Stigi.Core.ArithKind, leftForm: Stigi.Types.Linear.Form, rightForm: Stigi.Types.Linear.Form, curPool: U64) <below(0, poolCap), lessOrEqual(curPool, poolCap)>: Stigi.Types.LengthOutcome =
  arithKind match
    case CoreAdd => Stigi.Types.mapLinearOutcome(Stigi.Types.Linear.combine(poolCap, pool, leftForm, rightForm, false, curPool))
    case CoreSub => Stigi.Types.mapLinearOutcome(Stigi.Types.Linear.combine(poolCap, pool, leftForm, rightForm, true, curPool))
    case CoreMul => Stigi.Types.mapLinearOutcome(Stigi.Types.Linear.multiply(poolCap, pool, leftForm, rightForm, curPool))
    case CoreDiv =>
      if curPool < 65536 then Stigi.Types.mapLinearOutcome(Stigi.Types.Linear.divideConst(leftForm, rightForm, false, curPool)) else LengthPoolFull
    case CoreRem =>
      if curPool < 65536 then Stigi.Types.mapLinearOutcome(Stigi.Types.Linear.divideConst(leftForm, rightForm, true, curPool)) else LengthPoolFull
