// prompt: whether a body of a declaration kind starting with a token kind is one L0 expression.
def Stigi.Parse.hasExprBody(decl: Stigi.Ast.DeclKind, first: Stigi.Lex.TokenKind): Bool =
  decl is DeclDef | DeclVal | DeclLemma | DeclTest && !(first is TokEnd | TokCase | TokAt)
