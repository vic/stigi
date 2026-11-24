// prompt: where the else keyword of a conditional sits, or that it is missing.
enum Stigi.Syntax.Parse.ElsePosition:
  case ElseFound(elseToken: U64)
  case ElseMissing
