// prompt: represent a term in a Datalog atom or comparison literal.
pub enum Stigi.Datalog.Check.Term:
  case VarTerm(varNum: U64)
  case ConstSym(symNum: U64)
  case ConstNum(lowWord: U64, highWord: I64)
