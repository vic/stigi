// prompt: write the closing delimiter that can end a printed list layout.
def Stigi.Syntax.Print.closeFrame {Stigi.Mut} (tables: Stigi.Syntax.Tables, pen: Stigi.Syntax.Print.Pen, layout: Stigi.Syntax.Print.Layout, first: U64, endPos: U64) <lessOrEqual(first, endPos)>: Stigi.Syntax.Print.Pen =
  layout match
    case JoinedByComma | JoinedByBar | EachOnLine | StackedLines | EachAfterSpace => pen
    case InDelimiters(delimiter) => Stigi.Syntax.Print.putClose(tables, pen, delimiter)
    case WrappedIn(delimiter) | SpacedIn(delimiter) => if first < endPos then Stigi.Syntax.Print.putClose(tables, pen, delimiter) else pen
