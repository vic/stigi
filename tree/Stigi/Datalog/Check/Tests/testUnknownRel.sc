// prompt: test unknownRelation rejection when a body atom names an undeclared relation.
def Stigi.Datalog.Check.Tests.testUnknownRel {region: Stigi.Region, Stigi.Mut} (relCount: U64) <lessOrEqual(relCount, 4)>: Bool =
  val zeroWord: U64 = 0
  val initFalse: Bool = false
  val varZero: Stigi.Datalog.Check.Term = VarTerm(0)
  val emptyAtom: Stigi.Datalog.Check.Atom = Stigi.Datalog.Check.Tests.makeAtomOne(0, varZero)
  val emptyLit: Stigi.Datalog.Check.Lit = PosLit(emptyAtom)
  val emptyRule: Stigi.Datalog.Check.RuleDef = RuleEntry(emptyAtom, 0, 0)
  Array.tryFill{region}(4, zeroWord) match
    case FillFailed => false
    case Filled(relArities) =>
      relArities(0) := 1
      relArities(1) := 1
      relArities(2) := 1
      relArities(3) := 1
      Array.tryFill{region}(1, emptyLit) match
        case FillFailed => false
        case Filled(lits) =>
          lits(0) := PosLit(Stigi.Datalog.Check.Tests.makeAtomOne(9, varZero))
          Array.tryFill{region}(1, emptyRule) match
            case FillFailed => false
            case Filled(rules) =>
              rules(0) := RuleEntry(Stigi.Datalog.Check.Tests.makeAtomOne(0, varZero), 0, 1)
              Array.tryFill{region}(2, initFalse) match
                case FillFailed => false
                case Filled(bound) =>
                  Array.tryFill{region}(4, zeroWord) match
                    case FillFailed => false
                    case Filled(strata) =>
                      Stigi.Datalog.Check.stratifyProgram(4, relArities, relCount, 1, rules, 1, 1, lits, 1, 2, bound, strata) match
                        case UnresolvedRelation(ruleIdx, relNum) => ruleIdx == 0 && relNum == 9
                        case CheckPassed(_) => false
                        case ArityMismatch(_, _) => false
                        case RuleUnsafe(_, _) => false
                        case NegationCycle(_) => false
