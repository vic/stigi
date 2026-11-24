// prompt: print the body after a separator: the statements of a block each on its own line, or one space and the expression.
def Stigi.Syntax.Print.printBody {Stigi.Mut} (tables: Stigi.Syntax.Tables, body: Stigi.Syntax.NodeAt, pen: Stigi.Syntax.Print.Pen, indent: U64, depth: U64) <lessOrEqual(depth, Stigi.Limit.maxParseDepth), decreases(depth)>: Stigi.Syntax.Print.Pen =
  if depth < 1 then PenFailed(PrintTooDeep) else if Stigi.Limit.maxUnitBytes <= indent then PenOverflow else
    Stigi.Syntax.readSlot(tables, body) match
      case MalformedTable(slot) => PenFailed(MalformedNode(slot))
      case Fetched(found) =>
        Stigi.Syntax.Print.flowOf(found) match
          case BlockFlow(statements) => Stigi.Syntax.Print.printList(tables, statements, EachOnLine, pen, indent + 2, depth - 1)
          case AlwaysMultiline | BranchFlow(_, _) | ChildFlow(_) | SingleLine => Stigi.Syntax.Print.printNode(tables, body, Stigi.Syntax.Print.putLiteral(tables, pen, 1, " "), indent, depth - 1)
