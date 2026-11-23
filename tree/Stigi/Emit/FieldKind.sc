// prompt: closed field and binding kind shared by enum cases, params, handler state, and locals.
enum Stigi.Emit.FieldKind:
  case FieldEnum
  case FieldParam
  case FieldCapParam
  case FieldStateVar
  case FieldLocal
  case FieldLocalArray
  case FieldLocalCap
