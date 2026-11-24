// prompt: materialize a ground tuple row from an atom and current variable bindings.
def Stigi.Datalog.Engine.materializeAtom(varCap: U64, varVals: Array[Stigi.Datalog.Store.Cell](varCap), atom: Stigi.Datalog.Check.Atom) <below(0, varCap)>: Stigi.Datalog.Store.Tuple =
  atom match
    case AtomRef(relNum, arity, argZero, argOne, argTwo, argThree, argFour, argFive, argSix) =>
      val cellZero: Stigi.Datalog.Store.Cell = Stigi.Datalog.Engine.resolveTerm(varCap, varVals, argZero)
      val cellOne: Stigi.Datalog.Store.Cell = Stigi.Datalog.Engine.resolveTerm(varCap, varVals, argOne)
      val cellTwo: Stigi.Datalog.Store.Cell = Stigi.Datalog.Engine.resolveTerm(varCap, varVals, argTwo)
      val cellThree: Stigi.Datalog.Store.Cell = Stigi.Datalog.Engine.resolveTerm(varCap, varVals, argThree)
      val cellFour: Stigi.Datalog.Store.Cell = Stigi.Datalog.Engine.resolveTerm(varCap, varVals, argFour)
      val cellFive: Stigi.Datalog.Store.Cell = Stigi.Datalog.Engine.resolveTerm(varCap, varVals, argFive)
      val cellSix: Stigi.Datalog.Store.Cell = Stigi.Datalog.Engine.resolveTerm(varCap, varVals, argSix)
      StoreRow(relNum, arity, cellZero, cellOne, cellTwo, cellThree, cellFour, cellFive, cellSix)
