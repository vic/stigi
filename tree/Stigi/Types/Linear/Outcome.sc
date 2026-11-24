// prompt: represent the outcome of normalizing or combining linear length expressions.
pub enum Stigi.Types.Linear.Outcome:
  case LinearVal(form: Stigi.Types.Linear.Form, nextPool: U64)
  case LinearNotLinear
  case LinearOverflow
  case LinearDivZero
  case LinearPoolFull
