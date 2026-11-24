// prompt: whether a node spans several lines when printed.
def Stigi.Syntax.Print.isMultiline(tables: Stigi.Syntax.Tables, node: Stigi.Syntax.NodeAt, depth: U64) <lessOrEqual(depth, Stigi.Limit.maxParseDepth), decreases(depth)>: Bool =
  if depth < 1 then false else
    Stigi.Syntax.readSlot(tables, node) match
      case MalformedTable(_) => false
      case Fetched(found) =>
        Stigi.Syntax.Print.flowOf(found) match
          case BlockFlow(_) | AlwaysMultiline => true
          case BranchFlow(whenTrue, whenFalse) => Stigi.Syntax.Print.isMultiline(tables, whenTrue, depth - 1) || Stigi.Syntax.Print.isMultiline(tables, whenFalse, depth - 1)
          case ChildFlow(child) => Stigi.Syntax.Print.isMultiline(tables, child, depth - 1)
          case SingleLine => false
