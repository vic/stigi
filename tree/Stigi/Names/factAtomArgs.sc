// prompt: the argument run of a fact atom that holds values: the second argument of an at atom with exactly two arguments names a state and is left out.
def Stigi.Names.factAtomArgs(tables: Stigi.Syntax.Tables, name: Stigi.Syntax.Text, first: U64, end: U64) <lessOrEqual(first, end), lessOrEqual(end, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Span =
  name match
    case Text(nameFirst, nameEnd) =>
      if Stigi.Syntax.textIs(tables, nameFirst, nameEnd, 2, "at", 0, 3) || Stigi.Syntax.textIs(tables, nameFirst, nameEnd, 14, "Stigi.Facts.at", 0, 15) then
        if first < end then
          val second: U64 = first + 1
          if second < end then
            val third: U64 = second + 1
            if third == end then Span(first, second) else Span(first, end)
          else Span(first, end)
        else Span(first, end)
      else Span(first, end)
