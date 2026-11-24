// prompt: decide whether two normalized linear length forms are equal.
pub def Stigi.Types.sameLength(poolCap: U64, pool: Array[Stigi.Types.Linear.Term](poolCap), leftForm: Stigi.Types.Linear.Form, rightForm: Stigi.Types.Linear.Form) <lessOrEqual(1, poolCap)>: Bool =
  Stigi.Types.Linear.equalForms(poolCap, pool, leftForm, rightForm)
