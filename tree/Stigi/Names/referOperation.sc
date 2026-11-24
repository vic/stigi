// prompt: record a reference to an operation of a capability owner: an edge to the owner when it has the operation, else an unresolved reference.
def Stigi.Names.referOperation {Stigi.Mut} (walker: Stigi.Names.Walker, members: Stigi.Names.Members, tables: Stigi.Syntax.Tables, from: U64, owner: U64, first: U64, end: U64) <below(from, Stigi.Limit.maxUnits), lessOrEqual(first, end), lessOrEqual(end, Stigi.Limit.maxUnitBytes)>: Stigi.Names.Expanded =
  if Stigi.Names.hasOperation(members, tables, owner, first, end, Stigi.Names.firstMember(members, tables, first, end), Stigi.Names.tableSlots) then
    val Walker(tasks, depth, locals, localCount, edges, references, unresolved, steps, fault) = walker
    Expanded(Walker(tasks, depth, locals, localCount, Stigi.Names.addEdge(edges, from, owner), references, unresolved, steps, fault), ReferenceCounted)
  else Expanded(walker, UnresolvedReferenceCounted)
