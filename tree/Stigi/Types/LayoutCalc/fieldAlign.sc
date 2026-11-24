// prompt: return the proven alignment of an enum field descriptor.
def Stigi.Types.LayoutCalc.fieldAlign(field: Stigi.Types.LayoutCalc.FieldKind): U64 <below(0, result), lessOrEqual(result, 64)> =
  field match
    case FieldScalar(_, alignBytes) => alignBytes
    case FieldPointer => 8
