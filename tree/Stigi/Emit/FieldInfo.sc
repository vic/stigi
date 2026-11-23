// prompt: single-case typed field, parameter, or local variable binding record.
enum Stigi.Emit.FieldInfo:
  case FieldInfo(kind: Stigi.Emit.FieldKind, nameStart: U64, nameEnd: U64, typeStart: U64, typeEnd: U64, elemStart: U64, elemEnd: U64) <lessOrEqual(nameStart, nameEnd), lessOrEqual(typeStart, typeEnd), lessOrEqual(elemStart, elemEnd)>
