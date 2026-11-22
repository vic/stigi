// prompt: outcome of running one fuel-bounded compile-time program over checked IR instructions.
pub enum Stigi.Eval.EvalOutcome:
  case EvalValue(word: U64, fuelUsed: U64)
  case EvalOutOfFuel
  case EvalNotCompileTime
  case EvalArithmeticError
