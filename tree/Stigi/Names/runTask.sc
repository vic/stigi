// prompt: run the newest task of a unit walk after its step was counted: visit a node, visit a rule atom, bind a local, or drop the locals bound since a mark.
def Stigi.Names.runTask {Stigi.Mut} (walker: Stigi.Names.Walker, image: Stigi.Names.Image, members: Stigi.Names.Members, tables: Stigi.Syntax.Tables, from: U64) <below(from, Stigi.Limit.maxUnits)>: Stigi.Names.Expanded =
  val Walker(tasks, depth, locals, localCount, edges, references, unresolved, steps, fault) = walker
  if depth < 1 then Expanded(walker, NothingCounted) else
    val popped: Stigi.Names.Walker = Walker(tasks, depth - 1, locals, localCount, edges, references, unresolved, steps, fault)
    tasks(depth - 1) match
      case Visit(node) =>
        Stigi.Syntax.readSlot(tables, node) match
          case Fetched(found) => Stigi.Names.expand(popped, image, members, tables, from, found)
          case MalformedTable(_) => Expanded(Stigi.Names.failWith(popped, TableMalformed), NothingCounted)
      case VisitRule(node) =>
        Stigi.Syntax.readSlot(tables, node) match
          case Fetched(found) => Stigi.Names.expandRule(popped, image, members, tables, from, found)
          case MalformedTable(_) => Expanded(Stigi.Names.failWith(popped, TableMalformed), NothingCounted)
      case BindLocal(local) =>
        if localCount < Stigi.Names.maxScope then
          locals(localCount) := local
          Expanded(Walker(tasks, depth - 1, locals, localCount + 1, edges, references, unresolved, steps, fault), NothingCounted)
        else Expanded(Stigi.Names.failWith(popped, ScopeFull), NothingCounted)
      case Restore(mark) => if mark <= localCount then Expanded(Walker(tasks, depth - 1, locals, mark, edges, references, unresolved, steps, fault), NothingCounted) else Expanded(Stigi.Names.failWith(popped, ScopeFull), NothingCounted)
