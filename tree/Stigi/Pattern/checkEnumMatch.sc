// prompt: verify enum match exhaustiveness, redundancy, guarded non-covering, and wildcard ban via Maranget matrix rules.
def Stigi.Pattern.checkEnumMatch {region: Stigi.Region, Stigi.Mut} (caseCount: U64, armCap: Stigi.TokenCap, arms: Array[Stigi.Pattern.ArmInfo](armCap), armCount: U64) <below(0, caseCount), below(caseCount, 4096), lessOrEqual(armCount, armCap)>: Stigi.Pattern.Verdict =
  val Filled(covered) = Array.tryFill[Bool]{region}(caseCount, false) else
    case FillFailed => PatternMissing(0)
  var armIdx: U64 = 0
  var verdict: Stigi.Pattern.Verdict = PatternSound
  var halted: Bool = false
  while armIdx < armCount && !halted do
    @<lessOrEqual(armIdx, armCount)>
    val ArmInfo(isWild, caseTag, hasGuard) = arms(armIdx)
    if isWild then
      verdict = PatternWildcardEnum(armIdx)
      halted = true
    else if caseTag < caseCount then
      if covered(caseTag) then
        verdict = PatternRedundant(armIdx)
        halted = true
      else if !hasGuard then
        covered(caseTag) := true
      else ()
    else ()
    armIdx = armIdx + 1
  if halted then verdict else
    var checkIdx: U64 = 0
    while checkIdx < caseCount && !halted do
      @<lessOrEqual(checkIdx, caseCount)>
      if !covered(checkIdx) then
        verdict = PatternMissing(checkIdx)
        halted = true
      else ()
      checkIdx = checkIdx + 1
    verdict
