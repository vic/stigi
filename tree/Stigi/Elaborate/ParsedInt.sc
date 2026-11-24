// prompt: represent the result of reading one pattern literal bound as a signed 64-bit integer or a bad literal.
enum Stigi.Elaborate.ParsedInt:
  case ParsedInt(value: I64)
  case BadIntLiteral
