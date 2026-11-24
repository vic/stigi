// prompt: write the closing character of a delimiter.
def Stigi.Syntax.Print.putClose {Stigi.Mut} (tables: Stigi.Syntax.Tables, pen: Stigi.Syntax.Print.Pen, delimiter: Stigi.Syntax.Print.Delimiter) <below(0, Stigi.Limit.maxUnitBytes)>: Stigi.Syntax.Print.Pen =
  delimiter match
    case InParens => Stigi.Syntax.Print.putLiteral(tables, pen, 1, ")")
    case InBrackets => Stigi.Syntax.Print.putLiteral(tables, pen, 1, "]")
    case InBraces => Stigi.Syntax.Print.putLiteral(tables, pen, 1, "}")
