// prompt: verify T0 atomic transaction commit and rollback leaving committed state untouched on failure.
val Stigi.Tests.Door.commitSuite {Stigi.Test}: Stigi.Test.Result =
  val zeroByte: U8 = 0
  val zeroWord: U64 = 0
  Array.tryFill{Stigi.Region}(3, zeroWord) match
    case FillFailed => Stigi.Test.expect(false, true, "comm alloc failed")
    case Filled(committedLow) =>
      Array.tryFill{Stigi.Region}(3, zeroWord) match
        case FillFailed => Stigi.Test.expect(false, true, "cand alloc failed")
        case Filled(candidateLow) =>
          Array.tryFill{Stigi.Region}(3, zeroWord) match
            case FillFailed => Stigi.Test.expect(false, true, "code alloc failed")
            case Filled(candidateCode) =>
              Array.tryFill{Stigi.Region}(3, zeroByte) match
                case FillFailed => Stigi.Test.expect(false, true, "cached alloc failed")
                case Filled(candidateCached) =>
                  committedLow(0) := 100
                  committedLow(1) := 200
                  committedLow(2) := 300
                  candidateLow(0) := 101
                  candidateLow(1) := 202
                  candidateLow(2) := 303
                  candidateCode(0) := 0
                  candidateCode(1) := 0
                  candidateCode(2) := 7
                  candidateCached(0) := 1
                  candidateCached(1) := 0
                  candidateCached(2) := 0
                  val rejValid: Bool = Stigi.Door.commitComponent(3, committedLow, candidateLow, candidateCode, candidateCached) match
                    case TransactionAdmitted(_, _, _) => false
                    case TransactionRejected(failedUnit, rejectCode) => failedUnit == 2 && rejectCode == 7 && committedLow(0) == 100 && committedLow(1) == 200
                  candidateCode(2) := 0
                  val admValid: Bool = Stigi.Door.commitComponent(3, committedLow, candidateLow, candidateCode, candidateCached) match
                    case TransactionRejected(_, _) => false
                    case TransactionAdmitted(admittedCount, cachedCount, computedCount) => admittedCount == 3 && cachedCount == 1 && computedCount == 2 && committedLow(0) == 101 && committedLow(2) == 303
                  val allPassed: Bool = rejValid && admValid
                  Stigi.Test.expect(allPassed, true, "door T0 atomic commit and rollback must pass")
