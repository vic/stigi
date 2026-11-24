// prompt: syntax construct kind for synthetic depth-boundary round-trip tests.
enum Stigi.Syntax.Print.Tests.NestKind:
  case NestType
  case NestPattern
  case NestPatOr
  case NestIf
  case NestParen
  case NestBinary
  case NestIs
  case NestCall
  case NestAssert
  case NestDeepFact
  case NestVarInvariant
  case NestLocalExtern
  case NestDeepCall
  case NestDeepHandle
  case NestTypeDecl
  case NestDefReturn
