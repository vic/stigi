// prompt: verify SSA literal deduplication, dominator queries, loop extraction, callee summaries, and end-to-end Datalog closure.
val Stigi.Tests.Extract.ssaAndSummarySuite {Stigi.Test}: Stigi.Test.Result =
  val zeroByte: U8 = 0
  val zeroWord: U64 = 0
  val zeroSigned: I64 = 0
  Array.tryFill{Stigi.Region}(16, zeroByte) match
    case FillFailed => Stigi.Test.expect(false, true, "kinds alloc failed")
    case Filled(factKinds) =>
      Array.tryFill{Stigi.Region}(16, zeroWord) match
        case FillFailed => Stigi.Test.expect(false, true, "left alloc failed")
        case Filled(factLeft) =>
          Array.tryFill{Stigi.Region}(16, zeroWord) match
            case FillFailed => Stigi.Test.expect(false, true, "right alloc failed")
            case Filled(factRight) =>
              Array.tryFill{Stigi.Region}(16, zeroWord) match
                case FillFailed => Stigi.Test.expect(false, true, "scope alloc failed")
                case Filled(factScope) =>
                  Array.tryFill{Stigi.Region}(8, zeroSigned) match
                    case FillFailed => Stigi.Test.expect(false, true, "lit alloc failed")
                    case Filled(literals) =>
                      Array.tryFill{Stigi.Region}(4, zeroWord) match
                        case FillFailed => Stigi.Test.expect(false, true, "dom alloc failed")
                        case Filled(parentOf) =>
                          parentOf(0) := 0
                          parentOf(1) := 0
                          parentOf(2) := 1
                          parentOf(3) := 1
                          val domRootTwo: Bool = Stigi.Extract.dominates(4, parentOf, 2, 0)
                          val domSiblings: Bool = Stigi.Extract.dominates(4, parentOf, 2, 3)
                          val domSelf: Bool = Stigi.Extract.dominates(4, parentOf, 3, 3)
                          val extOutcome: Stigi.Extract.ExtractOutcome = Stigi.Extract.extractBoundsLoop(16, factKinds, factLeft, factRight, factScope, 8, literals, 16)
                          val extValid: Bool = extOutcome match
                            case ExtractBudgetExceeded => false
                            case Extracted(factCount, literalCount) => factCount == 7 && literalCount == 3
                          val dupCount: U64 = Stigi.Extract.dedupLiteral(8, literals, 3, 1)
                          val novelCount: U64 = Stigi.Extract.dedupLiteral(8, literals, dupCount, 42)
                          val dedupValid: Bool = dupCount == 3 && novelCount == 4
                          val sumOutcome: Stigi.Extract.CalleeSummary = Stigi.Extract.summarizeCallee(16, factKinds, factScope, 7, 1, false)
                          val sumValid: Bool = sumOutcome match
                            case Summary(entryCount, exitCount, propagatesTaint, isTotal) =>
                              entryCount == 0 && exitCount == 1 && !propagatesTaint && isTotal
                          val tagChecks: Bool = Stigi.Extract.factKindTag(FactTermOffset) == 3 && Stigi.Extract.factKindTag(FactSubtractNode) == 5 && Stigi.Extract.factKindTag(FactPhiNode) == 7 && Stigi.Extract.factKindTag(FactIndexWrite) == 9 && Stigi.Extract.factKindTag(FactDivisorNonZero) == 10 && Stigi.Extract.factKindTag(FactDenial) == 12
                          Stigi.Extract.factRowWithinCap(7, 16)
                          Array.tryFill{Stigi.Region}(2, zeroWord) match
                            case FillFailed => Stigi.Test.expect(false, true, "edgeLeft alloc failed")
                            case Filled(edgeLeft) =>
                              Array.tryFill{Stigi.Region}(2, zeroWord) match
                                case FillFailed => Stigi.Test.expect(false, true, "edgeRight alloc failed")
                                case Filled(edgeRight) =>
                                  Array.tryFill{Stigi.Region}(1, zeroWord) match
                                    case FillFailed => Stigi.Test.expect(false, true, "blockLeft alloc failed")
                                    case Filled(blockLeft) =>
                                      Array.tryFill{Stigi.Region}(1, zeroWord) match
                                        case FillFailed => Stigi.Test.expect(false, true, "blockRight alloc failed")
                                        case Filled(blockRight) =>
                                          edgeLeft(0) := factLeft(2)
                                          edgeRight(0) := factRight(2)
                                          edgeLeft(1) := factLeft(3)
                                          edgeRight(1) := factRight(3)
                                          blockLeft(0) := 99
                                          blockRight(0) := 99
                                          val datalogOut: Stigi.Datalog.RunOutcome = Stigi.Datalog.run{Stigi.Region}(2, edgeLeft, edgeRight, 1, blockLeft, blockRight, 64)
                                          val datalogValid: Bool = datalogOut match
                                            case EngineComplete(derivedRows, deltaRounds, indexProbes, rejectRows) => derivedRows == 3 && deltaRounds >= 1 && indexProbes >= 3 && rejectRows == 0
                                            case EngineRejected(reason) =>
                                              reason match
                                                case ReasonNegationCycle => false
                                                case ReasonBudgetExceeded => false
                                                case ReasonArithError => false
                                                case ReasonUnsafeRule => false
                                                case ReasonArityError => false
                                                case ReasonUnknownRelation => false
                                                case ReasonOutOfMemory => false
                                          val allPassed: Bool = domRootTwo && !domSiblings && domSelf && extValid && dedupValid && sumValid && tagChecks && datalogValid
                                          Stigi.Test.expect(allPassed, true, "extract SSA dedup, dom, summary, and datalog closure must pass")
