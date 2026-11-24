// prompt: verify range restriction safety for a single Datalog rule with fixed-point arithmetic binding.
def Stigi.Datalog.Check.checkRuleSafety {Stigi.Mut} (litCap: U64, lits: Array[Stigi.Datalog.Check.Lit](litCap), litCount: U64, varCap: U64, bound: Array[Bool](varCap), ruleIndex: U64, entry: Stigi.Datalog.Check.RuleDef) <below(0, varCap), lessOrEqual(litCount, litCap)>: Stigi.Datalog.Check.Outcome =
  var clearIdx: U64 = 0
  while clearIdx < varCap do
    @<lessOrEqual(clearIdx, varCap)>
    val nextClear: U64 = clearIdx + 1
    bound(clearIdx) := false
    clearIdx = nextClear
  entry match
    case RuleEntry(head, litStart, litEnd) =>
      if litEnd <= litCount then
        var markIdx: U64 = litStart
        while markIdx < litEnd do
          @<lessOrEqual(litStart, markIdx), lessOrEqual(markIdx, litEnd)>
          val nextMark: U64 = markIdx + 1
          val bodyLit: Stigi.Datalog.Check.Lit = lits(markIdx)
          bodyLit match
            case PosLit(bodyAtom) => Stigi.Datalog.Check.markAtomBound(varCap, bound, bodyAtom)
            case NegLit(_) => ()
            case CmpLit(_, _, _) => ()
            case ArithLit(_, _, _, _) => ()
          markIdx = nextMark
        var changed: Bool = true
        var passNum: U64 = 0
        while changed && passNum < litCount do
          @<lessOrEqual(passNum, litCount)>
          val nextPass: U64 = passNum + 1
          changed = false
          var arithIdx: U64 = litStart
          while arithIdx < litEnd do
            @<lessOrEqual(litStart, arithIdx), lessOrEqual(arithIdx, litEnd)>
            val nextArith: U64 = arithIdx + 1
            lits(arithIdx) match
              case ArithLit(_, dstVar, leftTerm, rightTerm) =>
                val leftGood: Bool = Stigi.Datalog.Check.isPassed(Stigi.Datalog.Check.checkTermBound(varCap, bound, ruleIndex, leftTerm))
                val rightGood: Bool = Stigi.Datalog.Check.isPassed(Stigi.Datalog.Check.checkTermBound(varCap, bound, ruleIndex, rightTerm))
                if leftGood && rightGood && dstVar < varCap then
                  if !bound(dstVar) then
                    bound(dstVar) := true
                    changed = true
                  else
                    ()
                else
                  ()
              case PosLit(_) => ()
              case NegLit(_) => ()
              case CmpLit(_, _, _) => ()
            arithIdx = nextArith
          passNum = nextPass
        var verdict: Stigi.Datalog.Check.Outcome = CheckPassed(0)
        var foundBad: Bool = false
        var checkIdx: U64 = litStart
        while checkIdx < litEnd && !foundBad do
          @<lessOrEqual(litStart, checkIdx), lessOrEqual(checkIdx, litEnd)>
          val nextCheck: U64 = checkIdx + 1
          val bodyLit: Stigi.Datalog.Check.Lit = lits(checkIdx)
          verdict = bodyLit match
            case PosLit(_) => CheckPassed(0)
            case NegLit(negAtom) => Stigi.Datalog.Check.checkAtomBound(varCap, bound, ruleIndex, negAtom)
            case CmpLit(_, leftTerm, rightTerm) =>
              val leftOut: Stigi.Datalog.Check.Outcome = Stigi.Datalog.Check.checkTermBound(varCap, bound, ruleIndex, leftTerm)
              if Stigi.Datalog.Check.isPassed(leftOut) then
                Stigi.Datalog.Check.checkTermBound(varCap, bound, ruleIndex, rightTerm)
              else
                leftOut
            case ArithLit(_, dstVar, leftTerm, rightTerm) =>
              val leftOut: Stigi.Datalog.Check.Outcome = Stigi.Datalog.Check.checkTermBound(varCap, bound, ruleIndex, leftTerm)
              if !Stigi.Datalog.Check.isPassed(leftOut) then
                leftOut
              else
                val rightOut: Stigi.Datalog.Check.Outcome = Stigi.Datalog.Check.checkTermBound(varCap, bound, ruleIndex, rightTerm)
                if !Stigi.Datalog.Check.isPassed(rightOut) then
                  rightOut
                else if dstVar < varCap then
                  CheckPassed(0)
                else
                  RuleUnsafe(ruleIndex, dstVar)
          foundBad = !Stigi.Datalog.Check.isPassed(verdict)
          checkIdx = nextCheck
        if foundBad then
          verdict
        else
          Stigi.Datalog.Check.checkAtomBound(varCap, bound, ruleIndex, head)
      else
        CheckPassed(0)
