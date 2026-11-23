// prompt: a punctuation token kind, or no punctuation at all.
enum Stigi.Lex.Punct:
  case Punct(kind: Stigi.Lex.TokenKind)
  case NotPunct
