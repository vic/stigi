// prompt: the last token of a dotted name that starts at a word token.
def Stigi.Syntax.Parse.nameLast(tables: Stigi.Syntax.Tables, token: U64, remaining: U64) <decreases(remaining)>: U64 =
  if remaining < 1 || Stigi.Limit.maxUnitBytes <= token then token else
    if Stigi.Syntax.Parse.kindAt(tables, token + 1) is TokDot && Stigi.Syntax.Parse.isWord(tables, token + 2) then Stigi.Syntax.Parse.nameLast(tables, token + 2, remaining - 1) else token
