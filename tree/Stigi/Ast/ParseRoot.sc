// prompt: the root of a parsed unit body: the index of its expression node, or a body that is no expression.
pub enum Stigi.Ast.ParseRoot:
  case ExprRoot(nodeIdx: U64)
  case NonExprBody
