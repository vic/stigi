// prompt: result of walking an expression left spine iteratively to its leaf.
enum Stigi.Syntax.Print.SpineResult:
  case SpineChain(leaf: U64, steps: U64, pen: Stigi.Syntax.Print.Pen)
  case SpineBare(facts: Stigi.Syntax.NodeAt)
  case SpineBad(pen: Stigi.Syntax.Print.Pen)
