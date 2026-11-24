// prompt: map an internal syntax binary operator to its public Stigi.Syntax.OperKind case.
pub def Stigi.Syntax.binOperKind(oper: Stigi.Syntax.BinaryOperator): Stigi.Syntax.OperKind =
  oper match
    case Plus => OperPlus
    case Minus => OperMinus
    case Times => OperTimes
    case Divide => OperDivide
    case Remainder => OperRem
    case LessThan => OperLess
    case AtMost => OperAtMost
    case GreaterThan => OperGreater
    case AtLeast => OperAtLeast
    case Equals => OperEquals
    case Differs => OperDiffers
    case LogicAnd => OperAnd
    case LogicOr => OperOr
    case PipeInto => OperPipe
