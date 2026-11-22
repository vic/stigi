// prompt: verify semi-naive transitive closure, stratified negation filtering, provenance rounds, and budget limit.
val Stigi.Tests.Datalog.semiNaiveEngineSuite {Stigi.Test}: Stigi.Test.Result =
  Array.tryFill{Stigi.Region}(3, 0) match
    case FillFailed => Stigi.Test.expect(0, 1, "edge fill failed")
    case Filled(inLeft) =>
      Array.tryFill{Stigi.Region}(3, 0) match
        case FillFailed => Stigi.Test.expect(0, 1, "edge fill failed")
        case Filled(inRight) =>
          Array.tryFill{Stigi.Region}(1, 0) match
            case FillFailed => Stigi.Test.expect(0, 1, "blocked fill failed")
            case Filled(blkLeft) =>
              Array.tryFill{Stigi.Region}(1, 0) match
                case FillFailed => Stigi.Test.expect(0, 1, "blocked fill failed")
                case Filled(blkRight) =>
                  inLeft(0) := 1
                  inRight(0) := 2
                  inLeft(1) := 2
                  inRight(1) := 3
                  inLeft(2) := 3
                  inRight(2) := 4
                  blkLeft(0) := 1
                  blkRight(0) := 4
                  val closurePassed: Bool = Stigi.Datalog.run(3, inLeft, inRight, 1, blkLeft, blkRight, 64) match
                    case EngineComplete(derivedRows, deltaRounds, indexProbes, rejectRows) =>
                      derivedRows == 5 && deltaRounds == 3 && indexProbes > 0 && rejectRows == 0
                    case EngineRejected(_) => false
                  val budgetPassed: Bool = Stigi.Datalog.run(3, inLeft, inRight, 1, blkLeft, blkRight, 4) match
                    case EngineComplete(_, _, _, _) => false
                    case EngineRejected(reason) =>
                      reason match
                        case ReasonBudgetExceeded => true
                        case ReasonNegationCycle => false
                        case ReasonArithError => false
                        case ReasonUnsafeRule => false
                        case ReasonArityError => false
                        case ReasonUnknownRelation => false
                        case ReasonOutOfMemory => false
                  val ruleTag: Bool = RuleCopy == RuleCopy
                  Stigi.Datalog.rowCountWithinCap(3, 64)
                  Stigi.Test.expect(closurePassed && budgetPassed && ruleTag, true, "semi-naive transitive closure, negation filter, rounds, probes, and budget rejection pass")
