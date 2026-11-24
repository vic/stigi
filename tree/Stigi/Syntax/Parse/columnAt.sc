// prompt: the column of one token in its line.
def Stigi.Syntax.Parse.columnAt(tables: Stigi.Syntax.Tables, token: U64) <below(0, Stigi.Limit.maxUnitBytes)>: U64 =
  val Token(_, _, _, _, column) = Stigi.Syntax.Parse.tokenAt(tables, token)
  column
