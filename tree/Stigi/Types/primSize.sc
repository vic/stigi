// prompt: map each primitive type kind to its exact byte size bounded by eight.
def Stigi.Types.primSize(kind: Stigi.Types.PrimKind): U64 <lessOrEqual(result, 8)> =
  case PrimUnit => 0
  case PrimBool => 1
  case PrimByte => 1
  case PrimWordShort => 4
  case PrimWordLong => 8
  case PrimIntShort => 4
  case PrimIntLong => 8
  case PrimHandle => 8
