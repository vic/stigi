// prompt: public mirror of Stigi.Syntax.BinaryOperator avoiding unqualified case collisions with Stigi.Exact.Ordering.
pub enum Stigi.Syntax.OperKind:
  case OperPlus
  case OperMinus
  case OperTimes
  case OperDivide
  case OperRem
  case OperLess
  case OperAtMost
  case OperGreater
  case OperAtLeast
  case OperEquals
  case OperDiffers
  case OperAnd
  case OperOr
  case OperPipe
