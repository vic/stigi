// prompt: run real lemma.dl S144 transitive lemma-assumes rules on Stigi.Datalog.Engine.
def Stigi.Datalog.Kernel.runLemmaAssumes {region: Stigi.Region, Stigi.Mut} (assertTarget: U64, expectReject: Bool) <below(assertTarget, 3)>: Bool =
  val zeroWord: U64 = 0
  val initFalse: Bool = false
  val zeroCell: Stigi.Datalog.Store.Cell = SymCell(0)
  val zeroRow: Stigi.Datalog.Store.Tuple = Stigi.Datalog.Kernel.rowOne(0, zeroCell)
  val varZero: Stigi.Datalog.Check.Term = VarTerm(0)
  val varOne: Stigi.Datalog.Check.Term = VarTerm(1)
  val emptyAtom: Stigi.Datalog.Check.Atom = Stigi.Datalog.Kernel.atomOne(0, varZero)
  val emptyLit: Stigi.Datalog.Check.Lit = PosLit(emptyAtom)
  val emptyRule: Stigi.Datalog.Check.RuleDef = RuleEntry(emptyAtom, 0, 0)
  Array.tryFill{region}(8, zeroWord) match
    case FillFailed => false
    case Filled(relArities) =>
      Array.tryFill{region}(8, emptyRule) match
        case FillFailed => false
        case Filled(rules) =>
          Array.tryFill{region}(8, emptyLit) match
            case FillFailed => false
            case Filled(lits) =>
              Array.tryFill{region}(4, initFalse) match
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
                                      Array.tryFill{region}(4, initFalse) match
                                        case FillFailed => false
                                        case Filled(varBound) =>
                                          Array.tryFill{region}(4, zeroCell) match
                                            case FillFailed => false
                                            case Filled(varVals) =>
                                              Array.tryFill{region}(8, zeroWord) match
                                                case FillFailed => false
                                                case Filled(ranks) =>
                                                  Array.tryFill{region}(8, StoreVacant) match
                                                    case FillFailed => false
                                                    case Filled(iterSlot) =>
                                                      Array.tryFill{region}(8, zeroWord) match
                                                        case FillFailed => false
                                                        case Filled(iterMask) =>
                                                          relArities(0) := 1
                                                          relArities(1) := 2
                                                          relArities(2) := 2
                                                          relArities(3) := 1
                                                          relArities(4) := 1
                                                          relArities(5) := 1
                                                          relArities(6) := 1
                                                          lits(0) := PosLit(Stigi.Datalog.Kernel.atomOne(3, varZero))
                                                          lits(1) := PosLit(Stigi.Datalog.Kernel.atomTwo(2, varZero, varOne))
                                                          lits(2) := PosLit(Stigi.Datalog.Kernel.atomOne(4, varOne))
                                                          lits(3) := PosLit(Stigi.Datalog.Kernel.atomTwo(1, varZero, varOne))
                                                          lits(4) := PosLit(Stigi.Datalog.Kernel.atomOne(4, varZero))
                                                          lits(5) := PosLit(Stigi.Datalog.Kernel.atomOne(0, varZero))
                                                          lits(6) := PosLit(Stigi.Datalog.Kernel.atomOne(5, varZero))
                                                          rules(0) := RuleEntry(Stigi.Datalog.Kernel.atomOne(4, varZero), 0, 1)
                                                          rules(1) := RuleEntry(Stigi.Datalog.Kernel.atomOne(4, varZero), 1, 3)
                                                          rules(2) := RuleEntry(Stigi.Datalog.Kernel.atomOne(5, varOne), 3, 5)
                                                          rules(3) := RuleEntry(Stigi.Datalog.Kernel.atomOne(6, varZero), 5, 7)
                                                          val targetInst: Stigi.Datalog.Store.Cell = if assertTarget == 0 then SymCell(1) else if assertTarget == 1 then SymCell(2) else SymCell(3)
                                                          val seedZero: Stigi.Datalog.Store.TupleOutcome = Stigi.Datalog.Store.insertTuple(8, 32, 64, rows, tupleSlots, relHead, nextInRel, Stigi.Datalog.Kernel.rowOne(0, SymCell(0)), 0)
                                                          val seedOne: Stigi.Datalog.Store.TupleOutcome = Stigi.Datalog.Store.insertTuple(8, 32, 64, rows, tupleSlots, relHead, nextInRel, Stigi.Datalog.Kernel.rowTwo(1, SymCell(1), SymCell(0)), 1)
                                                          val seedTwo: Stigi.Datalog.Store.TupleOutcome = Stigi.Datalog.Store.insertTuple(8, 32, 64, rows, tupleSlots, relHead, nextInRel, Stigi.Datalog.Kernel.rowTwo(2, SymCell(1), SymCell(2)), 2)
                                                          val seedThree: Stigi.Datalog.Store.TupleOutcome = Stigi.Datalog.Store.insertTuple(8, 32, 64, rows, tupleSlots, relHead, nextInRel, Stigi.Datalog.Kernel.rowOne(3, targetInst), 3)
                                                          val loaded: Bool = seedZero is TupleAdded(_, _) && seedOne is TupleAdded(_, _) && seedTwo is TupleAdded(_, _) && seedThree is TupleAdded(_, _)
                                                          if !loaded then
                                                            false
                                                          else
                                                            Stigi.Datalog.Engine.runProgram(8, 32, 64, 8, 8, 4, 8, relArities, 7, rules, 4, lits, 7, bound, strata, rows, tupleSlots, relHead, nextInRel, varBound, varVals, ranks, iterSlot, iterMask, 4, 64) match
                                                              case EvalDone(totalRows, _) =>
                                                                if totalRows <= 32 then
                                                                  val rejected: Bool = Stigi.Datalog.Store.hasTuple(32, 64, rows, tupleSlots, Stigi.Datalog.Kernel.rowOne(6, SymCell(0)), totalRows)
                                                                  rejected == expectReject
                                                                else
                                                                  false
                                                              case EvalRefused(_) => false
                                                              case EvalBudgetExceeded(_) => false
                                                              case EvalTableFull => false
                                                              case EvalArithError => false
