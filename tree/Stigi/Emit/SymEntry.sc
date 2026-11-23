// prompt: single-case indexed declaration record in the unified emitter symbol table.
enum Stigi.Emit.SymEntry:
  case SymEntry(kind: Stigi.Emit.SymKind, ownerIdx: U64, nameStart: U64, nameEnd: U64, auxStart: U64, auxEnd: U64, subStart: U64, subCount: U64, bodyStart: U64, bodyEnd: U64) <lessOrEqual(nameStart, nameEnd), lessOrEqual(auxStart, auxEnd), lessOrEqual(bodyStart, bodyEnd)>
