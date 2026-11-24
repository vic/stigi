// prompt: whether a term of a fact needs parentheses.
def Stigi.Syntax.Print.termParens(shape: Stigi.Syntax.Print.Shape): Bool =
  case ShapeBinary(_) | ShapeControl => true
  case ShapeUnary(_) | ShapeIs | ShapeStatement | ShapePlain => false
