// prompt: the first token at or after a token that is not a line break.
def Stigi.Syntax.Parse.lineStart(tables: Stigi.Syntax.Tables, token: U64, remaining: U64) <decreases(remaining)>: U64 =
  if remaining < 1 || Stigi.Limit.maxUnitBytes <= token || !(Stigi.Syntax.Parse.kindAt(tables, token) is TokNewline) then token else Stigi.Syntax.Parse.lineStart(tables, token + 1, remaining - 1)
