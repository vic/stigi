// prompt: flat arena AST node storing expression kind, token index, and child indices.
pub enum Stigi.Ast.Node:
  case Node(kind: Stigi.Ast.ExprKind, tokIndex: U64, leftChild: U64, rightChild: U64, extraChild: U64, extraFlags: U64)
