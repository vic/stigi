// prompt: whether a span of the list table holds any item.
def Stigi.Syntax.Print.hasItems(span: Stigi.Syntax.Span): Bool =
  val Span(first, end) = span
  first < end
