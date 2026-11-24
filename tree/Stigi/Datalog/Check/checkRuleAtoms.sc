// prompt: verify relation declarations and arities for the head and all body atoms of one rule.
def Stigi.Datalog.Check.checkRuleAtoms(relCap: U64, relArities: Array[U64](relCap), relCount: U64, litCap: U64, lits: Array[Stigi.Datalog.Check.Lit](litCap), litCount: U64, ruleIndex: U64, entry: Stigi.Datalog.Check.RuleDef) <lessOrEqual(relCount, relCap), lessOrEqual(litCount, litCap)>: Stigi.Datalog.Check.Outcome =
  entry match
    case RuleEntry(head, litStart, litEnd) =>
      val headOut: Stigi.Datalog.Check.Outcome = Stigi.Datalog.Check.checkAtomRef(relCap, relArities, relCount, ruleIndex, head)
      if !Stigi.Datalog.Check.isPassed(headOut) then
        headOut
      else if litEnd <= litCount then
        var verdict: Stigi.Datalog.Check.Outcome = CheckPassed(0)
        var foundBad: Bool = false
        var litIdx: U64 = litStart
        while litIdx < litEnd && !foundBad do
          @<lessOrEqual(litStart, litIdx), lessOrEqual(litIdx, litEnd)>
          val nextLit: U64 = litIdx + 1
          val bodyLit: Stigi.Datalog.Check.Lit = lits(litIdx)
          verdict = bodyLit match
            case PosLit(bodyAtom) => Stigi.Datalog.Check.checkAtomRef(relCap, relArities, relCount, ruleIndex, bodyAtom)
            case NegLit(bodyAtom) => Stigi.Datalog.Check.checkAtomRef(relCap, relArities, relCount, ruleIndex, bodyAtom)
            case CmpLit(_, _, _) => CheckPassed(0)
            case ArithLit(_, _, _, _) => CheckPassed(0)
          foundBad = !Stigi.Datalog.Check.isPassed(verdict)
          litIdx = nextLit
        verdict
      else
        CheckPassed(0)
