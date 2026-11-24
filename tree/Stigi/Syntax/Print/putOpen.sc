// prompt: write the opening character of a delimiter.
def Stigi.Syntax.Print.putOpen {Stigi.Mut} (tables: Stigi.Syntax.Tables, pen: Stigi.Syntax.Print.Pen, delimiter: Stigi.Syntax.Print.Delimiter) <below(0, Stigi.Limit.maxUnitBytes)>: Stigi.Syntax.Print.Pen =
  delimiter match
    case InParens => Stigi.Syntax.Print.putLiteral(tables, pen, 1, "(")
    case InBrackets => Stigi.Syntax.Print.putLiteral(tables, pen, 1, "[")
    case InBraces => Stigi.Syntax.Print.putLiteral(tables, pen, 1, "{")
