// prompt: push a visit of every node of a list span on the task stack so that the first item is visited first, failing the walk on a malformed item.
def Stigi.Names.pushSpan {Stigi.Mut} (walker: Stigi.Names.Walker, tables: Stigi.Syntax.Tables, span: Stigi.Syntax.Span): Stigi.Names.Walker =
  val Span(first, end) = span
  val Tables(_, _, _, lists, _, _) = tables
  var current: Stigi.Names.Walker = walker
  var at: U64 = end
  while first < at do
    @<lessOrEqual(at, end)>
    val item: U64 = lists(at - 1)
    current = if item < Stigi.Limit.maxUnitNodes then Stigi.Names.pushTask(current, Visit(item)) else Stigi.Names.failWith(current, TableMalformed)
    at = at - 1
  current
