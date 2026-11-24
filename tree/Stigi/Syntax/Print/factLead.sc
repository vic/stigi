// prompt: print the negation and the evidence name that can lead a fact.
def Stigi.Syntax.Print.factLead {Stigi.Mut} (tables: Stigi.Syntax.Tables, negated: Bool, evidence: Stigi.Syntax.NodeAt, pen: Stigi.Syntax.Print.Pen, indent: U64, depth: U64) <lessOrEqual(depth, Stigi.Limit.maxParseDepth), decreases(depth)>: Stigi.Syntax.Print.Pen =
  val negatedPen: Stigi.Syntax.Print.Pen = if negated then Stigi.Syntax.Print.putLiteral(tables, pen, 4, "not ") else pen
  if depth < 1 then PenFailed(PrintTooDeep) else if Stigi.Syntax.Print.isAbsent(tables, evidence) then negatedPen else Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.printNode(tables, evidence, negatedPen, indent, depth - 1), 2, ": ")
