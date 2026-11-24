// prompt: compute the strictly positive constant offset between termForm and baseForm when termForm equals baseForm plus a positive U64 constant.
pub def Stigi.Types.termOffsetOf(poolCap: U64, pool: Array[Stigi.Types.Linear.Term](poolCap), termForm: Stigi.Types.Linear.Form, baseForm: Stigi.Types.Linear.Form) <lessOrEqual(1, poolCap)>: Stigi.Types.TermOffset =
  Stigi.Types.Linear.offsetBetween(poolCap, pool, termForm, baseForm) match
    case OffsetUnrelated => WithoutTermOffset
    case OffsetDelta(WideInt(lowWord, highWord)) =>
      if highWord == 0 && 0 < lowWord then TermOffsetPlus(lowWord) else WithoutTermOffset
