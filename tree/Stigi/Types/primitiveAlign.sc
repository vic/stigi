// prompt: return the alignment in bytes of one primitive or handle type on the x86-64 Linux profile.
pub def Stigi.Types.primitiveAlign(kind: Stigi.Types.TypeKind) <lessOrEqual(0, 1)>: U64 =
  kind match
    case TypeByte => 1
    case TypeWord => 4
    case TypeLong => 8
    case TypeSignedWord => 4
    case TypeSignedLong => 8
    case TypeBool => 1
    case TypeUnit => 1
    case TypeNever => 1
    case TypeArray => 8
    case TypeCell => 8
    case TypeEnum => 8
    case TypeNominal => 8
