// prompt: whether the subject of an is test needs parentheses.
def Stigi.Syntax.Print.subjectParens(shape: Stigi.Syntax.Print.Shape) <below(0, Stigi.Limit.maxUnitNodes)>: Bool =
  case ShapeControl => true
  case ShapeBinary(inner) => inner < Stigi.Syntax.Print.precedence(Equals)
  case ShapeUnary(_) | ShapeIs | ShapeStatement | ShapePlain => false
