// prompt: write the keyword of a def or a lemma def.
def Stigi.Syntax.Print.putFlavor {Stigi.Mut} (tables: Stigi.Syntax.Tables, pen: Stigi.Syntax.Print.Pen, flavor: Stigi.Syntax.DefFlavor) <below(0, Stigi.Limit.maxUnitBytes)>: Stigi.Syntax.Print.Pen =
  flavor match
    case PlainDef => Stigi.Syntax.Print.putLiteral(tables, pen, 4, "def ")
    case LemmaDef => Stigi.Syntax.Print.putLiteral(tables, pen, 10, "lemma def ")
