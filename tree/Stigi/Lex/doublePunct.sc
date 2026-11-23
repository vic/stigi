// prompt: classify a two-byte operator pair into its token kind or TokEnd if not a pair.
def Stigi.Lex.doublePunct(first: U8, second: U8): Stigi.Lex.TokenKind =
  case (124, 62) => TokPipe
  case (61, 62) => TokArrow
  case (58, 61) => TokColonAssign
  case (61, 61) => TokEqual
  case (33, 61) => TokNotEqual
  case (60, 61) => TokLessEqual
  case (62, 61) => TokGreatEqual
  case (38, 38) => TokLogicAnd
  case (124, 124) => TokLogicOr
  case (_, _) => TokEnd
