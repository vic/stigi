// prompt: the kind of one token of the token table.
def Stigi.Syntax.Parse.kindAt(tables: Stigi.Syntax.Tables, token: U64) <below(0, Stigi.Limit.maxUnitBytes)>: Stigi.Lex.TokenKind =
  val Token(kind, _, _, _, _) = Stigi.Syntax.Parse.tokenAt(tables, token)
  kind
