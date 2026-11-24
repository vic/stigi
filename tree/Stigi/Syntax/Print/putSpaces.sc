// prompt: write a number of spaces at the printer position.
def Stigi.Syntax.Print.putSpaces {Stigi.Mut} (tables: Stigi.Syntax.Tables, pen: Stigi.Syntax.Print.Pen, count: U64) <decreases(count)>: Stigi.Syntax.Print.Pen =
  if count < 1 then pen else Stigi.Syntax.Print.putSpaces(tables, Stigi.Syntax.Print.putLiteral(tables, pen, 1, " "), count - 1)
