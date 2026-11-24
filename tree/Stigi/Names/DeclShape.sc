// prompt: what a declaration node says about its unit: its kind and pub flag, a val whose capability group decides its kind, or nothing.
enum Stigi.Names.DeclShape:
  case Declared(kind: Stigi.Names.UnitKind, isPublic: Bool)
  case ValueDecl(isPublic: Bool, caps: Stigi.Syntax.Span)
  case NotDeclaration
