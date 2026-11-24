// prompt: build a ternary Datalog atom for kernel rule encoding.
def Stigi.Datalog.Kernel.atomThree(relNum: U64, argZero: Stigi.Datalog.Check.Term, argOne: Stigi.Datalog.Check.Term, argTwo: Stigi.Datalog.Check.Term) <below(relNum, 16)>: Stigi.Datalog.Check.Atom =
  val padTerm: Stigi.Datalog.Check.Term = ConstSym(0)
  AtomRef(relNum, 3, argZero, argOne, argTwo, padTerm, padTerm, padTerm, padTerm)
