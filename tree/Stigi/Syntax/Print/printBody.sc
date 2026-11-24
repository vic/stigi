// prompt: print the body after a separator: the statements of a block each on its own line (spending two hops total from caller to statement), or one space and the expression.
def Stigi.Syntax.Print.printBody {Stigi.Mut} (tables: Stigi.Syntax.Tables, body: Stigi.Syntax.NodeAt, pen: Stigi.Syntax.Print.Pen, indent: U64, depth: U64) <lessOrEqual(depth, Stigi.Limit.maxParseDepth), decreases(depth)>: Stigi.Syntax.Print.Pen =
  if depth < 1 then PenFailed(PrintTooDeep) else if Stigi.Limit.maxUnitBytes <= indent then PenOverflow else
    Stigi.Syntax.readSlot(tables, body) match
      case MalformedTable(slot) => PenFailed(MalformedNode(slot))
      case Fetched(found) =>
        Stigi.Syntax.Print.flowOf(found) match
          case BlockFlow(statements) =>
            val Span(first, endPos) = statements
            val Tables(_, _, _, lists, _, _) = tables
            var current: Stigi.Syntax.Print.Pen = pen
            var index: U64 = first
            while index < endPos && current is Pen(_) do
              @<lessOrEqual(index, endPos)>
              val item: U64 = lists(index)
              current = if item < Stigi.Limit.maxUnitNodes then Stigi.Syntax.Print.printNode(tables, item, Stigi.Syntax.Print.putNewline(tables, current, indent + 2), indent + 2, depth - 1) else PenFailed(MalformedNode(index))
              index = index + 1
            current
          case AlwaysMultiline | BranchFlow(_, _) | ChildFlow(_) | SingleLine => Stigi.Syntax.Print.printNode(tables, body, Stigi.Syntax.Print.putLiteral(tables, pen, 1, " "), indent, depth - 1)
