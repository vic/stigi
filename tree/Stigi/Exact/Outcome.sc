// prompt: represent the result of a checked exact wide integer operation.
pub enum Stigi.Exact.Outcome:
  case ExactValue(value: Stigi.Exact.IntWide)
  case ExactOverflow
  case ExactDivideByZero
