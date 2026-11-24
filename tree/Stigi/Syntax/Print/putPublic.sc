// prompt: write the pub keyword of a public declaration.
def Stigi.Syntax.Print.putPublic {Stigi.Mut} (tables: Stigi.Syntax.Tables, pen: Stigi.Syntax.Print.Pen, isPublic: Bool) <below(0, Stigi.Limit.maxUnitBytes)>: Stigi.Syntax.Print.Pen =
  if isPublic then Stigi.Syntax.Print.putLiteral(tables, pen, 4, "pub ") else pen
