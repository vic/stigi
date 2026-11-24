// prompt: how a unit declaration keyword starts its header.
enum Stigi.Parse.DeclLead:
  case PlainDecl(kind: Stigi.Ast.DeclKind)
  case DefDecl(kind: Stigi.Ast.DeclKind)
  case ExternDecl
  case NotDecl
