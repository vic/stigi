// prompt: check relation references, rule safety, and Bellman-Ford stratification for a Datalog program.
pub def Stigi.Datalog.Check.stratifyProgram {Stigi.Mut} (relCap: U64, relArities: Array[U64](relCap), relCount: U64, ruleCap: U64, rules: Array[Stigi.Datalog.Check.RuleDef](ruleCap), ruleCount: U64, litCap: U64, lits: Array[Stigi.Datalog.Check.Lit](litCap), litCount: U64, varCap: U64, bound: Array[Bool](varCap), strata: Array[U64](relCap)) <below(0, varCap), lessOrEqual(relCount, relCap), lessOrEqual(ruleCount, ruleCap), lessOrEqual(litCount, litCap)>: Stigi.Datalog.Check.Outcome =
  var initIdx: U64 = 0
  while initIdx < relCount do
    @<lessOrEqual(initIdx, relCount)>
    val nextInit: U64 = initIdx + 1
    strata(initIdx) := 0
    initIdx = nextInit
  var verdict: Stigi.Datalog.Check.Outcome = CheckPassed(0)
  var foundBad: Bool = false
  var ruleIdx: U64 = 0
  while ruleIdx < ruleCount && !foundBad do
    @<lessOrEqual(ruleIdx, ruleCount)>
    val nextRule: U64 = ruleIdx + 1
    val curRule: Stigi.Datalog.Check.RuleDef = rules(ruleIdx)
    val atomOut: Stigi.Datalog.Check.Outcome = Stigi.Datalog.Check.checkRuleAtoms(relCap, relArities, relCount, litCap, lits, litCount, ruleIdx, curRule)
    if Stigi.Datalog.Check.isPassed(atomOut) then
      verdict = Stigi.Datalog.Check.checkRuleSafety(litCap, lits, litCount, varCap, bound, ruleIdx, curRule)
    else
      verdict = atomOut
    foundBad = !Stigi.Datalog.Check.isPassed(verdict)
    ruleIdx = nextRule
  if foundBad then
    verdict
  else
    var passIdx: U64 = 0
    while passIdx < relCount && !foundBad do
      @<lessOrEqual(passIdx, relCount)>
      val nextPass: U64 = passIdx + 1
      var relaxIdx: U64 = 0
      while relaxIdx < ruleCount && !foundBad do
        @<lessOrEqual(relaxIdx, ruleCount)>
        val nextRelax: U64 = relaxIdx + 1
        val curRule: Stigi.Datalog.Check.RuleDef = rules(relaxIdx)
        verdict = Stigi.Datalog.Check.relaxOneRule(relCap, strata, relCount, litCap, lits, litCount, curRule)
        foundBad = !Stigi.Datalog.Check.isPassed(verdict)
        relaxIdx = nextRelax
      passIdx = nextPass
    if foundBad then
      verdict
    else
      var maxStratum: U64 = 0
      var scanIdx: U64 = 0
      while scanIdx < relCount && !foundBad do
        @<lessOrEqual(scanIdx, relCount)>
        val nextScan: U64 = scanIdx + 1
        val curStratum: U64 = strata(scanIdx)
        if curStratum >= relCount then
          verdict = NegationCycle(scanIdx)
          foundBad = true
        else if curStratum > maxStratum then
          maxStratum = curStratum
        else
          ()
        scanIdx = nextScan
      if foundBad then
        verdict
      else
        CheckPassed(maxStratum)
