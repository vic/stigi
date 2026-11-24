// prompt: the declaration kind and first token of a unit body.
enum Stigi.Parse.BodyStart:
  case BodyAt(kind: Stigi.Ast.DeclKind, startTok: U64)
