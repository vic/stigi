// prompt: run real ratchet.dl S143 P1.2 and V6_pre spec-side freeze rules on Stigi.Datalog.Engine.
def Stigi.Datalog.Kernel.runRatchet {region: Stigi.Region, Stigi.Mut} (isFrame: Bool, actionKind: U64, expectReject: Bool) <below(actionKind, 4)>: Bool =
  val zeroWord: U64 = 0
  val initFalse: Bool = false
  val zeroCell: Stigi.Datalog.Store.Cell = SymCell(0)
  val zeroRow: Stigi.Datalog.Store.Tuple = Stigi.Datalog.Kernel.rowOne(0, zeroCell)
  val varZero: Stigi.Datalog.Check.Term = VarTerm(0)
  val varOne: Stigi.Datalog.Check.Term = VarTerm(1)
  val emptyAtom: Stigi.Datalog.Check.Atom = Stigi.Datalog.Kernel.atomOne(0, varZero)
  val emptyLit: Stigi.Datalog.Check.Lit = PosLit(emptyAtom)
  val emptyRule: Stigi.Datalog.Check.RuleDef = RuleEntry(emptyAtom, 0, 0)
  Array.tryFill{region}(12, zeroWord) match
    case FillFailed => false
    case Filled(relArities) =>
      Array.tryFill{region}(8, emptyRule) match
        case FillFailed => false
        case Filled(rules) =>
          Array.tryFill{region}(16, emptyLit) match
            case FillFailed => false
            case Filled(lits) =>
              Array.tryFill{region}(4, initFalse) match
                case FillFailed => false
                case Filled(bound) =>
                  Array.tryFill{region}(12, zeroWord) match
                    case FillFailed => false
                    case Filled(strata) =>
                      Array.tryFill{region}(32, zeroRow) match
                        case FillFailed => false
                        case Filled(rows) =>
                          Array.tryFill{region}(64, StoreVacant) match
                            case FillFailed => false
                            case Filled(tupleSlots) =>
                              Array.tryFill{region}(12, StoreVacant) match
                                case FillFailed => false
                                case Filled(relHead) =>
                                  Array.tryFill{region}(32, StoreVacant) match
                                    case FillFailed => false
                                    case Filled(nextInRel) =>
                                      Array.tryFill{region}(4, initFalse) match
                                        case FillFailed => false
                                        case Filled(varBound) =>
                                          Array.tryFill{region}(4, zeroCell) match
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
                                                          relArities(0) := 1
                                                          relArities(1) := 1
                                                          relArities(2) := 1
                                                          relArities(3) := 1
                                                          relArities(4) := 2
                                                          relArities(5) := 1
                                                          relArities(6) := 1
                                                          relArities(7) := 1
                                                          relArities(8) := 1
                                                          relArities(9) := 1
                                                          relArities(10) := 1
                                                          relArities(11) := 1
                                                          lits(0) := PosLit(Stigi.Datalog.Kernel.atomOne(2, varZero))
                                                          lits(1) := PosLit(Stigi.Datalog.Kernel.atomOne(5, varZero))
                                                          lits(2) := PosLit(Stigi.Datalog.Kernel.atomTwo(4, varZero, varOne))
                                                          lits(3) := PosLit(Stigi.Datalog.Kernel.atomOne(3, varZero))
                                                          lits(4) := PosLit(Stigi.Datalog.Kernel.atomOne(6, varZero))
                                                          lits(5) := PosLit(Stigi.Datalog.Kernel.atomTwo(4, varZero, varOne))
                                                          lits(6) := PosLit(Stigi.Datalog.Kernel.atomOne(5, varZero))
                                                          lits(7) := NegLit(Stigi.Datalog.Kernel.atomOne(6, varZero))
                                                          lits(8) := PosLit(Stigi.Datalog.Kernel.atomOne(0, ConstSym(1)))
                                                          lits(9) := PosLit(Stigi.Datalog.Kernel.atomOne(7, varZero))
                                                          lits(10) := PosLit(Stigi.Datalog.Kernel.atomOne(8, varZero))
                                                          lits(11) := PosLit(Stigi.Datalog.Kernel.atomOne(0, ConstSym(1)))
                                                          lits(12) := PosLit(Stigi.Datalog.Kernel.atomOne(7, varZero))
                                                          lits(13) := PosLit(Stigi.Datalog.Kernel.atomOne(9, varZero))
                                                          lits(14) := PosLit(Stigi.Datalog.Kernel.atomOne(1, varZero))
                                                          lits(15) := PosLit(Stigi.Datalog.Kernel.atomOne(10, varZero))
                                                          rules(0) := RuleEntry(Stigi.Datalog.Kernel.atomOne(5, varZero), 0, 1)
                                                          rules(1) := RuleEntry(Stigi.Datalog.Kernel.atomOne(5, varOne), 1, 3)
                                                          rules(2) := RuleEntry(Stigi.Datalog.Kernel.atomOne(6, varZero), 3, 4)
                                                          rules(3) := RuleEntry(Stigi.Datalog.Kernel.atomOne(6, varOne), 4, 6)
                                                          rules(4) := RuleEntry(Stigi.Datalog.Kernel.atomOne(7, varZero), 6, 8)
                                                          rules(5) := RuleEntry(Stigi.Datalog.Kernel.atomOne(10, varZero), 8, 11)
                                                          rules(6) := RuleEntry(Stigi.Datalog.Kernel.atomOne(10, varZero), 11, 14)
                                                          rules(7) := RuleEntry(Stigi.Datalog.Kernel.atomOne(11, varZero), 14, 16)
                                                          val txnCell: Stigi.Datalog.Store.Cell = if isFrame then SymCell(1) else SymCell(0)
                                                          val seedZero: Stigi.Datalog.Store.TupleOutcome = Stigi.Datalog.Store.insertTuple(12, 32, 64, rows, tupleSlots, relHead, nextInRel, Stigi.Datalog.Kernel.rowOne(0, txnCell), 0)
                                                          val seedOne: Stigi.Datalog.Store.TupleOutcome = Stigi.Datalog.Store.insertTuple(12, 32, 64, rows, tupleSlots, relHead, nextInRel, Stigi.Datalog.Kernel.rowOne(1, SymCell(2)), 1)
                                                          val seedTwo: Stigi.Datalog.Store.TupleOutcome = Stigi.Datalog.Store.insertTuple(12, 32, 64, rows, tupleSlots, relHead, nextInRel, Stigi.Datalog.Kernel.rowOne(1, SymCell(3)), 2)
                                                          val seedThree: Stigi.Datalog.Store.TupleOutcome = Stigi.Datalog.Store.insertTuple(12, 32, 64, rows, tupleSlots, relHead, nextInRel, Stigi.Datalog.Kernel.rowOne(1, SymCell(4)), 3)
                                                          val seedFour: Stigi.Datalog.Store.TupleOutcome = Stigi.Datalog.Store.insertTuple(12, 32, 64, rows, tupleSlots, relHead, nextInRel, Stigi.Datalog.Kernel.rowOne(2, SymCell(2)), 4)
                                                          val seedFive: Stigi.Datalog.Store.TupleOutcome = Stigi.Datalog.Store.insertTuple(12, 32, 64, rows, tupleSlots, relHead, nextInRel, Stigi.Datalog.Kernel.rowOne(3, SymCell(3)), 5)
                                                          val seedSix: Stigi.Datalog.Store.TupleOutcome = Stigi.Datalog.Store.insertTuple(12, 32, 64, rows, tupleSlots, relHead, nextInRel, Stigi.Datalog.Kernel.rowTwo(4, SymCell(2), SymCell(4)), 6)
                                                          val actionRow: Stigi.Datalog.Store.Tuple = if actionKind == 2 then Stigi.Datalog.Kernel.rowOne(9, SymCell(4)) else Stigi.Datalog.Kernel.rowOne(8, SymCell(4))
                                                          val seedSeven: Stigi.Datalog.Store.TupleOutcome = Stigi.Datalog.Store.insertTuple(12, 32, 64, rows, tupleSlots, relHead, nextInRel, actionRow, 7)
                                                          val extraRow: Stigi.Datalog.Store.Tuple = if actionKind == 3 then Stigi.Datalog.Kernel.rowTwo(4, SymCell(3), SymCell(4)) else Stigi.Datalog.Kernel.rowOne(1, SymCell(5))
                                                          val seedEight: Stigi.Datalog.Store.TupleOutcome = Stigi.Datalog.Store.insertTuple(12, 32, 64, rows, tupleSlots, relHead, nextInRel, extraRow, 8)
                                                          val loadedOne: Bool = seedZero is TupleAdded(_, _) && seedOne is TupleAdded(_, _) && seedTwo is TupleAdded(_, _) && seedThree is TupleAdded(_, _) && seedFour is TupleAdded(_, _)
                                                          val loadedTwo: Bool = seedFive is TupleAdded(_, _) && seedSix is TupleAdded(_, _) && seedSeven is TupleAdded(_, _) && seedEight is TupleAdded(_, _)
                                                          if !(loadedOne && loadedTwo) then
                                                            false
                                                          else
                                                            Stigi.Datalog.Engine.runProgram(12, 32, 64, 8, 16, 4, 8, relArities, 12, rules, 8, lits, 16, bound, strata, rows, tupleSlots, relHead, nextInRel, varBound, varVals, ranks, iterSlot, iterMask, 9, 64) match
                                                              case EvalDone(totalRows, _) =>
                                                                if totalRows <= 32 then
                                                                  val rejected: Bool = Stigi.Datalog.Store.hasTuple(32, 64, rows, tupleSlots, Stigi.Datalog.Kernel.rowOne(11, SymCell(4)), totalRows)
                                                                  rejected == expectReject
                                                                else
                                                                  false
                                                              case EvalRefused(_) => false
                                                              case EvalBudgetExceeded(_) => false
                                                              case EvalTableFull => false
                                                              case EvalArithError => false
