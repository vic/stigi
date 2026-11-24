// prompt: how a token kind can start an expression or a statement.
enum Stigi.Syntax.Parse.Lead:
  case StartsLiteral(kind: Stigi.Syntax.LiteralKind)
  case StartsName
  case StartsParen
  case StartsPrefix(operator: Stigi.Syntax.UnaryOperator)
  case StartsIf
  case StartsWhile
  case StartsVal
  case StartsVar
  case StartsHandle
  case StartsAssert
  case StartsDecl
  case StartsNothing
