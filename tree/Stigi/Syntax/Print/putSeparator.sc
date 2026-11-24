// prompt: write what comes before one item of a printed list.
def Stigi.Syntax.Print.putSeparator {Stigi.Mut} (tables: Stigi.Syntax.Tables, pen: Stigi.Syntax.Print.Pen, layout: Stigi.Syntax.Print.Layout, isFirst: Bool, indent: U64) <below(0, Stigi.Limit.maxUnitBytes)>: Stigi.Syntax.Print.Pen =
  layout match
    case JoinedByComma => if isFirst then pen else Stigi.Syntax.Print.putLiteral(tables, pen, 2, ", ")
    case JoinedByBar => if isFirst then pen else Stigi.Syntax.Print.putLiteral(tables, pen, 3, " | ")
    case EachOnLine => Stigi.Syntax.Print.putNewline(tables, pen, indent)
    case StackedLines => if isFirst then pen else Stigi.Syntax.Print.putNewline(tables, pen, indent)
    case EachAfterSpace => Stigi.Syntax.Print.putLiteral(tables, pen, 1, " ")
