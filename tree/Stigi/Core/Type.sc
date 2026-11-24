// prompt: one normal type of the checked IR: a primitive, a nominal unit applied to interned type arguments and length rows, or a type variable.
pub enum Stigi.Core.Type:
  case Primitive(kind: Stigi.Core.Primitive)
  case Named(unit: Stigi.Core.Reference, arguments: Stigi.Core.Span, lengths: Stigi.Core.Span)
  case Variable(level: U64)
