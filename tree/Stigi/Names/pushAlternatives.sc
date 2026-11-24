// prompt: push the alternatives of an or-pattern so that only the first binds: the first alternative runs last and keeps its locals, a mark drops the locals of every other alternative.
def Stigi.Names.pushAlternatives {Stigi.Mut} (walker: Stigi.Names.Walker, tables: Stigi.Syntax.Tables, span: Stigi.Syntax.Span): Stigi.Names.Walker =
  val Span(first, end) = span
  val Tables(_, _, _, lists, _, _) = tables
  val Walker(_, _, _, localCount, _, _, _, _, _) = walker
  if first < end then
    val next: U64 = first + 1
    val head: U64 = lists(first)
    var current: Stigi.Names.Walker = if head < Stigi.Limit.maxUnitNodes then Stigi.Names.pushTask(Stigi.Names.pushTask(walker, Visit(head)), Restore(localCount)) else Stigi.Names.failWith(walker, TableMalformed)
    var at: U64 = end
    while next < at do
      @<lessOrEqual(at, end)>
      val item: U64 = lists(at - 1)
      current = if item < Stigi.Limit.maxUnitNodes then Stigi.Names.pushTask(current, Visit(item)) else Stigi.Names.failWith(current, TableMalformed)
      at = at - 1
    current
  else walker
