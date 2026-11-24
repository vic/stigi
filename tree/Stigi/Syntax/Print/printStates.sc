// prompt: print the state, start and stop lines of a capability, entering printNode directly on startState.
def Stigi.Syntax.Print.printStates {Stigi.Mut} (tables: Stigi.Syntax.Tables, states: Stigi.Syntax.Span, startState: Stigi.Syntax.NodeAt, stops: Stigi.Syntax.Span, pen: Stigi.Syntax.Print.Pen, indent: U64, depth: U64) <lessOrEqual(depth, Stigi.Limit.maxParseDepth), decreases(depth)>: Stigi.Syntax.Print.Pen =
  if depth < 1 then PenFailed(PrintTooDeep) else if !Stigi.Syntax.Print.hasItems(states) then pen else
    val stated: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printList(tables, states, JoinedByComma, Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.putNewline(tables, pen, indent), 6, "state "), indent, depth - 1)
    val started: Stigi.Syntax.Print.Pen = Stigi.Syntax.Print.printNode(tables, startState, Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.putNewline(tables, stated, indent), 6, "start "), indent, depth - 1)
    Stigi.Syntax.Print.printList(tables, stops, JoinedByComma, Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.putNewline(tables, started, indent), 5, "stop "), indent, depth - 1)
