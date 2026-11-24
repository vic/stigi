// prompt: build a four-column Datalog atom for kernel rule encoding.
def Stigi.Datalog.Kernel.atomFour(relNum: U64, argZero: Stigi.Datalog.Check.Term, argOne: Stigi.Datalog.Check.Term, argTwo: Stigi.Datalog.Check.Term, argThree: Stigi.Datalog.Check.Term) <below(relNum, 16)>: Stigi.Datalog.Check.Atom =
  val padTerm: Stigi.Datalog.Check.Term = ConstSym(0)
  AtomRef(relNum, 4, argZero, argOne, argTwo, argThree, padTerm, padTerm, padTerm)
