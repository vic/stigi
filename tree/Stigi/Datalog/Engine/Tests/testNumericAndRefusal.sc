// prompt: test exact 128-bit numeric comparison filtering and O2 refusal pass-through.
def Stigi.Datalog.Engine.Tests.testNumericAndRefusal {region: Stigi.Region, Stigi.Mut} (relCount: U64) <lessOrEqual(relCount, 4)>: Bool =
  val zeroWord: U64 = 0
  val initFalse: Bool = false
  val zeroCell: Stigi.Datalog.Store.Cell = SymCell(0)
  val zeroRow: Stigi.Datalog.Store.Tuple = Stigi.Datalog.Engine.Tests.makeRowOne(0, zeroCell)
  val varZero: Stigi.Datalog.Check.Term = VarTerm(0)
  val emptyAtom: Stigi.Datalog.Check.Atom = Stigi.Datalog.Engine.Tests.makeAtomOne(0, varZero)
  val emptyLit: Stigi.Datalog.Check.Lit = PosLit(emptyAtom)
  val emptyRule: Stigi.Datalog.Check.RuleDef = RuleEntry(emptyAtom, 0, 0)
  Array.tryFill{region}(4, zeroWord) match
    case FillFailed => false
    case Filled(relArities) =>
      Array.tryFill{region}(4, emptyRule) match
        case FillFailed => false
        case Filled(rules) =>
          Array.tryFill{region}(8, emptyLit) match
            case FillFailed => false
            case Filled(lits) =>
              Array.tryFill{region}(4, initFalse) match
                case FillFailed => false
                case Filled(bound) =>
                  Array.tryFill{region}(4, zeroWord) match
                    case FillFailed => false
                    case Filled(strata) =>
                      Array.tryFill{region}(16, zeroRow) match
                        case FillFailed => false
                        case Filled(rows) =>
                          Array.tryFill{region}(32, StoreVacant) match
                            case FillFailed => false
                            case Filled(tupleSlots) =>
                              Array.tryFill{region}(4, StoreVacant) match
                                case FillFailed => false
                                case Filled(relHead) =>
                                  Array.tryFill{region}(16, StoreVacant) match
                                    case FillFailed => false
                                    case Filled(nextInRel) =>
                                      Array.tryFill{region}(4, initFalse) match
                                        case FillFailed => false
                                        case Filled(varBound) =>
                                          Array.tryFill{region}(4, zeroCell) match
                                            case FillFailed => false
                                            case Filled(varVals) =>
                                              Array.tryFill{region}(4, zeroWord) match
                                                case FillFailed => false
                                                case Filled(ranks) =>
                                                  Array.tryFill{region}(8, StoreVacant) match
                                                    case FillFailed => false
                                                    case Filled(iterSlot) =>
                                                      Array.tryFill{region}(8, zeroWord) match
                                                        case FillFailed => false
                                                        case Filled(iterMask) =>
                                                          relArities(0) := 1
                                                          relArities(1) := 1
                                                          lits(0) := PosLit(Stigi.Datalog.Engine.Tests.makeAtomOne(0, varZero))
                                                          lits(1) := CmpLit(CmpGreater, varZero, ConstNum(0, 0))
                                                          rules(0) := RuleEntry(Stigi.Datalog.Engine.Tests.makeAtomOne(1, varZero), 0, 2)
                                                          val bigRow: Stigi.Datalog.Store.Tuple = Stigi.Datalog.Engine.Tests.makeRowOne(0, NumCell(WideInt(0, 1)))
                                                          val negRow: Stigi.Datalog.Store.Tuple = Stigi.Datalog.Engine.Tests.makeRowOne(0, NumCell(WideInt(18446744073709551615, -1)))
                                                          val insOne: Stigi.Datalog.Store.TupleOutcome = Stigi.Datalog.Store.insertTuple(4, 16, 32, rows, tupleSlots, relHead, nextInRel, bigRow, 0)
                                                          val insTwo: Stigi.Datalog.Store.TupleOutcome = Stigi.Datalog.Store.insertTuple(4, 16, 32, rows, tupleSlots, relHead, nextInRel, negRow, 1)
                                                          val chkNum: Bool = if insOne is TupleAdded(_, _) && insTwo is TupleAdded(_, _) then
                                                            Stigi.Datalog.Engine.runProgram(4, 16, 32, 4, 8, 4, 4, relArities, relCount, rules, 1, lits, 2, bound, strata, rows, tupleSlots, relHead, nextInRel, varBound, varVals, ranks, iterSlot, iterMask, 2, 64) match
                                                              case EvalDone(totalRows, derivedRows) =>
                                                                val hasBig: Bool = Stigi.Datalog.Store.hasTuple(16, 32, rows, tupleSlots, Stigi.Datalog.Engine.Tests.makeRowOne(1, NumCell(WideInt(0, 1))), 3)
                                                                val absentNeg: Bool = !Stigi.Datalog.Store.hasTuple(16, 32, rows, tupleSlots, Stigi.Datalog.Engine.Tests.makeRowOne(1, NumCell(WideInt(18446744073709551615, -1))), 3)
                                                                totalRows == 3 && derivedRows == 1 && hasBig && absentNeg
                                                              case EvalRefused(_) => false
                                                              case EvalBudgetExceeded(_) => false
                                                              case EvalTableFull => false
                                                              case EvalArithError => false
                                                          else
                                                            false
                                                          lits(2) := PosLit(Stigi.Datalog.Engine.Tests.makeAtomOne(0, varZero))
                                                          lits(3) := NegLit(Stigi.Datalog.Engine.Tests.makeAtomOne(1, varZero))
                                                          rules(1) := RuleEntry(Stigi.Datalog.Engine.Tests.makeAtomOne(1, varZero), 2, 4)
                                                          val chkRefuse: Bool = Stigi.Datalog.Engine.runProgram(4, 16, 32, 4, 8, 4, 4, relArities, relCount, rules, 2, lits, 4, bound, strata, rows, tupleSlots, relHead, nextInRel, varBound, varVals, ranks, iterSlot, iterMask, 2, 64) match
                                                            case EvalRefused(reason) =>
                                                              reason match
                                                                case NegationCycle(_) => true
                                                                case CheckPassed(_) => false
                                                                case UnresolvedRelation(_, _) => false
                                                                case ArityMismatch(_, _) => false
                                                                case RuleUnsafe(_, _) => false
                                                            case EvalDone(_, _) => false
                                                            case EvalBudgetExceeded(_) => false
                                                            case EvalTableFull => false
                                                            case EvalArithError => false
                                                          chkNum && chkRefuse
