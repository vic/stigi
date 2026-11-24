// prompt: turn one binder classification into a normalized linear length outcome in the pool.
def Stigi.Types.resolveBinderKind {Stigi.Mut} (poolCap: U64, pool: Array[Stigi.Types.Linear.Term](poolCap), poolEnd: U64, slot: U64, kind: Stigi.Types.BinderKind) <lessOrEqual(poolEnd, poolCap)>: Stigi.Types.LengthOutcome =
  kind match
    case ImmutableBinder =>
      if poolEnd < poolCap then
        pool(poolEnd) := LinearTerm(slot, WideInt(1, 0))
        LengthValue(LinearForm(WideInt(0, 0), poolEnd, 1), poolEnd + 1)
      else LengthPoolFull
    case MutableBinder => LengthNotLinear
    case KnownBinder(knownForm) => LengthValue(knownForm, poolEnd)
