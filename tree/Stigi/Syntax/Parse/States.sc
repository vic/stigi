// prompt: the state, start and stop lines of a capability.
enum Stigi.Syntax.Parse.States:
  case States(states: Stigi.Syntax.Span, startState: Stigi.Syntax.NodeAt, stops: Stigi.Syntax.Span, progress: Stigi.Syntax.Parse.Progress)
  case StatesFailed(fault: Stigi.Syntax.Parse.Fault, token: U64)
