// prompt: the front of a def head: its type parameters, the groups before its value parameters and the value parameters.
enum Stigi.Syntax.Parse.HeadFront:
  case HeadFront(typeParams: Stigi.Syntax.Span, preGroups: Stigi.Syntax.Span, valueGroup: Bool, params: Stigi.Syntax.Span, progress: Stigi.Syntax.Parse.Progress)
  case HeadFrontFailed(fault: Stigi.Syntax.Parse.Fault, token: U64)
