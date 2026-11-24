// prompt: whether a callee, an index target or an asserted expression needs parentheses.
def Stigi.Syntax.Print.atomParens(shape: Stigi.Syntax.Print.Shape): Bool =
  case ShapeBinary(_) | ShapeUnary(_) | ShapeControl | ShapeStatement => true
  case ShapeIs | ShapePlain => false
