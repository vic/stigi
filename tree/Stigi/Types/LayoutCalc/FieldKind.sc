// prompt: represent an enum field layout as either a scalar size/alignment or a nullable-eligible pointer.
pub enum Stigi.Types.LayoutCalc.FieldKind:
  case FieldScalar(sizeBytes: U64, alignBytes: U64) <below(0, alignBytes), lessOrEqual(alignBytes, 64)>
  case FieldPointer
