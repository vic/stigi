// prompt: record a classified builtin call frame kind and sub-operation code.
pub enum Stigi.Emit.BuiltinTag:
  case BuiltinInfo(frameKind: U8, subKind: U8)
