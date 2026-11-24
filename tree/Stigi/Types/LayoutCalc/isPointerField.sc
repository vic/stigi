// prompt: return true when a field descriptor is a nullable-eligible Array or Cell pointer.
def Stigi.Types.LayoutCalc.isPointerField(field: Stigi.Types.LayoutCalc.FieldKind): Bool =
  field match
    case FieldPointer => true
    case FieldScalar(_, _) => false
