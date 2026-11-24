// prompt: working pool, binder tables, and evaluation stacks for normalizing Checked-IR length expressions into canonical linear forms.
pub enum Stigi.Types.LengthContext:
  case LengthSpace(poolCapacity: U64, pool: Array[Stigi.Types.Linear.Term](poolCapacity), poolEnd: U64, binderCapacity: U64, binders: Array[Stigi.Types.BinderKind](binderCapacity), globalCapacity: U64, globals: Array[Stigi.Types.BinderKind](globalCapacity), nodeStack: Array[U64](64), stepStack: Array[U64](64), formStack: Array[Stigi.Types.Linear.Form](64)) <lessOrEqual(1, poolCapacity), lessOrEqual(poolEnd, poolCapacity)>
