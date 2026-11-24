// prompt: the two operands and the operator of a fact comparison or arithmetic fact, or the fault.
enum Stigi.Syntax.Parse.Operands:
  case Operands(left: Stigi.Syntax.NodeAt, operator: Stigi.Syntax.BinaryOperator, right: Stigi.Syntax.NodeAt, progress: Stigi.Syntax.Parse.Progress)
  case OperandsFailed(fault: Stigi.Syntax.Parse.Fault, token: U64)
