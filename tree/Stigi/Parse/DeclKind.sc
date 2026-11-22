// prompt: declaration kinds recognized by the L0 unit parser.
pub enum Stigi.Parse.DeclKind:
  case DeclDef
  case DeclLemma
  case DeclVal
  case DeclEnum
  case DeclType
  case DeclCapability
  case DeclHandler
  case DeclExtern
  case DeclRelation
  case DeclRule
  case DeclUnknown
