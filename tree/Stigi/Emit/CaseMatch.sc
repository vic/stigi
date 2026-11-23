// prompt: record a resolved enum case lookup with its owning enum unit and zero-based local tag.
pub enum Stigi.Emit.CaseMatch:
  case CaseHit(found: Bool, caseIdx: U64, enumUnit: U64, localTag: U64)
