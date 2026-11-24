// prompt: run real bounds.dl and arith.dl M4 quotient decrease rules on Stigi.Datalog.Engine.
def Stigi.Datalog.Kernel.runBoundsAndQuotient {region: Stigi.Region, Stigi.Mut} (divValue: I64, expectReject: Bool) <lessOrEqual(1, divValue)>: Bool =
  val zeroWord: U64 = 0
  val initFalse: Bool = false
  val zeroCell: Stigi.Datalog.Store.Cell = SymCell(0)
  val zeroRow: Stigi.Datalog.Store.Tuple = Stigi.Datalog.Kernel.rowOne(0, zeroCell)
  val varZero: Stigi.Datalog.Check.Term = VarTerm(0)
  val varOne: Stigi.Datalog.Check.Term = VarTerm(1)
  val varTwo: Stigi.Datalog.Check.Term = VarTerm(2)
  val varThree: Stigi.Datalog.Check.Term = VarTerm(3)
  val varFour: Stigi.Datalog.Check.Term = VarTerm(4)
  val varFive: Stigi.Datalog.Check.Term = VarTerm(5)
  val emptyAtom: Stigi.Datalog.Check.Atom = Stigi.Datalog.Kernel.atomOne(0, varZero)
  val emptyLit: Stigi.Datalog.Check.Lit = PosLit(emptyAtom)
  val emptyRule: Stigi.Datalog.Check.RuleDef = RuleEntry(emptyAtom, 0, 0)
  Array.tryFill{region}(8, zeroWord) match
    case FillFailed => false
    case Filled(relArities) =>
      Array.tryFill{region}(8, emptyRule) match
        case FillFailed => false
        case Filled(rules) =>
          Array.tryFill{region}(16, emptyLit) match
            case FillFailed => false
            case Filled(lits) =>
              Array.tryFill{region}(8, initFalse) match
                case FillFailed => false
                case Filled(bound) =>
                  Array.tryFill{region}(8, zeroWord) match
                    case FillFailed => false
                    case Filled(strata) =>
                      Array.tryFill{region}(32, zeroRow) match
                        case FillFailed => false
                        case Filled(rows) =>
                          Array.tryFill{region}(64, StoreVacant) match
                            case FillFailed => false
                            case Filled(tupleSlots) =>
                              Array.tryFill{region}(8, StoreVacant) match
                                case FillFailed => false
                                case Filled(relHead) =>
                                  Array.tryFill{region}(32, StoreVacant) match
                                    case FillFailed => false
                                    case Filled(nextInRel) =>
                                      Array.tryFill{region}(8, initFalse) match
                                        case FillFailed => false
                                        case Filled(varBound) =>
                                          Array.tryFill{region}(8, zeroCell) match
                                            case FillFailed => false
                                            case Filled(varVals) =>
                                              Array.tryFill{region}(8, zeroWord) match
                                                case FillFailed => false
                                                case Filled(ranks) =>
                                                  Array.tryFill{region}(16, StoreVacant) match
                                                    case FillFailed => false
                                                    case Filled(iterSlot) =>
                                                      Array.tryFill{region}(16, zeroWord) match
                                                        case FillFailed => false
                                                        case Filled(iterMask) =>
                                                          relArities(0) := 3
                                                          relArities(1) := 3
                                                          relArities(2) := 3
                                                          relArities(3) := 4
                                                          relArities(4) := 3
                                                          relArities(5) := 5
                                                          relArities(6) := 2
                                                          relArities(7) := 2
                                                          lits(0) := PosLit(Stigi.Datalog.Kernel.atomThree(0, varZero, varOne, varTwo))
                                                          lits(1) := PosLit(Stigi.Datalog.Kernel.atomThree(2, varZero, varOne, varThree))
                                                          lits(2) := PosLit(Stigi.Datalog.Kernel.atomThree(1, varOne, varTwo, varThree))
                                                          lits(3) := PosLit(Stigi.Datalog.Kernel.atomFour(3, varZero, varOne, varTwo, varThree))
                                                          lits(4) := PosLit(Stigi.Datalog.Kernel.atomThree(4, varTwo, varFour, varThree))
                                                          lits(5) := CmpLit(CmpGreaterOrEqual, varFour, ConstNum(2, 0))
                                                          lits(6) := PosLit(Stigi.Datalog.Kernel.atomThree(4, varOne, varFive, varThree))
                                                          lits(7) := CmpLit(CmpGreaterOrEqual, varFive, ConstNum(1, 0))
                                                          lits(8) := PosLit(Stigi.Datalog.Kernel.atomFive(5, varZero, varOne, varTwo, varThree, varFour))
                                                          lits(9) := PosLit(Stigi.Datalog.Kernel.atomThree(2, varOne, varTwo, varThree))
                                                          lits(10) := PosLit(Stigi.Datalog.Kernel.atomFive(5, varZero, varOne, varTwo, varThree, varFour))
                                                          lits(11) := NegLit(Stigi.Datalog.Kernel.atomTwo(6, varZero, varFour))
                                                          rules(0) := RuleEntry(Stigi.Datalog.Kernel.atomThree(2, varZero, varOne, varTwo), 0, 1)
                                                          rules(1) := RuleEntry(Stigi.Datalog.Kernel.atomThree(2, varZero, varTwo, varThree), 1, 3)
                                                          rules(2) := RuleEntry(Stigi.Datalog.Kernel.atomThree(2, varZero, varOne, varThree), 3, 8)
                                                          rules(3) := RuleEntry(Stigi.Datalog.Kernel.atomTwo(6, varZero, varFour), 8, 10)
                                                          rules(4) := RuleEntry(Stigi.Datalog.Kernel.atomTwo(7, varZero, varFour), 10, 12)
                                                          val divLow: U64 = U64.ofI64(divValue)
                                                          val seedZero: Stigi.Datalog.Store.TupleOutcome = Stigi.Datalog.Store.insertTuple(8, 32, 64, rows, tupleSlots, relHead, nextInRel, Stigi.Datalog.Kernel.rowFour(3, SymCell(0), SymCell(1), SymCell(2), SymCell(3)), 0)
                                                          val seedOne: Stigi.Datalog.Store.TupleOutcome = Stigi.Datalog.Store.insertTuple(8, 32, 64, rows, tupleSlots, relHead, nextInRel, Stigi.Datalog.Kernel.rowThree(4, SymCell(1), NumCell(WideInt(1, 0)), SymCell(3)), 1)
                                                          val seedTwo: Stigi.Datalog.Store.TupleOutcome = Stigi.Datalog.Store.insertTuple(8, 32, 64, rows, tupleSlots, relHead, nextInRel, Stigi.Datalog.Kernel.rowThree(4, SymCell(2), NumCell(WideInt(divLow, 0)), SymCell(3)), 2)
                                                          val seedThree: Stigi.Datalog.Store.TupleOutcome = Stigi.Datalog.Store.insertTuple(8, 32, 64, rows, tupleSlots, relHead, nextInRel, Stigi.Datalog.Kernel.rowFive(5, SymCell(4), SymCell(0), SymCell(1), SymCell(3), SymCell(5)), 3)
                                                          val loaded: Bool = seedZero is TupleAdded(_, _) && seedOne is TupleAdded(_, _) && seedTwo is TupleAdded(_, _) && seedThree is TupleAdded(_, _)
                                                          if !loaded then
                                                            false
                                                          else
                                                            Stigi.Datalog.Engine.runProgram(8, 32, 64, 8, 16, 8, 8, relArities, 8, rules, 5, lits, 12, bound, strata, rows, tupleSlots, relHead, nextInRel, varBound, varVals, ranks, iterSlot, iterMask, 4, 64) match
                                                              case EvalDone(totalRows, _) =>
                                                                if totalRows <= 32 then
                                                                  val rejected: Bool = Stigi.Datalog.Store.hasTuple(32, 64, rows, tupleSlots, Stigi.Datalog.Kernel.rowTwo(7, SymCell(4), SymCell(5)), totalRows)
                                                                  rejected == expectReject
                                                                else
                                                                  false
                                                              case EvalRefused(_) => false
                                                              case EvalBudgetExceeded(_) => false
                                                              case EvalTableFull => false
                                                              case EvalArithError => false
