// prompt: represent one arm of a one-level Checked-IR decision node.
pub enum Stigi.Core.Arm:
  case CaseArm(caseIndex: U64, binders: Stigi.Core.Span, bodyAt: U64)
  case ValueArm(lowBound: I64, highBound: I64, bodyAt: U64) <lessOrEqual(lowBound, highBound)>
  case RestArm(bodyAt: U64)
