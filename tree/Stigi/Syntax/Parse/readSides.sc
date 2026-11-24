// prompt: read the two terms and the operator of a fact comparison or arithmetic fact.
def Stigi.Syntax.Parse.readSides {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress) <below(0, Stigi.Limit.maxUnitBytes)>: Stigi.Syntax.Parse.Operands =
  Stigi.Syntax.Parse.readTerm(tables, start) match
    case Failed(fault, at) => OperandsFailed(fault, at)
    case Built(left, afterLeft) =>
      val Progress(operatorToken, nodes, lists, pending) = afterLeft
      if Stigi.Limit.maxUnitBytes <= operatorToken then OperandsFailed(UnexpectedToken, operatorToken) else
        Stigi.Syntax.Parse.infixOf(Stigi.Syntax.Parse.kindAt(tables, operatorToken)) match
          case Standalone => OperandsFailed(UnexpectedToken, operatorToken)
          case Infix(operator) =>
            Stigi.Syntax.Parse.readTerm(tables, Progress(operatorToken + 1, nodes, lists, pending)) match
              case Failed(fault, at) => OperandsFailed(fault, at)
              case Built(right, after) => Operands(left, operator, right, after)
