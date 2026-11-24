// prompt: test negationCycle rejection on a two-relation cycle through negation.
def Stigi.Datalog.Check.Tests.testNegCycle {region: Stigi.Region, Stigi.Mut} (relCount: U64) <lessOrEqual(relCount, 3)>: Bool =
  val zeroWord: U64 = 0
  val initFalse: Bool = false
  val varZero: Stigi.Datalog.Check.Term = VarTerm(0)
  val emptyAtom: Stigi.Datalog.Check.Atom = Stigi.Datalog.Check.Tests.makeAtomOne(0, varZero)
  val emptyLit: Stigi.Datalog.Check.Lit = PosLit(emptyAtom)
  val emptyRule: Stigi.Datalog.Check.RuleDef = RuleEntry(emptyAtom, 0, 0)
  Array.tryFill{region}(3, zeroWord) match
    case FillFailed => false
    case Filled(relArities) =>
      relArities(0) := 1
      relArities(1) := 1
      relArities(2) := 1
      Array.tryFill{region}(3, emptyLit) match
        case FillFailed => false
        case Filled(lits) =>
          lits(0) := PosLit(Stigi.Datalog.Check.Tests.makeAtomOne(1, varZero))
          lits(1) := NegLit(Stigi.Datalog.Check.Tests.makeAtomOne(2, varZero))
          lits(2) := PosLit(Stigi.Datalog.Check.Tests.makeAtomOne(0, varZero))
          Array.tryFill{region}(2, emptyRule) match
            case FillFailed => false
            case Filled(rules) =>
              rules(0) := RuleEntry(Stigi.Datalog.Check.Tests.makeAtomOne(0, varZero), 0, 2)
              rules(1) := RuleEntry(Stigi.Datalog.Check.Tests.makeAtomOne(2, varZero), 2, 3)
              Array.tryFill{region}(2, initFalse) match
                case FillFailed => false
                case Filled(bound) =>
                  Array.tryFill{region}(3, zeroWord) match
                    case FillFailed => false
                    case Filled(strata) =>
                      Stigi.Datalog.Check.stratifyProgram(3, relArities, relCount, 2, rules, 2, 3, lits, 3, 2, bound, strata) match
                        case NegationCycle(_) => true
                        case CheckPassed(_) => false
                        case UnresolvedRelation(_, _) => false
                        case ArityMismatch(_, _) => false
                        case RuleUnsafe(_, _) => false
