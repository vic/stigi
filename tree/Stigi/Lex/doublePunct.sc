// prompt: classify a two-byte operator pair into its token kind.
def Stigi.Lex.doublePunct(first: U8, second: U8): Stigi.Lex.Punct =
  case ('|', '>') => Punct(TokPipe)
  case ('=', '>') => Punct(TokArrow)
  case (':', '=') => Punct(TokColonAssign)
  case ('=', '=') => Punct(TokEqual)
  case ('!', '=') => Punct(TokNotEqual)
  case ('<', '=') => Punct(TokLessEqual)
  case ('>', '=') => Punct(TokGreatEqual)
  case ('&', '&') => Punct(TokLogicAnd)
  case ('|', '|') => Punct(TokLogicOr)
  case ('.', '.') => Punct(TokDotDot)
  case (_, _) => NotPunct
