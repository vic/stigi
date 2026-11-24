// prompt: parsed unit header pairing declaration kind, visibility, name byte span, body range, and body root.
pub enum Stigi.Ast.UnitHeader:
  case UnitHeader(kind: Stigi.Ast.DeclKind, isPublic: Bool, nameStart: U64, nameEnd: U64, bodyStartTok: U64, bodyEndTok: U64, root: Stigi.Ast.ParseRoot) <lessOrEqual(nameStart, nameEnd), lessOrEqual(bodyStartTok, bodyEndTok)>
