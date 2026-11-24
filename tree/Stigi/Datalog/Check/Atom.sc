// prompt: represent a Datalog atom with relation index, bounded arity, and up to seven term slots.
pub enum Stigi.Datalog.Check.Atom:
  case AtomRef(relNum: U64, arity: U64, argZero: Stigi.Datalog.Check.Term, argOne: Stigi.Datalog.Check.Term, argTwo: Stigi.Datalog.Check.Term, argThree: Stigi.Datalog.Check.Term, argFour: Stigi.Datalog.Check.Term, argFive: Stigi.Datalog.Check.Term, argSix: Stigi.Datalog.Check.Term) <lessOrEqual(arity, 7)>
