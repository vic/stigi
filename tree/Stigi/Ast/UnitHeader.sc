// prompt: parsed unit header pairing declaration kind, visibility, name byte span, and body range.
pub enum Stigi.Ast.UnitHeader:
  case UnitHeader(kind: Stigi.Ast.DeclKind, isPublic: Bool, nameStart: U64, nameEnd: U64, bodyStartTok: U64, bodyEndTok: U64, rootNode: U64) <lessOrEqual(nameStart, nameEnd), lessOrEqual(bodyStartTok, bodyEndTok)>
