// prompt: one step of the walk of a unit: visit a node, visit an atom of a rule by its names alone, bind a local, or drop the locals bound since a mark.
enum Stigi.Names.Task:
  case Visit(node: Stigi.Syntax.NodeAt)
  case VisitRule(node: Stigi.Syntax.NodeAt)
  case BindLocal(local: Stigi.Names.Local)
  case Restore(mark: U64)
