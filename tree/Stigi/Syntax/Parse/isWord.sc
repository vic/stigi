// prompt: whether one token is a word: an identifier, a capitalized name, or the state, start or stop keyword used as a name.
def Stigi.Syntax.Parse.isWord(tables: Stigi.Syntax.Tables, token: U64) <below(0, Stigi.Limit.maxUnitBytes)>: Bool =
  val kind: Stigi.Lex.TokenKind = Stigi.Syntax.Parse.kindAt(tables, token)
  kind is TokIdent || kind is TokUpper || kind is TokState || kind is TokStart || kind is TokStop
