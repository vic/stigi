// prompt: represent the outcome of lowering one probe source for the elaboration tests.
enum Stigi.Elaborate.Tests.Probe:
  case ProbeFillFailed
  case ProbeUnparsed
  case ProbeOverflow
  case ProbeInvalid
  case ProbeLowered(exprCount: U64, armCount: U64, firstExpr: Stigi.Core.Expr, firstArm: Stigi.Core.Arm, secondArm: Stigi.Core.Arm)
