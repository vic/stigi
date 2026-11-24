// prompt: print a list in its delimiters, also when it is empty.
def Stigi.Syntax.Print.enclosed {Stigi.Mut} (tables: Stigi.Syntax.Tables, span: Stigi.Syntax.Span, delimiter: Stigi.Syntax.Print.Delimiter, pen: Stigi.Syntax.Print.Pen, indent: U64, depth: U64) <lessOrEqual(depth, Stigi.Limit.maxParseDepth), decreases(depth)>: Stigi.Syntax.Print.Pen =
  if depth < 1 then PenFailed(PrintTooDeep) else Stigi.Syntax.Print.putClose(tables, Stigi.Syntax.Print.printList(tables, span, JoinedByComma, Stigi.Syntax.Print.putOpen(tables, pen, delimiter), indent, depth - 1), delimiter)
