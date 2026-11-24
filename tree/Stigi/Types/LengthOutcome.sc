// prompt: represent the outcome of normalizing a Checked-IR length expression into an unsigned linear form.
pub enum Stigi.Types.LengthOutcome:
  case LengthValue(form: Stigi.Types.Linear.Form, nextPool: U64)
  case LengthNotLinear
  case LengthOverflow
  case LengthDivZero
  case LengthPoolFull
  case LengthNotUnsigned
