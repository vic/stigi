// prompt: single-case metadata record describing return type and monomorphization context for one function body.
enum Stigi.Emit.BodyMeta:
  case BodyMeta(retBeg: U64, retFin: U64, monoBeg: U64, monoFin: U64, isTest: Bool, isHandler: Bool, ownerIdx: U64) <lessOrEqual(retBeg, retFin), lessOrEqual(monoBeg, monoFin)>
