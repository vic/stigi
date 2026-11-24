// prompt: represent a Datalog rule with its head atom and body literal range.
pub enum Stigi.Datalog.Check.RuleDef:
  case RuleEntry(head: Stigi.Datalog.Check.Atom, litStart: U64, litEnd: U64) <lessOrEqual(litStart, litEnd)>
