// prompt: whether the token at an index is the word alias.
def Stigi.Syntax.Parse.isAliasWord(tables: Stigi.Syntax.Tables, token: U64) <below(0, Stigi.Limit.maxUnitBytes)>: Bool =
  if Stigi.Limit.maxUnitBytes <= token || !(Stigi.Syntax.Parse.kindAt(tables, token) is TokIdent) then false else
    Stigi.Syntax.Parse.spanText(tables, token, token) match
      case Text(first, end) => Stigi.Syntax.textIs(tables, first, end, 5, "alias", 0, 6)
