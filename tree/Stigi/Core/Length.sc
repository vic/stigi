// prompt: a length term in normal form: a constant plus a span of linear parts in binder order.
pub enum Stigi.Core.Length:
  case Linear(constant: U64, parts: Stigi.Core.Span)
