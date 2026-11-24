// prompt: one step down an iterative postfix or infix left spine.
enum Stigi.Syntax.Print.LeftStep:
  case SpineNext(child: U64, wrapParen: Bool)
  case BareAssert(facts: Stigi.Syntax.NodeAt)
  case SpineLeaf
