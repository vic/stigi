// prompt: verify tuple insertion, dedup, membership, and per-relation chains.
val Stigi.Datalog.Store.Tests.tupleSuite {Stigi.Test}: Stigi.Test.Result =
  val zeroCell: Stigi.Datalog.Store.Cell = SymCell(0)
  val zeroTuple: Stigi.Datalog.Store.Tuple = StoreRow(0, 0, zeroCell, zeroCell, zeroCell, zeroCell, zeroCell, zeroCell, zeroCell)
  val allPass: Bool = Array.tryFill{Stigi.Region}(32, zeroTuple) match
    case FillFailed => false
    case Filled(rows) =>
      Array.tryFill{Stigi.Region}(64, StoreVacant) match
        case FillFailed => false
        case Filled(tupleSlots) =>
          Array.tryFill{Stigi.Region}(8, StoreVacant) match
            case FillFailed => false
            case Filled(relHead) =>
              Array.tryFill{Stigi.Region}(32, StoreVacant) match
                case FillFailed => false
                case Filled(nextInRel) =>
                  val tupleTwo: Stigi.Datalog.Store.Tuple = StoreRow(0, 2, SymCell(0), NumCell(WideInt(42, 0)), zeroCell, zeroCell, zeroCell, zeroCell, zeroCell)
                  val tupleThree: Stigi.Datalog.Store.Tuple = StoreRow(1, 3, SymCell(1), NumCell(WideInt(18446744073709551615, -1)), SymCell(2), zeroCell, zeroCell, zeroCell, zeroCell)
                  val tupleSeven: Stigi.Datalog.Store.Tuple = StoreRow(0, 7, SymCell(0), SymCell(1), SymCell(2), NumCell(WideInt(10, 0)), NumCell(WideInt(20, 0)), NumCell(WideInt(30, 0)), NumCell(WideInt(40, 0)))
                  val tupleSevenDiff: Stigi.Datalog.Store.Tuple = StoreRow(0, 7, SymCell(0), SymCell(1), SymCell(2), NumCell(WideInt(10, 0)), NumCell(WideInt(20, 0)), NumCell(WideInt(30, 0)), NumCell(WideInt(41, 0)))
                  val chkAddTwo: Bool = Stigi.Datalog.Store.insertTuple(8, 32, 64, rows, tupleSlots, relHead, nextInRel, tupleTwo, 0) match
                    case TupleAdded(idxTwo, rowCntOne) => idxTwo == 0 && rowCntOne == 1
                    case TupleDuplicate(_) => false
                    case TupleTableFull => false
                  val chkAddThree: Bool = Stigi.Datalog.Store.insertTuple(8, 32, 64, rows, tupleSlots, relHead, nextInRel, tupleThree, 1) match
                    case TupleAdded(idxThree, rowCntTwo) => idxThree == 1 && rowCntTwo == 2
                    case TupleDuplicate(_) => false
                    case TupleTableFull => false
                  val chkAddSeven: Bool = Stigi.Datalog.Store.insertTuple(8, 32, 64, rows, tupleSlots, relHead, nextInRel, tupleSeven, 2) match
                    case TupleAdded(idxSeven, rowCntThree) => idxSeven == 2 && rowCntThree == 3
                    case TupleDuplicate(_) => false
                    case TupleTableFull => false
                  val chkDupSeven: Bool = Stigi.Datalog.Store.insertTuple(8, 32, 64, rows, tupleSlots, relHead, nextInRel, tupleSeven, 3) match
                    case TupleDuplicate(dupIdx) => dupIdx == 2
                    case TupleAdded(_, _) => false
                    case TupleTableFull => false
                  val chkHasTwo: Bool = Stigi.Datalog.Store.hasTuple(32, 64, rows, tupleSlots, tupleTwo, 3)
                  val chkHasThree: Bool = Stigi.Datalog.Store.hasTuple(32, 64, rows, tupleSlots, tupleThree, 3)
                  val chkHasSeven: Bool = Stigi.Datalog.Store.hasTuple(32, 64, rows, tupleSlots, tupleSeven, 3)
                  val chkMissDiff: Bool = !Stigi.Datalog.Store.hasTuple(32, 64, rows, tupleSlots, tupleSevenDiff, 3)
                  val chkChainZero: Bool = Stigi.Datalog.Store.Tests.countChain(8, 32, relHead, nextInRel, 0) == 2
                  val chkChainOne: Bool = Stigi.Datalog.Store.Tests.countChain(8, 32, relHead, nextInRel, 1) == 1
                  val chkChainTwo: Bool = Stigi.Datalog.Store.Tests.countChain(8, 32, relHead, nextInRel, 2) == 0
                  val chkAdds: Bool = chkAddTwo && chkAddThree && chkAddSeven && chkDupSeven
                  val chkHas: Bool = chkHasTwo && chkHasThree && chkHasSeven && chkMissDiff
                  val chkChains: Bool = chkChainZero && chkChainOne && chkChainTwo
                  chkAdds && chkHas && chkChains
  Stigi.Test.expect(allPass, true, "Stigi.Datalog.Store tuple insertion must dedup, answer membership, and keep per-relation chains")
