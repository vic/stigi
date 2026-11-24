// prompt: the binding strength of a binary operator, from the pipe up to the product operators.
pub def Stigi.Syntax.Print.precedence(operator: Stigi.Syntax.BinaryOperator): U64 =
  case PipeInto => 1
  case LogicOr => 2
  case LogicAnd => 3
  case Equals | Differs | LessThan | AtMost | GreaterThan | AtLeast => 4
  case Plus | Minus => 5
  case Times | Divide | Remainder => 6
