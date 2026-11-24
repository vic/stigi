// prompt: whether the operand of a prefix operator needs parentheses; a negation under a negation takes them.
def Stigi.Syntax.Print.operandParens(shape: Stigi.Syntax.Print.Shape, outerNegates: Bool): Bool =
  shape match
    case ShapeBinary(_) | ShapeControl | ShapeIs => true
    case ShapeUnary(negates) => negates && outerNegates
    case ShapeStatement | ShapePlain => false
