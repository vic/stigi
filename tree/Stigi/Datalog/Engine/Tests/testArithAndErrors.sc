// prompt: test exact 128-bit arithmetic literal derivation, overflow refusal, and divide-by-zero refusal.
def Stigi.Datalog.Engine.Tests.testArithAndErrors {region: Stigi.Region, Stigi.Mut} (relCount: U64) <lessOrEqual(relCount, 4)>: Bool =
  val zeroWord: U64 = 0
  val initFalse: Bool = false
  val zeroCell: Stigi.Datalog.Store.Cell = SymCell(0)
  val zeroRow: Stigi.Datalog.Store.Tuple = Stigi.Datalog.Engine.Tests.makeRowOne(0, zeroCell)
  val varZero: Stigi.Datalog.Check.Term = VarTerm(0)
  val varOne: Stigi.Datalog.Check.Term = VarTerm(1)
  val varTwo: Stigi.Datalog.Check.Term = VarTerm(2)
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
                                                          relArities(1) := 3
                                                          relArities(2) := 1
                                                          lits(0) := PosLit(Stigi.Datalog.Engine.Tests.makeAtomOne(0, varZero))
                                                          lits(1) := PosLit(Stigi.Datalog.Engine.Tests.makeAtomOne(0, varOne))
                                                          lits(2) := ArithLit(ArithAdd, 2, varZero, varOne)
                                                          rules(0) := RuleEntry(Stigi.Datalog.Engine.Tests.makeAtomThree(1, varTwo, varZero, varOne), 0, 3)
                                                          val maxUnsignedCell: Stigi.Datalog.Store.Cell = NumCell(WideInt(18446744073709551615, 0))
                                                          val oneCell: Stigi.Datalog.Store.Cell = NumCell(WideInt(1, 0))
                                                          val twoPow64Cell: Stigi.Datalog.Store.Cell = NumCell(WideInt(0, 1))
                                                          val insOne: Stigi.Datalog.Store.TupleOutcome = Stigi.Datalog.Store.insertTuple(4, 16, 32, rows, tupleSlots, relHead, nextInRel, Stigi.Datalog.Engine.Tests.makeRowOne(0, maxUnsignedCell), 0)
                                                          val insTwo: Stigi.Datalog.Store.TupleOutcome = Stigi.Datalog.Store.insertTuple(4, 16, 32, rows, tupleSlots, relHead, nextInRel, Stigi.Datalog.Engine.Tests.makeRowOne(0, oneCell), 1)
                                                          val chkSum: Bool = if insOne is TupleAdded(_, _) && insTwo is TupleAdded(_, _) then
                                                            Stigi.Datalog.Engine.runProgram(4, 16, 32, 4, 8, 4, 4, relArities, relCount, rules, 1, lits, 3, bound, strata, rows, tupleSlots, relHead, nextInRel, varBound, varVals, ranks, iterSlot, iterMask, 2, 64) match
                                                              case EvalDone(totalRows, derivedRows) =>
                                                                val wantRow: Stigi.Datalog.Store.Tuple = Stigi.Datalog.Engine.Tests.makeRowThree(1, twoPow64Cell, maxUnsignedCell, oneCell)
                                                                if totalRows <= 16 then
                                                                  derivedRows == 4 && Stigi.Datalog.Store.hasTuple(16, 32, rows, tupleSlots, wantRow, totalRows)
                                                                else
                                                                  false
                                                              case EvalRefused(_) => false
                                                              case EvalBudgetExceeded(_) => false
                                                              case EvalTableFull => false
                                                              case EvalArithError => false
                                                          else
                                                            false
                                                          lits(0) := PosLit(Stigi.Datalog.Engine.Tests.makeAtomOne(0, varZero))
                                                          lits(1) := ArithLit(ArithAdd, 1, varZero, ConstNum(1, 0))
                                                          rules(0) := RuleEntry(Stigi.Datalog.Engine.Tests.makeAtomOne(2, varOne), 0, 2)
                                                          val maxWideCell: Stigi.Datalog.Store.Cell = NumCell(WideInt(18446744073709551615, 9223372036854775807))
                                                          val insMax: Stigi.Datalog.Store.TupleOutcome = Stigi.Datalog.Store.insertTuple(4, 16, 32, rows, tupleSlots, relHead, nextInRel, Stigi.Datalog.Engine.Tests.makeRowOne(0, maxWideCell), 6)
                                                          val chkOver: Bool = if insMax is TupleAdded(_, _) then
                                                            Stigi.Datalog.Engine.runProgram(4, 16, 32, 4, 8, 4, 4, relArities, relCount, rules, 1, lits, 2, bound, strata, rows, tupleSlots, relHead, nextInRel, varBound, varVals, ranks, iterSlot, iterMask, 7, 64) match
                                                              case EvalArithError => true
                                                              case EvalDone(_, _) => false
                                                              case EvalRefused(_) => false
                                                              case EvalBudgetExceeded(_) => false
                                                              case EvalTableFull => false
                                                          else
                                                            false
                                                          lits(1) := ArithLit(ArithDiv, 1, varZero, ConstNum(0, 0))
                                                          val chkDivZero: Bool = Stigi.Datalog.Engine.runProgram(4, 16, 32, 4, 8, 4, 4, relArities, relCount, rules, 1, lits, 2, bound, strata, rows, tupleSlots, relHead, nextInRel, varBound, varVals, ranks, iterSlot, iterMask, 7, 64) match
                                                            case EvalArithError => true
                                                            case EvalDone(_, _) => false
                                                            case EvalRefused(_) => false
                                                            case EvalBudgetExceeded(_) => false
                                                            case EvalTableFull => false
                                                          chkSum && chkOver && chkDivZero
