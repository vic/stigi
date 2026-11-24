// prompt: build a binary Datalog atom for kernel rule encoding.
def Stigi.Datalog.Kernel.atomTwo(relNum: U64, argZero: Stigi.Datalog.Check.Term, argOne: Stigi.Datalog.Check.Term) <below(relNum, 16)>: Stigi.Datalog.Check.Atom =
  val padTerm: Stigi.Datalog.Check.Term = ConstSym(0)
  AtomRef(relNum, 2, argZero, argOne, padTerm, padTerm, padTerm, padTerm, padTerm)
