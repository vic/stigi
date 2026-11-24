// prompt: record one dependency edge of a unit walk from its unit to a unit the walk resolved: a full edge table is an edges fault.
def Stigi.Names.walkEdge {Stigi.Mut} (walker: Stigi.Names.Walker, from: U64, to: U64) <below(from, Stigi.Limit.maxUnits), below(to, Stigi.Limit.maxUnits)>: Stigi.Names.Walker =
  val Walker(tasks, depth, locals, localCount, edges, references, unresolved, steps, fault) = walker
  Stigi.Names.addEdge(edges, from, to) match
    case EdgeKept(next) => Walker(tasks, depth, locals, localCount, next, references, unresolved, steps, fault)
    case EdgesFull => Stigi.Names.failWith(walker, EdgesExhausted)
