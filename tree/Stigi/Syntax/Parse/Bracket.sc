// prompt: the pair of tokens that encloses a list, or the end of the line that ends a list.
enum Stigi.Syntax.Parse.Bracket:
  case Parens
  case Brackets
  case Braces
  case Angles
  case LineEnd
