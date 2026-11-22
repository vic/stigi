// prompt: outcome of parsing a unit header and its declared dotted name.
pub enum Stigi.Parse.UnitHeaderOutcome:
  case UnitHeader(kind: Stigi.Parse.DeclKind, isPublic: Bool, nameStart: U64, nameEnd: U64)
  case UnitHeaderError(offset: U64, reasonCode: U64)
