// prompt: map each primitive type kind to its exact positive alignment bounded by eight.
def Stigi.Types.primAlign(kind: Stigi.Types.PrimKind): U64 <below(0, result), lessOrEqual(result, 8)> =
  case PrimUnit => 1
  case PrimBool => 1
  case PrimByte => 1
  case PrimWordShort => 4
  case PrimWordLong => 8
  case PrimIntShort => 4
  case PrimIntLong => 8
  case PrimHandle => 8
