// prompt: push the binding of every parameter of a span on the task stack so that the first parameter is bound first, failing the walk on a malformed entry.
def Stigi.Names.pushParamBinders {Stigi.Mut} (walker: Stigi.Names.Walker, tables: Stigi.Syntax.Tables, span: Stigi.Syntax.Span): Stigi.Names.Walker =
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
        case ParamFound(name, _) => Stigi.Names.pushTask(current, BindLocal(Local(name, WithoutCapability)))
    else Stigi.Names.failWith(current, TableMalformed)
    at = at - 1
  current
