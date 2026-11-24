// prompt: resolve a Datalog term to a ground cell using current variable bindings.
def Stigi.Datalog.Engine.resolveTerm(varCap: U64, varVals: Array[Stigi.Datalog.Store.Cell](varCap), term: Stigi.Datalog.Check.Term) <below(0, varCap)>: Stigi.Datalog.Store.Cell =
  term match
    case VarTerm(varNum) =>
      if varNum < varCap then
        varVals(varNum)
      else
        SymCell(0)
    case ConstSym(symNum) => SymCell(symNum)
    case ConstNum(lowWord, highWord) => NumCell(WideInt(lowWord, highWord))
