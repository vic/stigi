// prompt: closed expression kind enum for flat arena-indexed AST nodes.
pub enum Stigi.Ast.ExprKind:
  case ExprInt
  case ExprBool
  case ExprStr
  case ExprChar
  case ExprIdent
  case ExprCall
  case ExprBin
  case ExprUnary
  case ExprPipe
  case ExprIf
  case ExprWhile
  case ExprMatch
  case ExprVal
  case ExprVar
  case ExprAssign
  case ExprIndexWrite
  case ExprHandle
  case ExprBlock
  case ExprAssert
  case ExprUnit
