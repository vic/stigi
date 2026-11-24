// prompt: print the scrutinees of a match: one, parenthesized when it is a control expression, or several in parentheses.
def Stigi.Syntax.Print.printScrutinees {Stigi.Mut} (tables: Stigi.Syntax.Tables, scrutinees: Stigi.Syntax.Span, pen: Stigi.Syntax.Print.Pen, indent: U64, depth: U64) <lessOrEqual(depth, Stigi.Limit.maxParseDepth), decreases(depth)>: Stigi.Syntax.Print.Pen =
  val Span(first, end) = scrutinees
  val Tables(_, _, _, lists, _, _) = tables
  if depth < 1 then PenFailed(PrintTooDeep) else if first < end && end - first < 2 then
    val single: U64 = lists(first)
    if single < Stigi.Limit.maxUnitNodes then Stigi.Syntax.Print.printParened(tables, single, Stigi.Syntax.Print.scrutineeParens(Stigi.Syntax.Print.shapeOf(tables, single)), pen, indent, depth - 1) else PenFailed(MalformedNode(first))
  else Stigi.Syntax.Print.enclosed(tables, scrutinees, InParens, pen, indent, depth - 1)
