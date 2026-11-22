// prompt: verify stratification accepts positive cycles and stratified negation while rejecting negation cycles.
val Stigi.Tests.Datalog.stratificationSuite {Stigi.Test}: Stigi.Test.Result =
  val zeroWord: U64 = 0
  val zeroByte: U8 = 0
  Array.tryFill{Stigi.Region}(2, zeroWord) match
    case FillFailed => Stigi.Test.expect(0, 1, "stratification fill failed")
    case Filled(heads) =>
      Array.tryFill{Stigi.Region}(2, zeroWord) match
        case FillFailed => Stigi.Test.expect(0, 1, "stratification fill failed")
        case Filled(bodies) =>
          Array.tryFill{Stigi.Region}(2, zeroByte) match
            case FillFailed => Stigi.Test.expect(0, 1, "stratification fill failed")
            case Filled(negs) =>
              heads(0) := 0
              bodies(0) := 1
              negs(0) := 1
              heads(1) := 1
              bodies(1) := 0
              negs(1) := 0
              val cycleRejected: Bool = Stigi.Datalog.stratify(2, 2, heads, bodies, negs) match
                case StratifyCycle => true
                case StratifiedValid(_) => false
              heads(1) := 1
              bodies(1) := 1
              negs(1) := 0
              val stratAccepted: Bool = Stigi.Datalog.stratify(2, 2, heads, bodies, negs) match
                case StratifyCycle => false
                case StratifiedValid(level) => level == 1
              Stigi.Test.expect(cycleRejected && stratAccepted, true, "negation cycle rejected and stratified negation plus positive self-cycle accepted")
