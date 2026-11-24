// prompt: represent one variable and exact signed coefficient term in a linear length expression.
pub enum Stigi.Types.Linear.Term:
  case LinearTerm(varNum: U64, coeff: Stigi.Exact.IntWide)
