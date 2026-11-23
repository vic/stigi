// prompt: closed inferred expression type returned by Stigi.Emit.inferType without integer tags.
enum Stigi.Emit.InferredType:
  case InferBool
  case InferWord
  case InferWordShort
  case InferByte
  case InferSignedLong
  case InferSignedWord
  case InferEnum(enumSymIdx: U64, nameStart: U64, nameEnd: U64) <lessOrEqual(nameStart, nameEnd)>
  case InferNamed(nameStart: U64, nameEnd: U64) <lessOrEqual(nameStart, nameEnd)>
  case InferUnknown
