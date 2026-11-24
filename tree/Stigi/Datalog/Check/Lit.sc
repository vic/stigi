// prompt: represent a positive, negated, comparison, or arithmetic assignment literal in a Datalog rule body.
pub enum Stigi.Datalog.Check.Lit:
  case PosLit(atom: Stigi.Datalog.Check.Atom)
  case NegLit(atom: Stigi.Datalog.Check.Atom)
  case CmpLit(kind: Stigi.Datalog.Check.CmpKind, left: Stigi.Datalog.Check.Term, right: Stigi.Datalog.Check.Term)
  case ArithLit(kind: Stigi.Datalog.Check.ArithKind, dstVar: U64, left: Stigi.Datalog.Check.Term, right: Stigi.Datalog.Check.Term)
