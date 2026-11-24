// prompt: write a prefix operator.
def Stigi.Syntax.Print.putUnary {Stigi.Mut} (tables: Stigi.Syntax.Tables, pen: Stigi.Syntax.Print.Pen, operator: Stigi.Syntax.UnaryOperator) <below(0, Stigi.Limit.maxUnitBytes)>: Stigi.Syntax.Print.Pen =
  operator match
    case LogicNot => Stigi.Syntax.Print.putLiteral(tables, pen, 1, "!")
    case Negate => Stigi.Syntax.Print.putLiteral(tables, pen, 1, "-")
