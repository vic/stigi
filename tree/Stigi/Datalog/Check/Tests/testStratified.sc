// prompt: test stratifiedOk with transitive reachability and stratified unreachability.
def Stigi.Datalog.Check.Tests.testStratified {region: Stigi.Region, Stigi.Mut} (relCount: U64) <lessOrEqual(relCount, 4)>: Bool =
  val zeroWord: U64 = 0
  val initFalse: Bool = false
  val varZero: Stigi.Datalog.Check.Term = VarTerm(0)
  val varOne: Stigi.Datalog.Check.Term = VarTerm(1)
  val varTwo: Stigi.Datalog.Check.Term = VarTerm(2)
  val emptyAtom: Stigi.Datalog.Check.Atom = Stigi.Datalog.Check.Tests.makeAtomOne(0, varZero)
  val emptyLit: Stigi.Datalog.Check.Lit = PosLit(emptyAtom)
  val emptyRule: Stigi.Datalog.Check.RuleDef = RuleEntry(emptyAtom, 0, 0)
  Array.tryFill{region}(4, zeroWord) match
    case FillFailed => false
    case Filled(relArities) =>
      relArities(0) := 2
      relArities(1) := 2
      relArities(2) := 1
      relArities(3) := 2
      Array.tryFill{region}(6, emptyLit) match
        case FillFailed => false
        case Filled(lits) =>
          lits(0) := PosLit(Stigi.Datalog.Check.Tests.makeAtomTwo(0, varZero, varOne))
          lits(1) := PosLit(Stigi.Datalog.Check.Tests.makeAtomTwo(1, varZero, varOne))
          lits(2) := PosLit(Stigi.Datalog.Check.Tests.makeAtomTwo(0, varOne, varTwo))
          lits(3) := PosLit(Stigi.Datalog.Check.Tests.makeAtomOne(2, varZero))
          lits(4) := PosLit(Stigi.Datalog.Check.Tests.makeAtomOne(2, varOne))
          lits(5) := NegLit(Stigi.Datalog.Check.Tests.makeAtomTwo(1, varZero, varOne))
          Array.tryFill{region}(3, emptyRule) match
            case FillFailed => false
            case Filled(rules) =>
              rules(0) := RuleEntry(Stigi.Datalog.Check.Tests.makeAtomTwo(1, varZero, varOne), 0, 1)
              rules(1) := RuleEntry(Stigi.Datalog.Check.Tests.makeAtomTwo(1, varZero, varTwo), 1, 3)
              rules(2) := RuleEntry(Stigi.Datalog.Check.Tests.makeAtomTwo(3, varZero, varOne), 3, 6)
              Array.tryFill{region}(4, initFalse) match
                case FillFailed => false
                case Filled(bound) =>
                  Array.tryFill{region}(4, zeroWord) match
                    case FillFailed => false
                    case Filled(strata) =>
                      val resOut: Stigi.Datalog.Check.Outcome = Stigi.Datalog.Check.stratifyProgram(4, relArities, relCount, 3, rules, 3, 6, lits, 6, 4, bound, strata)
                      val passed: Bool = resOut match
                        case CheckPassed(maxStratum) => maxStratum == 1
                        case UnresolvedRelation(_, _) => false
                        case ArityMismatch(_, _) => false
                        case RuleUnsafe(_, _) => false
                        case NegationCycle(_) => false
                      passed && strata(3) > strata(1)
