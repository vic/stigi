// prompt: whether one side of a binary operator of a given precedence needs parentheses; the right side also takes them at equal precedence.
def Stigi.Syntax.Print.sideParens(shape: Stigi.Syntax.Print.Shape, outer: U64, rightSide: Bool) <below(0, Stigi.Limit.maxUnitNodes)>: Bool =
  shape match
    case ShapeControl => true
    case ShapeBinary(inner) => inner < outer || rightSide && inner == outer
    case ShapeIs => Stigi.Syntax.Print.precedence(Equals) < outer || rightSide && Stigi.Syntax.Print.precedence(Equals) == outer
    case ShapeUnary(_) | ShapeStatement | ShapePlain => false
