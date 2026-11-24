// prompt: print a span of fact nodes in one layout without node-depth recursion.
def Stigi.Syntax.Print.printFactList {Stigi.Mut} (tables: Stigi.Syntax.Tables, span: Stigi.Syntax.Span, layout: Stigi.Syntax.Print.Layout, pen: Stigi.Syntax.Print.Pen, indent: U64) <lessOrEqual(indent, Stigi.Limit.maxUnitBytes)>: Stigi.Syntax.Print.Pen =
  val Span(first, endPos) = span
  val Tables(_, _, _, lists, _, _) = tables
  var current: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.openFrame(tables, pen, layout, first, endPos)
  var index: U64 = first
  while index < endPos && current is Pen(_) do
    @<lessOrEqual(index, endPos)>
    val item: U64 = lists(index)
    current = if item < Stigi.Limit.maxUnitNodes then Stigi.Syntax.Print.printFact(tables, item, Stigi.Syntax.Print.putSeparator(tables, current, layout, index == first, indent)) else PenFailed(MalformedNode(index))
    index = index + 1
  Stigi.Syntax.Print.closeFrame(tables, current, layout, first, endPos)
