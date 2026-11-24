// prompt: mark a variable term as bound in the rule safety table when within capacity.
def Stigi.Datalog.Check.markTermBound {Stigi.Mut} (varCap: U64, bound: Array[Bool](varCap), term: Stigi.Datalog.Check.Term) <below(0, varCap)>: Unit =
  term match
    case VarTerm(varNum) =>
      if varNum < varCap then
        bound(varNum) := true
      else
        ()
    case ConstSym(_) => ()
    case ConstNum(_, _) => ()
