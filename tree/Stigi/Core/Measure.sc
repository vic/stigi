// prompt: represent an optional loop termination measure in the Checked IR.
pub enum Stigi.Core.Measure:
  case Unmeasured
  case DecreasesExpr(exprAt: U64)
