// prompt: closed symbol table entry kind for the checked-IR C11 emitter.
enum Stigi.Emit.SymKind:
  case SymEnum
  case SymCase
  case SymCap
  case SymCapMethod
  case SymHandler
  case SymHandlerMethod
  case SymGiven
  case SymGivenMethod
  case SymFunc
  case SymGenericFunc
  case SymExternBody
  case SymExternAbi
  case SymTest
  case SymVal
  case SymTypeAlias
