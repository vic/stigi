// prompt: where the following list items of a call or a handle start: its type and capability arguments and whether value arguments in parentheses follow.
enum Stigi.Syntax.Parse.CallHead:
  case CallHead(typeArgs: Stigi.Syntax.Span, capArgs: Stigi.Syntax.Span, parens: Bool, progress: Stigi.Syntax.Parse.Progress)
  case CallHeadFailed(fault: Stigi.Syntax.Parse.Fault, token: U64)
