// prompt: record a reference to an operation of a capability owner: an edge to the owner when it has the operation, else an unresolved reference.
def Stigi.Names.referOperation {Stigi.Mut} (walker: Stigi.Names.Walker, members: Stigi.Names.Members, tables: Stigi.Syntax.Tables, from: U64, owner: U64, text: Stigi.Syntax.Text) <below(from, Stigi.Limit.maxUnits), below(owner, Stigi.Limit.maxUnits)>: Stigi.Names.Expanded =
  val Text(first, end) = text
  if Stigi.Names.hasOperation(members, tables, owner, first, end, Stigi.Names.firstMember(members, tables, first, end), Stigi.Names.tableSlots) then
    Expanded(Stigi.Names.walkEdge(walker, from, owner), ReferenceCounted)
  else Expanded(walker, UnresolvedReferenceCounted)
