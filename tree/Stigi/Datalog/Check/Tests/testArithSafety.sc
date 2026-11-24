// prompt: test safe and unsafe ArithLit variable binding in rule safety checking.
def Stigi.Datalog.Check.Tests.testArithSafety {region: Stigi.Region, Stigi.Mut} (relCount: U64) <lessOrEqual(relCount, 2)>: Bool =
  val zeroWord: U64 = 0
  val initFalse: Bool = false
  val varZero: Stigi.Datalog.Check.Term = VarTerm(0)
  val varOne: Stigi.Datalog.Check.Term = VarTerm(1)
  val varTwo: Stigi.Datalog.Check.Term = VarTerm(2)
  val emptyAtom: Stigi.Datalog.Check.Atom = Stigi.Datalog.Check.Tests.makeAtomOne(0, varZero)
  val emptyLit: Stigi.Datalog.Check.Lit = PosLit(emptyAtom)
  val emptyRule: Stigi.Datalog.Check.RuleDef = RuleEntry(emptyAtom, 0, 0)
  Array.tryFill{region}(2, zeroWord) match
    case FillFailed => false
    case Filled(relArities) =>
      relArities(0) := 1
      relArities(1) := 1
      Array.tryFill{region}(2, emptyLit) match
        case FillFailed => false
        case Filled(lits) =>
          lits(0) := PosLit(Stigi.Datalog.Check.Tests.makeAtomOne(1, varZero))
          lits(1) := ArithLit(ArithAdd, 1, varZero, ConstNum(1, 0))
          Array.tryFill{region}(1, emptyRule) match
            case FillFailed => false
            case Filled(rules) =>
              rules(0) := RuleEntry(Stigi.Datalog.Check.Tests.makeAtomOne(0, varOne), 0, 2)
              Array.tryFill{region}(4, initFalse) match
                case FillFailed => false
                case Filled(bound) =>
                  Array.tryFill{region}(2, zeroWord) match
                    case FillFailed => false
                    case Filled(strata) =>
                      val safePass: Bool = Stigi.Datalog.Check.stratifyProgram(2, relArities, relCount, 1, rules, 1, 2, lits, 2, 4, bound, strata) match
                        case CheckPassed(maxStratum) => maxStratum == 0
                        case UnresolvedRelation(_, _) => false
                        case ArityMismatch(_, _) => false
                        case RuleUnsafe(_, _) => false
                        case NegationCycle(_) => false
                      lits(1) := ArithLit(ArithAdd, 2, varZero, varOne)
                      rules(0) := RuleEntry(Stigi.Datalog.Check.Tests.makeAtomOne(0, varTwo), 0, 2)
                      val unsafePass: Bool = Stigi.Datalog.Check.stratifyProgram(2, relArities, relCount, 1, rules, 1, 2, lits, 2, 4, bound, strata) match
                        case RuleUnsafe(ruleIdx, varNum) => ruleIdx == 0 && varNum == 1
                        case CheckPassed(_) => false
                        case UnresolvedRelation(_, _) => false
                        case ArityMismatch(_, _) => false
                        case NegationCycle(_) => false
                      safePass && unsafePass
