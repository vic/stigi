// prompt: construct a unary Datalog atom for engine tests.
def Stigi.Datalog.Engine.Tests.makeAtomOne(relNum: U64, argZero: Stigi.Datalog.Check.Term) <below(relNum, 16)>: Stigi.Datalog.Check.Atom =
  val padTerm: Stigi.Datalog.Check.Term = ConstSym(0)
  AtomRef(relNum, 1, argZero, padTerm, padTerm, padTerm, padTerm, padTerm, padTerm)
