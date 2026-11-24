// prompt: scope the binder of a handle statement: a handle with an explicit body keeps its binder only inside that body, so the scope is restored after the body; a handle without a body is a statement whose binder lives on until the enclosing block ends, like a val local.
def Stigi.Names.handleScope {Stigi.Mut} (walker: Stigi.Names.Walker, tables: Stigi.Syntax.Tables, body: Stigi.Syntax.NodeAt, localCount: U64) <below(body, Stigi.Limit.maxUnitNodes)>: Stigi.Names.Walker =
  Stigi.Syntax.readSlot(tables, body) match
    case MalformedTable(_) => Stigi.Names.failWith(walker, TableMalformed)
    case Fetched(bodyNode) => if bodyNode is Absent(_) then walker else Stigi.Names.pushTask(walker, Restore(localCount))
