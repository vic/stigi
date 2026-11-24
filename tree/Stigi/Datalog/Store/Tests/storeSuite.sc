// prompt: verify symbol interning, O(N log N) byte-order ranking, tuple dedup, membership, and per-relation chains.
val Stigi.Datalog.Store.Tests.storeSuite {Stigi.Test}: Stigi.Test.Result =
  val strDelta: Array[U8](5) = "delta"
  val strBeta: Array[U8](4) = "beta"
  val strAlpha: Array[U8](5) = "alpha"
  val strEpsilon: Array[U8](7) = "epsilon"
  val strGamma: Array[U8](5) = "gamma"
  val strAlphaTwo: Array[U8](6) = "alpha2"
  val zeroByte: U8 = U8.ofU64(0)
  val zeroCell: Stigi.Datalog.Store.Cell = SymCell(0)
  val zeroTuple: Stigi.Datalog.Store.Tuple = StoreRow(0, 0, zeroCell, zeroCell, zeroCell, zeroCell, zeroCell, zeroCell, zeroCell)
  val allPass: Bool = Array.tryFill{Stigi.Region}(128, zeroByte) match
    case FillFailed => false
    case Filled(pool) =>
      Array.tryFill{Stigi.Region}(16, 0) match
        case FillFailed => false
        case Filled(starts) =>
          Array.tryFill{Stigi.Region}(16, 0) match
            case FillFailed => false
            case Filled(ends) =>
              Array.tryFill{Stigi.Region}(16, 0) match
                case FillFailed => false
                case Filled(order) =>
                  Array.tryFill{Stigi.Region}(16, 0) match
                    case FillFailed => false
                    case Filled(scratch) =>
                      Array.tryFill{Stigi.Region}(16, 0) match
                        case FillFailed => false
                        case Filled(ranks) =>
                          Array.tryFill{Stigi.Region}(32, StoreVacant) match
                            case FillFailed => false
                            case Filled(symSlots) =>
                              Array.tryFill{Stigi.Region}(32, zeroTuple) match
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
                                              Stigi.Datalog.Store.internSymbol(16, 128, 32, 5, pool, starts, ends, symSlots, strDelta, 0, 5, 0, 0) match
                                                case SymTableFull => false
                                                case Interned(symZero, cntOne, poolOne) =>
                                                  if symZero != 0 || cntOne != 1 || poolOne != 5 then false else Stigi.Datalog.Store.internSymbol(16, 128, 32, 4, pool, starts, ends, symSlots, strBeta, 0, 4, 1, 5) match
                                                    case SymTableFull => false
                                                    case Interned(symOne, cntTwo, poolTwo) =>
                                                      if symOne != 1 || cntTwo != 2 || poolTwo != 9 then false else Stigi.Datalog.Store.internSymbol(16, 128, 32, 5, pool, starts, ends, symSlots, strAlpha, 0, 5, 2, 9) match
                                                        case SymTableFull => false
                                                        case Interned(symTwo, cntThree, poolThree) =>
                                                          if symTwo != 2 || cntThree != 3 || poolThree != 14 then false else Stigi.Datalog.Store.internSymbol(16, 128, 32, 7, pool, starts, ends, symSlots, strEpsilon, 0, 7, 3, 14) match
                                                            case SymTableFull => false
                                                            case Interned(symThree, cntFour, poolFour) =>
                                                              if symThree != 3 || cntFour != 4 || poolFour != 21 then false else Stigi.Datalog.Store.internSymbol(16, 128, 32, 5, pool, starts, ends, symSlots, strGamma, 0, 5, 4, 21) match
                                                                case SymTableFull => false
                                                                case Interned(symFour, cntFive, poolFive) =>
                                                                  if symFour != 4 || cntFive != 5 || poolFive != 26 then false else Stigi.Datalog.Store.internSymbol(16, 128, 32, 6, pool, starts, ends, symSlots, strAlphaTwo, 0, 6, 5, 26) match
                                                                    case SymTableFull => false
                                                                    case Interned(symFive, cntSix, poolSix) =>
                                                                      if symFive != 5 || cntSix != 6 || poolSix != 32 then false else Stigi.Datalog.Store.internSymbol(16, 128, 32, 5, pool, starts, ends, symSlots, strAlpha, 0, 5, 6, 32) match
                                                                        case SymTableFull => false
                                                                        case Interned(dupAlphaSym, cntSeven, poolSeven) =>
                                                                          val chkDupSym: Bool = dupAlphaSym == 2 && cntSeven == 6 && poolSeven == 32
                                                                          val rankedCount: U64 = Stigi.Datalog.Store.computeRanks(16, 128, pool, starts, ends, order, scratch, ranks, 6)
                                                                          val chkRanksOne: Bool = rankedCount == 6 && ranks(0) == 3 && ranks(1) == 2 && ranks(2) == 0
                                                                          val chkRanksTwo: Bool = ranks(3) == 4 && ranks(4) == 5 && ranks(5) == 1
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
                                                                          chkDupSym && chkRanksOne && chkRanksTwo && chkAddTwo && chkAddThree && chkAddSeven && chkDupSeven && chkHasTwo && chkHasThree && chkHasSeven && chkMissDiff && chkChainZero && chkChainOne && chkChainTwo
  Stigi.Test.expect(allPass, true, "Stigi.Datalog.Store suite must pass interning, O(N log N) merge-sort ranking, dedup, and chains")
