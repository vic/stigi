// prompt: check that a term is either a constant or a variable bound by a positive or arithmetic body literal.
def Stigi.Datalog.Check.checkTermBound(varCap: U64, bound: Array[Bool](varCap), ruleIndex: U64, term: Stigi.Datalog.Check.Term) <below(0, varCap)>: Stigi.Datalog.Check.Outcome =
  term match
    case VarTerm(varNum) =>
      if varNum < varCap then
        if bound(varNum) then
          CheckPassed(0)
        else
          RuleUnsafe(ruleIndex, varNum)
      else
        RuleUnsafe(ruleIndex, varNum)
    case ConstSym(_) => CheckPassed(0)
    case ConstNum(_, _) => CheckPassed(0)
