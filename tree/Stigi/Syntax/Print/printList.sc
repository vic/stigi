// prompt: print the items of a span of the list table in one layout, including optional or mandatory delimiters.
def Stigi.Syntax.Print.printList {Stigi.Mut} (tables: Stigi.Syntax.Tables, span: Stigi.Syntax.Span, layout: Stigi.Syntax.Print.Layout, pen: Stigi.Syntax.Print.Pen, indent: U64, depth: U64) <lessOrEqual(depth, Stigi.Limit.maxParseDepth), decreases(depth)>: Stigi.Syntax.Print.Pen =
  val Span(first, endPos) = span
  if depth < 1 then PenFailed(PrintTooDeep) else if Stigi.Limit.maxUnitBytes < indent then PenOverflow else
    val Tables(_, _, _, lists, _, _) = tables
    var current: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.openFrame(tables, pen, layout, first, endPos)
    var index: U64 = first
    while index < endPos && current is Pen(_) do
      @<lessOrEqual(index, endPos)>
      val item: U64 = lists(index)
      current = if item < Stigi.Limit.maxUnitNodes then Stigi.Syntax.Print.printNode(tables, item, Stigi.Syntax.Print.putSeparator(tables, current, layout, index == first, indent), indent, depth - 1) else PenFailed(MalformedNode(index))
      index = index + 1
    Stigi.Syntax.Print.closeFrame(tables, current, layout, first, endPos)
