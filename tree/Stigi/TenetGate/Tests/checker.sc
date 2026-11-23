// prompt: verify tenet gate accepts valid prompted source and rejects missing prompt or forbidden words.
val Stigi.TenetGate.Tests.checker {Stigi.Test}: Stigi.Test.Result =
  val goodSrc: Array[U8](28) = "// prompt: clean unit.\ndef a"
  val missingPrompt: Array[U8](12) = "def clean = "
  val goodPass: Bool = Stigi.TenetGate.checkSource(28, goodSrc, 28) match
    case GateClean => true
    case GateMissingPrompt => false
    case GateBannedWord(_) => false
    case GateCrypticName(_) => false
    case GateTrailingDigit(_) => false
  val missPass: Bool = Stigi.TenetGate.checkSource(12, missingPrompt, 12) match
    case GateMissingPrompt => true
    case GateClean => false
    case GateBannedWord(_) => false
    case GateCrypticName(_) => false
    case GateTrailingDigit(_) => false
  val pairPass: Bool = Stigi.TenetGate.isAllowedPair(111, 102) && !Stigi.TenetGate.isAllowedPair(111, 107)
  Stigi.Test.expect(goodPass && missPass && pairPass, true, "tenet gate must enforce prompt header and allowlist")
