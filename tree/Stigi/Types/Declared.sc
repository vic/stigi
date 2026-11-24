// prompt: a referenced type unit as normalization sees it: a type without facts with its right-hand side, a type with facts applied to its arguments with its right-hand side, or an enum applied to its arguments.
enum Stigi.Types.Declared:
  case AliasType(rightSide: Stigi.Core.Type)
  case FactType(unit: Stigi.Core.Reference, arguments: Stigi.Core.Span, lengths: Stigi.Core.Span, rightSide: Stigi.Core.Type)
  case EnumType(unit: Stigi.Core.Reference, arguments: Stigi.Core.Span, lengths: Stigi.Core.Span)
