// prompt: single-case split point returned when locating the lowest-precedence top-level operator.
enum Stigi.Emit.OperSplit:
  case OperFound(operPos: U64, operKind: Stigi.Lex.TokenKind)
  case OperNone
