// prompt: the outcome of reading the declaration of a parsed unit: the declaration node, or that the root or the declaration is missing.
enum Stigi.Names.DeclRead:
  case DeclFound(decl: Stigi.Syntax.Node)
  case DeclMissing
