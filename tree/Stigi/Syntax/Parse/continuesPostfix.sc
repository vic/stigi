// prompt: whether a call, a type or capability argument list, or an assertion follows the parsed operand.
def Stigi.Syntax.Parse.continuesPostfix(tables: Stigi.Syntax.Tables, built: Stigi.Syntax.Parse.Built) <below(0, Stigi.Limit.maxUnitBytes)>: Bool =
  built match
    case Failed(_, _) => false
    case Built(_, progress) =>
      val Progress(token, _, _, _) = progress
      val kind: Stigi.Lex.TokenKind = Stigi.Syntax.Parse.kindAt(tables, token)
      kind is TokLeftBrack || kind is TokLeftBrace || kind is TokLeftParen || kind is TokAt
