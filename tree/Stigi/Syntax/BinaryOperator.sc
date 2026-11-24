// prompt: a binary operator of an expression or a fact, from the pipe to the product operators.
enum Stigi.Syntax.BinaryOperator:
  case Plus
  case Minus
  case Times
  case Divide
  case Remainder
  case LessThan
  case AtMost
  case GreaterThan
  case AtLeast
  case Equals
  case Differs
  case LogicAnd
  case LogicOr
  case PipeInto
