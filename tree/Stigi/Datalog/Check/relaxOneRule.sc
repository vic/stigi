// prompt: relax stratum lower bounds along all body literals of one Datalog rule.
def Stigi.Datalog.Check.relaxOneRule {Stigi.Mut} (relCap: U64, strata: Array[U64](relCap), relCount: U64, litCap: U64, lits: Array[Stigi.Datalog.Check.Lit](litCap), litCount: U64, entry: Stigi.Datalog.Check.RuleDef) <lessOrEqual(relCount, relCap), lessOrEqual(litCount, litCap)>: Stigi.Datalog.Check.Outcome =
  entry match
    case RuleEntry(head, litStart, litEnd) =>
      head match
        case AtomRef(headRel, _, _, _, _, _, _, _, _) =>
          if headRel < relCount && litEnd <= litCount then
            var verdict: Stigi.Datalog.Check.Outcome = CheckPassed(0)
            var foundCycle: Bool = false
            var litIdx: U64 = litStart
            while litIdx < litEnd && !foundCycle do
              @<lessOrEqual(litStart, litIdx), lessOrEqual(litIdx, litEnd)>
              val nextLit: U64 = litIdx + 1
              val bodyLit: Stigi.Datalog.Check.Lit = lits(litIdx)
              verdict = bodyLit match
                case PosLit(bodyAtom) =>
                  bodyAtom match
                    case AtomRef(bodyRel, _, _, _, _, _, _, _, _) =>
                      if bodyRel < relCount then
                        val bodyStratum: U64 = strata(bodyRel)
                        if strata(headRel) < bodyStratum then
                          strata(headRel) := bodyStratum
                        else
                          ()
                      else
                        ()
                  CheckPassed(0)
                case NegLit(bodyAtom) =>
                  bodyAtom match
                    case AtomRef(bodyRel, _, _, _, _, _, _, _, _) =>
                      if bodyRel < relCount then
                        val bodyStratum: U64 = strata(bodyRel)
                        if bodyStratum < relCount then
                          val needed: U64 = bodyStratum + 1
                          if strata(headRel) < needed then
                            strata(headRel) := needed
                          else
                            ()
                          if needed >= relCount then
                            NegationCycle(headRel)
                          else
                            CheckPassed(0)
                        else
                          NegationCycle(headRel)
                      else
                        CheckPassed(0)
                case CmpLit(_, _, _) => CheckPassed(0)
                case ArithLit(_, _, _, _) => CheckPassed(0)
              foundCycle = !Stigi.Datalog.Check.isPassed(verdict)
              litIdx = nextLit
            verdict
          else
            CheckPassed(0)
