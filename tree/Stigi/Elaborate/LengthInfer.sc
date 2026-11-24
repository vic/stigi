// prompt: represent the outcome of inferring an omitted Array of U8 literal length during elaboration.
enum Stigi.Elaborate.LengthInfer:
  case LengthInferred(byteCount: U64)
  case LengthExplicit
  case LengthInferInvalid
