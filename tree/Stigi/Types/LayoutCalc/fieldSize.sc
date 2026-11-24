// prompt: return the byte size of an enum field descriptor.
def Stigi.Types.LayoutCalc.fieldSize(field: Stigi.Types.LayoutCalc.FieldKind): U64 <lessOrEqual(result, Stigi.Limit.maxSourceBytes)> =
  field match
    case FieldScalar(sizeBytes, _) => if sizeBytes <= Stigi.Limit.maxSourceBytes then sizeBytes else Stigi.Limit.maxSourceBytes
    case FieldPointer => 8
