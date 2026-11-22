// prompt: return the byte size of one primitive or handle type on the x86-64 Linux profile.
pub def Stigi.Types.primitiveSize(kind: Stigi.Types.TypeKind) <lessOrEqual(0, 1)>: U64 =
  kind match
    case TypeByte => 1
    case TypeWord => 4
    case TypeLong => 8
    case TypeSignedWord => 4
    case TypeSignedLong => 8
    case TypeBool => 1
    case TypeUnit => 0
    case TypeNever => 0
    case TypeArray => 8
    case TypeCell => 8
    case TypeEnum => 8
    case TypeNominal => 8
