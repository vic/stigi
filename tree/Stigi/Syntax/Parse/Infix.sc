// prompt: the binary operator that a token kind writes, or none.
enum Stigi.Syntax.Parse.Infix:
  case Infix(operator: Stigi.Syntax.BinaryOperator)
  case Standalone
