// prompt: run transitive closure and stratified negation in forward or reverse order with configurable budget.
def Stigi.Datalog.Engine.Tests.runReachGraph {region: Stigi.Region, Stigi.Mut} (reverseOrder: Bool, rowBudget: U64) <below(0, rowBudget)>: Bool =
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
                      Array.tryFill{region}(64, zeroRow) match
                        case FillFailed => false
                        case Filled(rows) =>
                          Array.tryFill{region}(128, StoreVacant) match
                            case FillFailed => false
                            case Filled(tupleSlots) =>
                              Array.tryFill{region}(8, StoreVacant) match
                                case FillFailed => false
                                case Filled(relHead) =>
                                  Array.tryFill{region}(64, StoreVacant) match
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
                                                          relArities(0) := 2
                                                          relArities(1) := 2
                                                          relArities(2) := 1
                                                          relArities(3) := 2
                                                          lits(0) := PosLit(Stigi.Datalog.Engine.Tests.makeAtomTwo(0, varZero, varOne))
                                                          lits(1) := PosLit(Stigi.Datalog.Engine.Tests.makeAtomTwo(1, varZero, varOne))
                                                          lits(2) := PosLit(Stigi.Datalog.Engine.Tests.makeAtomTwo(0, varOne, varTwo))
                                                          lits(3) := PosLit(Stigi.Datalog.Engine.Tests.makeAtomOne(2, varZero))
                                                          lits(4) := PosLit(Stigi.Datalog.Engine.Tests.makeAtomOne(2, varOne))
                                                          lits(5) := NegLit(Stigi.Datalog.Engine.Tests.makeAtomTwo(1, varZero, varOne))
                                                          if reverseOrder then
                                                            rules(0) := RuleEntry(Stigi.Datalog.Engine.Tests.makeAtomTwo(3, varZero, varOne), 3, 6)
                                                            rules(1) := RuleEntry(Stigi.Datalog.Engine.Tests.makeAtomTwo(1, varZero, varTwo), 1, 3)
                                                            rules(2) := RuleEntry(Stigi.Datalog.Engine.Tests.makeAtomTwo(1, varZero, varOne), 0, 1)
                                                          else
                                                            rules(0) := RuleEntry(Stigi.Datalog.Engine.Tests.makeAtomTwo(1, varZero, varOne), 0, 1)
                                                            rules(1) := RuleEntry(Stigi.Datalog.Engine.Tests.makeAtomTwo(1, varZero, varTwo), 1, 3)
                                                            rules(2) := RuleEntry(Stigi.Datalog.Engine.Tests.makeAtomTwo(3, varZero, varOne), 3, 6)
                                                          val rowZero: Stigi.Datalog.Store.Tuple = if reverseOrder then Stigi.Datalog.Engine.Tests.makeRowOne(2, SymCell(3)) else Stigi.Datalog.Engine.Tests.makeRowTwo(0, SymCell(0), SymCell(1))
                                                          val rowOne: Stigi.Datalog.Store.Tuple = if reverseOrder then Stigi.Datalog.Engine.Tests.makeRowOne(2, SymCell(2)) else Stigi.Datalog.Engine.Tests.makeRowTwo(0, SymCell(1), SymCell(2))
                                                          val rowTwo: Stigi.Datalog.Store.Tuple = if reverseOrder then Stigi.Datalog.Engine.Tests.makeRowOne(2, SymCell(1)) else Stigi.Datalog.Engine.Tests.makeRowTwo(0, SymCell(2), SymCell(3))
                                                          val rowThree: Stigi.Datalog.Store.Tuple = if reverseOrder then Stigi.Datalog.Engine.Tests.makeRowOne(2, SymCell(0)) else Stigi.Datalog.Engine.Tests.makeRowOne(2, SymCell(0))
                                                          val rowFour: Stigi.Datalog.Store.Tuple = if reverseOrder then Stigi.Datalog.Engine.Tests.makeRowTwo(0, SymCell(2), SymCell(3)) else Stigi.Datalog.Engine.Tests.makeRowOne(2, SymCell(1))
                                                          val rowFive: Stigi.Datalog.Store.Tuple = if reverseOrder then Stigi.Datalog.Engine.Tests.makeRowTwo(0, SymCell(1), SymCell(2)) else Stigi.Datalog.Engine.Tests.makeRowOne(2, SymCell(2))
                                                          val rowSix: Stigi.Datalog.Store.Tuple = if reverseOrder then Stigi.Datalog.Engine.Tests.makeRowTwo(0, SymCell(0), SymCell(1)) else Stigi.Datalog.Engine.Tests.makeRowOne(2, SymCell(3))
                                                          val addZero: Stigi.Datalog.Store.TupleOutcome = Stigi.Datalog.Store.insertTuple(8, 64, 128, rows, tupleSlots, relHead, nextInRel, rowZero, 0)
                                                          val addOne: Stigi.Datalog.Store.TupleOutcome = Stigi.Datalog.Store.insertTuple(8, 64, 128, rows, tupleSlots, relHead, nextInRel, rowOne, 1)
                                                          val addTwo: Stigi.Datalog.Store.TupleOutcome = Stigi.Datalog.Store.insertTuple(8, 64, 128, rows, tupleSlots, relHead, nextInRel, rowTwo, 2)
                                                          val addThree: Stigi.Datalog.Store.TupleOutcome = Stigi.Datalog.Store.insertTuple(8, 64, 128, rows, tupleSlots, relHead, nextInRel, rowThree, 3)
                                                          val addFour: Stigi.Datalog.Store.TupleOutcome = Stigi.Datalog.Store.insertTuple(8, 64, 128, rows, tupleSlots, relHead, nextInRel, rowFour, 4)
                                                          val addFive: Stigi.Datalog.Store.TupleOutcome = Stigi.Datalog.Store.insertTuple(8, 64, 128, rows, tupleSlots, relHead, nextInRel, rowFive, 5)
                                                          val addSix: Stigi.Datalog.Store.TupleOutcome = Stigi.Datalog.Store.insertTuple(8, 64, 128, rows, tupleSlots, relHead, nextInRel, rowSix, 6)
                                                          val seedsLoaded: Bool = addZero is TupleAdded(_, _) && addOne is TupleAdded(_, _) && addTwo is TupleAdded(_, _) && addThree is TupleAdded(_, _) && addFour is TupleAdded(_, _) && addFive is TupleAdded(_, _) && addSix is TupleAdded(_, _)
                                                          if !seedsLoaded then
                                                            false
                                                          else
                                                            val evalOut: Stigi.Datalog.Engine.EvalResult = Stigi.Datalog.Engine.runProgram(8, 64, 128, 8, 16, 8, 8, relArities, 4, rules, 3, lits, 6, bound, strata, rows, tupleSlots, relHead, nextInRel, varBound, varVals, ranks, iterSlot, iterMask, 7, rowBudget)
                                                            if rowBudget < 16 then
                                                              evalOut match
                                                                case EvalBudgetExceeded(_) => true
                                                                case EvalDone(_, _) => false
                                                                case EvalRefused(_) => false
                                                                case EvalTableFull => false
                                                                case EvalArithError => false
                                                            else
                                                              evalOut match
                                                                case EvalDone(totalRows, derivedRows) =>
                                                                  val chkCounts: Bool = totalRows == 23 && derivedRows == 16
                                                                  val hasReachZeroThree: Bool = Stigi.Datalog.Store.hasTuple(64, 128, rows, tupleSlots, Stigi.Datalog.Engine.Tests.makeRowTwo(1, SymCell(0), SymCell(3)), 23)
                                                                  val absentReachThreeZero: Bool = !Stigi.Datalog.Store.hasTuple(64, 128, rows, tupleSlots, Stigi.Datalog.Engine.Tests.makeRowTwo(1, SymCell(3), SymCell(0)), 23)
                                                                  val hasUnreachThreeZero: Bool = Stigi.Datalog.Store.hasTuple(64, 128, rows, tupleSlots, Stigi.Datalog.Engine.Tests.makeRowTwo(3, SymCell(3), SymCell(0)), 23)
                                                                  val absentUnreachZeroThree: Bool = !Stigi.Datalog.Store.hasTuple(64, 128, rows, tupleSlots, Stigi.Datalog.Engine.Tests.makeRowTwo(3, SymCell(0), SymCell(3)), 23)
                                                                  chkCounts && hasReachZeroThree && absentReachThreeZero && hasUnreachThreeZero && absentUnreachZeroThree
                                                                case EvalRefused(_) => false
                                                                case EvalBudgetExceeded(_) => false
                                                                case EvalTableFull => false
                                                                case EvalArithError => false
