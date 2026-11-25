// prompt: represent the result of reading one unsigned expression literal: its 64-bit word, a value past the largest unsigned word, or a malformed literal.
enum Stigi.Elaborate.WideInt:
  case WideFits(value: U64)
  case WideOverflow
  case WideMalformed
