// prompt: push the visit of the declared type of every parameter of a span on the task stack so that the first type is visited first, failing the walk on a malformed entry.
def Stigi.Names.pushParamTypes {Stigi.Mut} (walker: Stigi.Names.Walker, tables: Stigi.Syntax.Tables, span: Stigi.Syntax.Span): Stigi.Names.Walker =
  val Span(first, end) = span
  val Tables(_, _, _, lists, _, _) = tables
  var current: Stigi.Names.Walker = walker
  var at: U64 = end
  while first < at do
    @<lessOrEqual(at, end)>
    val item: U64 = lists(at - 1)
    current = if item < Stigi.Limit.maxUnitNodes then
      Stigi.Names.paramAt(tables, item) match
        case ParamBroken => Stigi.Names.failWith(current, TableMalformed)
        case ParamFound(_, declared) => Stigi.Names.pushTask(current, Visit(declared))
    else Stigi.Names.failWith(current, TableMalformed)
    at = at - 1
  current
