// prompt: one pending right-side or postfix action at a step along an expression left spine.
enum Stigi.Syntax.Print.SpineAction:
  case EmitBinary(operator: Stigi.Syntax.BinaryOperator, right: Stigi.Syntax.NodeAt, wrapParen: Bool)
  case EmitIs(pattern: Stigi.Syntax.NodeAt, wrapParen: Bool)
  case EmitCall(typeArgs: Stigi.Syntax.Span, capArgs: Stigi.Syntax.Span, args: Stigi.Syntax.Span, parens: Bool, wrapParen: Bool)
  case EmitIndex(index: Stigi.Syntax.NodeAt, wrapParen: Bool)
  case EmitAssert(facts: Stigi.Syntax.NodeAt, wrapParen: Bool)
  case MissingSpine(badSlot: U64)
