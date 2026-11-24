// prompt: write a binary operator with one space on each side.
def Stigi.Syntax.Print.putBinary {Stigi.Mut} (tables: Stigi.Syntax.Tables, pen: Stigi.Syntax.Print.Pen, operator: Stigi.Syntax.BinaryOperator) <below(0, Stigi.Limit.maxUnitBytes)>: Stigi.Syntax.Print.Pen =
  operator match
    case PipeInto => Stigi.Syntax.Print.putLiteral(tables, pen, 4, " |> ")
    case LogicOr => Stigi.Syntax.Print.putLiteral(tables, pen, 4, " || ")
    case LogicAnd => Stigi.Syntax.Print.putLiteral(tables, pen, 4, " && ")
    case Equals => Stigi.Syntax.Print.putLiteral(tables, pen, 4, " == ")
    case Differs => Stigi.Syntax.Print.putLiteral(tables, pen, 4, " != ")
    case LessThan => Stigi.Syntax.Print.putLiteral(tables, pen, 3, " < ")
    case AtMost => Stigi.Syntax.Print.putLiteral(tables, pen, 4, " <= ")
    case GreaterThan => Stigi.Syntax.Print.putLiteral(tables, pen, 3, " > ")
    case AtLeast => Stigi.Syntax.Print.putLiteral(tables, pen, 4, " >= ")
    case Plus => Stigi.Syntax.Print.putLiteral(tables, pen, 3, " + ")
    case Minus => Stigi.Syntax.Print.putLiteral(tables, pen, 3, " - ")
    case Times => Stigi.Syntax.Print.putLiteral(tables, pen, 3, " * ")
    case Divide => Stigi.Syntax.Print.putLiteral(tables, pen, 3, " / ")
    case Remainder => Stigi.Syntax.Print.putLiteral(tables, pen, 3, " % ")
