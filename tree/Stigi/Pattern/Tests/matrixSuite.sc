// prompt: verify Maranget pattern exhaustiveness, guarded non-covering, redundancy, and wildcard rejection.
val Stigi.Pattern.Tests.matrixSuite {Stigi.Test}: Stigi.Test.Result =
  val passed: Bool =
    val Filled(arms) = Array.tryFill[Stigi.Pattern.ArmInfo]{Stigi.Region}(4, ArmInfo(false, 0, false)) else
      case FillFailed => false
    arms(0) := ArmInfo(false, 0, false)
    arms(1) := ArmInfo(false, 1, false)
    val soundValid: Bool = Stigi.Pattern.checkEnumMatch{Stigi.Region}(2, 4, arms, 2) match
      case PatternSound => true
      case PatternMissing(_) => false
      case PatternRedundant(_) => false
      case PatternWildcardEnum(_) => false
    arms(1) := ArmInfo(false, 1, true)
    val guardMiss: Bool = Stigi.Pattern.checkEnumMatch{Stigi.Region}(2, 4, arms, 2) match
      case PatternMissing(missIdx) => missIdx == 1
      case PatternSound => false
      case PatternRedundant(_) => false
      case PatternWildcardEnum(_) => false
    arms(1) := ArmInfo(false, 0, false)
    val dupCaught: Bool = Stigi.Pattern.checkEnumMatch{Stigi.Region}(2, 4, arms, 2) match
      case PatternRedundant(dupIdx) => dupIdx == 1
      case PatternSound => false
      case PatternMissing(_) => false
      case PatternWildcardEnum(_) => false
    arms(1) := ArmInfo(true, 0, false)
    val wildCaught: Bool = Stigi.Pattern.checkEnumMatch{Stigi.Region}(2, 4, arms, 2) match
      case PatternWildcardEnum(wildIdx) => wildIdx == 1
      case PatternSound => false
      case PatternMissing(_) => false
      case PatternRedundant(_) => false
    soundValid && guardMiss && dupCaught && wildCaught
  Stigi.Test.expect(passed, true, "Maranget matrix checker must enforce exhaustiveness, guards, redundancy, and wildcard ban")
