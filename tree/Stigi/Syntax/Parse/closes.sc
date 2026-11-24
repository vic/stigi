// prompt: whether a token kind closes a bracket pair or ends a line list.
def Stigi.Syntax.Parse.closes(kind: Stigi.Lex.TokenKind, bracket: Stigi.Syntax.Parse.Bracket): Bool =
  bracket match
    case Parens => kind is TokRightParen
    case Brackets => kind is TokRightBrack
    case Braces => kind is TokRightBrace
    case Angles => kind is TokGreat
    case LineEnd => kind is TokNewline || kind is TokEnd
