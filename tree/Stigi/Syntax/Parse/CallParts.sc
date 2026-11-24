// prompt: the callee and the arguments of a call node, or a node that is no call.
enum Stigi.Syntax.Parse.CallParts:
  case CallParts(callee: Stigi.Syntax.NodeAt, args: Stigi.Syntax.Span)
  case PlainNode
