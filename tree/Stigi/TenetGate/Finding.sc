// prompt: finding returned by the tenet gate on identifiers and unit text.
pub enum Stigi.TenetGate.Finding:
  case FindingClean
  case FindingPromptMissing
  case FindingCrypticName(offset: U64)
  case FindingForbiddenWord(offset: U64)
  case FindingTrailingDigit(offset: U64)
  case FindingTabIndent(offset: U64)
