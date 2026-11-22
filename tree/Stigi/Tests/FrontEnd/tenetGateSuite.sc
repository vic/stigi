// prompt: test tenet gate prompt check, case folding, short subword whitelist, and disallowed words.
val Stigi.Tests.FrontEnd.tenetGateSuite {Stigi.Test}: Stigi.Test.Result =
  val lowerValid: Bool = Stigi.TenetGate.toLowerByte(77) == 109
  val cleanName: Array[U8](14) = "readBlockBytes"
  val cleanOutcome: Stigi.TenetGate.Finding = Stigi.TenetGate.checkIdentifier(14, cleanName)
  val isClean: Bool = cleanOutcome match
    case FindingClean => true
    case FindingPromptMissing => false
    case FindingCrypticName(_) => false
    case FindingForbiddenWord(_) => false
    case FindingTrailingDigit(_) => false
    case FindingTabIndent(_) => false
  val digitName: Array[U8](5) = "item9"
  val digitOutcome: Stigi.TenetGate.Finding = Stigi.TenetGate.checkIdentifier(5, digitName)
  val isTrailingDigit: Bool = digitOutcome match
    case FindingTrailingDigit(off) => off == 4
    case FindingClean => false
    case FindingPromptMissing => false
    case FindingCrypticName(_) => false
    case FindingForbiddenWord(_) => false
    case FindingTabIndent(_) => false
  val shortAllowed: Array[U8](10) = "countOfRow"
  val shortOutcome: Stigi.TenetGate.Finding = Stigi.TenetGate.checkIdentifier(10, shortAllowed)
  val isShortClean: Bool = shortOutcome match
    case FindingClean => true
    case FindingPromptMissing => false
    case FindingCrypticName(_) => false
    case FindingForbiddenWord(_) => false
    case FindingTrailingDigit(_) => false
    case FindingTabIndent(_) => false
  val validUnit: Array[U8](28) = "// prompt: sum two numbers\n\n"
  val promptValid: Bool = Stigi.TenetGate.hasPromptLine(28, validUnit)
  val unitFinding: Stigi.TenetGate.Finding = Stigi.TenetGate.checkUnitSource(28, validUnit)
  val unitClean: Bool = unitFinding match
    case FindingClean => true
    case FindingPromptMissing => false
    case FindingCrypticName(_) => false
    case FindingForbiddenWord(_) => false
    case FindingTrailingDigit(_) => false
    case FindingTabIndent(_) => false
  val allPassed: Bool = lowerValid && isClean && isTrailingDigit && isShortClean && promptValid && unitClean
  Stigi.Test.expect(allPassed, true, "tenet gate checks case folding, naming rules, prompt line, and tabs")
