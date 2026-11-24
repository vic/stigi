// prompt: write the space and opening delimiter that can lead a printed list layout.
def Stigi.Syntax.Print.openFrame {Stigi.Mut} (tables: Stigi.Syntax.Tables, pen: Stigi.Syntax.Print.Pen, layout: Stigi.Syntax.Print.Layout, first: U64, endPos: U64) <lessOrEqual(first, endPos)>: Stigi.Syntax.Print.Pen =
  layout match
    case JoinedByComma | JoinedByBar | EachOnLine | StackedLines | EachAfterSpace => pen
    case InDelimiters(delimiter) => Stigi.Syntax.Print.putOpen(tables, pen, delimiter)
    case WrappedIn(delimiter) => if first < endPos then Stigi.Syntax.Print.putOpen(tables, pen, delimiter) else pen
    case SpacedIn(delimiter) => if first < endPos then Stigi.Syntax.Print.putOpen(tables, Stigi.Syntax.Print.putLiteral(tables, pen, 1, " "), delimiter) else pen
