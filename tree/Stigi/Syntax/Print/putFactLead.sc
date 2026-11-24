// prompt: print the optional negation and evidence prefix of a fact.
def Stigi.Syntax.Print.putFactLead {Stigi.Mut} (tables: Stigi.Syntax.Tables, pen: Stigi.Syntax.Print.Pen, negated: Bool, evidence: Stigi.Syntax.NodeAt) <below(evidence, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Print.Pen =
  val negPen: Stigi.Syntax.Print.Pen = if negated then Stigi.Syntax.Print.putLiteral(tables, pen, 4, "not ") else pen
  if Stigi.Syntax.Print.isAbsent(tables, evidence) then negPen else Stigi.Syntax.Print.putLiteral(tables, Stigi.Syntax.Print.printTermAtom(tables, evidence, false, negPen), 2, ": ")
