// prompt: how a token kind nests inside a parenthesized token run.
enum Stigi.Parse.Nesting:
  case Opens
  case Closes
  case Separates
  case Ends
  case Inside
