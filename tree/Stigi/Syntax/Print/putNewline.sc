// prompt: start a new line indented by a number of spaces.
def Stigi.Syntax.Print.putNewline {Stigi.Mut} (tables: Stigi.Syntax.Tables, pen: Stigi.Syntax.Print.Pen, indent: U64) <below(0, Stigi.Limit.maxUnitBytes)>: Stigi.Syntax.Print.Pen =
  Stigi.Syntax.Print.putSpaces(tables, Stigi.Syntax.Print.putLiteral(tables, pen, 1, "\n"), indent)
