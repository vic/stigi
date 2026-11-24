// prompt: the hash of one primitive kind, distinct for every kind.
def Stigi.Types.primitiveHash(kind: Stigi.Core.Primitive): U64 =
  kind match
    case UnsignedByte => 1
    case UnsignedWord => 2
    case UnsignedLong => 3
    case SignedWord => 4
    case SignedLong => 5
    case Boolean => 6
    case UnitValue => 7
    case NeverValue => 8
