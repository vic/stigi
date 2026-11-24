// prompt: whether a node spans several lines when printed: a block, always, by its two branches, by one child, or never.
enum Stigi.Syntax.Print.Flow:
  case BlockFlow(statements: Stigi.Syntax.Span)
  case AlwaysMultiline
  case BranchFlow(whenTrue: Stigi.Syntax.NodeAt, whenFalse: Stigi.Syntax.NodeAt)
  case ChildFlow(child: Stigi.Syntax.NodeAt)
  case SingleLine
