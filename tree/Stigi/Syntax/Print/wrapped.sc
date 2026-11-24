// prompt: print a list in its delimiters only when it holds items.
def Stigi.Syntax.Print.wrapped {Stigi.Mut} (tables: Stigi.Syntax.Tables, span: Stigi.Syntax.Span, delimiter: Stigi.Syntax.Print.Delimiter, pen: Stigi.Syntax.Print.Pen, indent: U64, depth: U64) <lessOrEqual(depth, Stigi.Limit.maxParseDepth), decreases(depth)>: Stigi.Syntax.Print.Pen =
  if depth < 1 then PenFailed(PrintTooDeep) else if Stigi.Syntax.Print.hasItems(span) then Stigi.Syntax.Print.enclosed(tables, span, delimiter, pen, indent, depth - 1) else pen
