// prompt: verify the O(N log N) byte-order ranking over six interned symbols.
val Stigi.Datalog.Store.Tests.rankSuite {Stigi.Test}: Stigi.Test.Result =
  val strDelta: Array[U8](5) = "delta"
  val strBeta: Array[U8](4) = "beta"
  val strAlpha: Array[U8](5) = "alpha"
  val strEpsilon: Array[U8](7) = "epsilon"
  val strGamma: Array[U8](5) = "gamma"
  val strAlphaTwo: Array[U8](6) = "alpha2"
  val zeroByte: U8 = U8.ofU64(0)
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
                              val chkDelta: Bool = Stigi.Datalog.Store.internSymbol(16, 128, 32, 5, pool, starts, ends, symSlots, strDelta, 0, 5, 0, 0) match
                                case SymTableFull => false
                                case Interned(symZero, cntOne, poolOne) => symZero == 0 && cntOne == 1 && poolOne == 5
                              val chkBeta: Bool = Stigi.Datalog.Store.internSymbol(16, 128, 32, 4, pool, starts, ends, symSlots, strBeta, 0, 4, 1, 5) match
                                case SymTableFull => false
                                case Interned(symOne, cntTwo, poolTwo) => symOne == 1 && cntTwo == 2 && poolTwo == 9
                              val chkAlpha: Bool = Stigi.Datalog.Store.internSymbol(16, 128, 32, 5, pool, starts, ends, symSlots, strAlpha, 0, 5, 2, 9) match
                                case SymTableFull => false
                                case Interned(symTwo, cntThree, poolThree) => symTwo == 2 && cntThree == 3 && poolThree == 14
                              val chkEpsilon: Bool = Stigi.Datalog.Store.internSymbol(16, 128, 32, 7, pool, starts, ends, symSlots, strEpsilon, 0, 7, 3, 14) match
                                case SymTableFull => false
                                case Interned(symThree, cntFour, poolFour) => symThree == 3 && cntFour == 4 && poolFour == 21
                              val chkGamma: Bool = Stigi.Datalog.Store.internSymbol(16, 128, 32, 5, pool, starts, ends, symSlots, strGamma, 0, 5, 4, 21) match
                                case SymTableFull => false
                                case Interned(symFour, cntFive, poolFive) => symFour == 4 && cntFive == 5 && poolFive == 26
                              val chkAlphaTwo: Bool = Stigi.Datalog.Store.internSymbol(16, 128, 32, 6, pool, starts, ends, symSlots, strAlphaTwo, 0, 6, 5, 26) match
                                case SymTableFull => false
                                case Interned(symFive, cntSix, poolSix) => symFive == 5 && cntSix == 6 && poolSix == 32
                              val rankedCount: U64 = Stigi.Datalog.Store.computeRanks(16, 128, pool, starts, ends, order, scratch, ranks, 6)
                              val chkRanksOne: Bool = rankedCount == 6 && ranks(0) == 3 && ranks(1) == 2 && ranks(2) == 0
                              val chkRanksTwo: Bool = ranks(3) == 4 && ranks(4) == 5 && ranks(5) == 1
                              val chkInterns: Bool = chkDelta && chkBeta && chkAlpha && chkEpsilon
                              chkInterns && chkGamma && chkAlphaTwo && chkRanksOne && chkRanksTwo
  Stigi.Test.expect(allPass, true, "Stigi.Datalog.Store O(N log N) merge-sort ranking must order six interned symbols by bytes")
