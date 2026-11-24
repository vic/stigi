// prompt: how the printer must parenthesize a child expression, from the kind of its node.
enum Stigi.Syntax.Print.Shape:
  case ShapeBinary(precedence: U64)
  case ShapeUnary(negates: Bool)
  case ShapeIs
  case ShapeControl
  case ShapeStatement
  case ShapePlain
