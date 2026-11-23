// prompt: single-case Datalog rule descriptor specifying head relation, body relations, and join shape.
pub enum Stigi.Datalog.RuleSpec:
  case RuleSpec(headRel: U64, leftRel: U64, rightRel: U64, shape: Stigi.Datalog.JoinShape)
