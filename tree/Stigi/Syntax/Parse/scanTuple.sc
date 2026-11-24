// prompt: whether the parenthesized group that the scan is in holds a comma at its own level and is followed by match.
def Stigi.Syntax.Parse.scanTuple(tables: Stigi.Syntax.Tables, token: U64, nesting: U64, commas: Bool, remaining: U64) <decreases(remaining)>: Bool =
  if remaining < 1 || Stigi.Limit.maxUnitBytes <= token || Stigi.Limit.maxUnitBytes <= nesting then false else
    val kind: Stigi.Lex.TokenKind = Stigi.Syntax.Parse.kindAt(tables, token)
    if kind is TokLeftParen || kind is TokLeftBrack || kind is TokLeftBrace then Stigi.Syntax.Parse.scanTuple(tables, token + 1, nesting + 1, commas, remaining - 1) else if kind is TokRightParen || kind is TokRightBrack || kind is TokRightBrace then
      if nesting < 2 then commas && Stigi.Syntax.Parse.kindAt(tables, token + 1) is TokMatch else Stigi.Syntax.Parse.scanTuple(tables, token + 1, nesting - 1, commas, remaining - 1)
    else if kind is TokNewline || kind is TokEnd then false else Stigi.Syntax.Parse.scanTuple(tables, token + 1, nesting, commas || nesting < 2 && kind is TokComma, remaining - 1)
