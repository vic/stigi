// prompt: the declaration kind a header keyword starts, or no header.
enum Stigi.Parse.DeclStart:
  case Started(kind: Stigi.Ast.DeclKind)
  case NotStarted
