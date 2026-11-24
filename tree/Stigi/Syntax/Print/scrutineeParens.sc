// prompt: whether the scrutinee of a match needs parentheses.
def Stigi.Syntax.Print.scrutineeParens(shape: Stigi.Syntax.Print.Shape): Bool =
  case ShapeControl => true
  case ShapeBinary(_) | ShapeUnary(_) | ShapeIs | ShapeStatement | ShapePlain => false
