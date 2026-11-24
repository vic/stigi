// prompt: the normal type of one referenced type unit: a type without facts is its right-hand side, a type with facts or an enum is nominal by its unit reference.
def Stigi.Types.normalize(declared: Stigi.Types.Declared): Stigi.Core.Type =
  declared match
    case AliasType(rightSide) => rightSide
    case FactType(unit, arguments, lengths, _) => Named(unit, arguments, lengths)
    case EnumType(unit, arguments, lengths) => Named(unit, arguments, lengths)
