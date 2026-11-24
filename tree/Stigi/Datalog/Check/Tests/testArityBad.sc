// prompt: test relationArity rejection when an atom supplies wrong arity.
def Stigi.Datalog.Check.Tests.testArityBad {region: Stigi.Region, Stigi.Mut} (relCount: U64) <lessOrEqual(relCount, 2)>: Bool =
  val zeroWord: U64 = 0
  val initFalse: Bool = false
  val varZero: Stigi.Datalog.Check.Term = VarTerm(0)
  val varOne: Stigi.Datalog.Check.Term = VarTerm(1)
  val emptyAtom: Stigi.Datalog.Check.Atom = Stigi.Datalog.Check.Tests.makeAtomTwo(0, varZero, varOne)
  val emptyLit: Stigi.Datalog.Check.Lit = PosLit(emptyAtom)
  val emptyRule: Stigi.Datalog.Check.RuleDef = RuleEntry(emptyAtom, 0, 0)
  Array.tryFill{region}(2, zeroWord) match
    case FillFailed => false
    case Filled(relArities) =>
      relArities(0) := 2
      relArities(1) := 2
      Array.tryFill{region}(1, emptyLit) match
        case FillFailed => false
        case Filled(lits) =>
          lits(0) := PosLit(Stigi.Datalog.Check.Tests.makeAtomTwo(1, varZero, varOne))
          Array.tryFill{region}(1, emptyRule) match
            case FillFailed => false
            case Filled(rules) =>
              rules(0) := RuleEntry(Stigi.Datalog.Check.Tests.makeAtomThree(0, varZero, varOne, varZero), 0, 1)
              Array.tryFill{region}(2, initFalse) match
                case FillFailed => false
                case Filled(bound) =>
                  Array.tryFill{region}(2, zeroWord) match
                    case FillFailed => false
                    case Filled(strata) =>
                      Stigi.Datalog.Check.stratifyProgram(2, relArities, relCount, 1, rules, 1, 1, lits, 1, 2, bound, strata) match
                        case ArityMismatch(ruleIdx, relNum) => ruleIdx == 0 && relNum == 0
                        case CheckPassed(_) => false
                        case UnresolvedRelation(_, _) => false
                        case RuleUnsafe(_, _) => false
                        case NegationCycle(_) => false
