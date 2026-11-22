// prompt: verify exact 128-bit integer arithmetic including 2^64 width, negatives, overflow, and division.
val Stigi.Tests.Datalog.exactIntegerSuite {Stigi.Test}: Stigi.Test.Result =
  val maxWord: Stigi.Datalog.Integer = Stigi.Datalog.integerFromUnsigned(0xffffffffffffffff)
  val oneVal: Stigi.Datalog.Integer = Stigi.Datalog.integerFromUnsigned(1)
  val negSeven: Stigi.Datalog.Integer = Stigi.Datalog.integerFromSigned(-7)
  val posThree: Stigi.Datalog.Integer = Stigi.Datalog.integerFromSigned(3)
  val carryPassed: Bool = Stigi.Datalog.integerAdd(maxWord, oneVal) match
    case IntegerValue(pair) =>
      pair match
        case ExactInteger(lowWord, highWord) => lowWord == 0 && highWord == 1
    case IntegerOverflow => false
  val maxSigned: Stigi.Datalog.Integer = ExactInteger(0xffffffffffffffff, 0x7fffffffffffffff)
  val addOverflowPassed: Bool = Stigi.Datalog.integerAdd(maxSigned, oneVal) match
    case IntegerValue(_) => false
    case IntegerOverflow => true
  val subPassed: Bool = Stigi.Datalog.integerSubtract(posThree, negSeven) match
    case IntegerValue(pair) =>
      pair match
        case ExactInteger(lowWord, highWord) => lowWord == 10 && highWord == 0
    case IntegerOverflow => false
  val mulBigPassed: Bool = Stigi.Datalog.integerMultiply(Stigi.Datalog.integerFromUnsigned(0x100000000), Stigi.Datalog.integerFromUnsigned(0x100000000)) match
    case IntegerValue(pair) =>
      pair match
        case ExactInteger(lowWord, highWord) => lowWord == 0 && highWord == 1
    case IntegerOverflow => false
  val mulNegPassed: Bool = Stigi.Datalog.integerMultiply(negSeven, posThree) match
    case IntegerValue(valOut) => Stigi.Datalog.integerCompare(valOut, Stigi.Datalog.integerFromSigned(-21)) == 0
    case IntegerOverflow => false
  val divWidePassed: Bool = Stigi.Datalog.integerQuotient(ExactInteger(0, 1), Stigi.Datalog.integerFromUnsigned(0x100000000)) match
    case IntegerValue(pair) =>
      pair match
        case ExactInteger(lowWord, highWord) => lowWord == 0x100000000 && highWord == 0
    case IntegerOverflow => false
  val divZeroPassed: Bool = Stigi.Datalog.integerQuotient(oneVal, ExactInteger(0, 0)) match
    case IntegerValue(_) => false
    case IntegerOverflow => true
  val cmpPassed: Bool = Stigi.Datalog.integerCompare(negSeven, posThree) == -1 && Stigi.Datalog.integerCompare(posThree, negSeven) == 1 && Stigi.Datalog.integerCompare(posThree, posThree) == 0
  val mixPassed: Bool = U64.mix(0) == 0xe220a8397b1dcdaf && U64.mix(1) == 0x910a2dec89025cc1 && U64.mix(0x8000000000000000) == 0x481ec0a212a9f3db && U64.mix(0xffffffffffffffff) == 0xe4d971771b652c20 && Stigi.Datalog.rowBudget == 1000000
  Stigi.Test.expect(carryPassed && addOverflowPassed && subPassed && mulBigPassed && mulNegPassed && divWidePassed && divZeroPassed && cmpPassed && mixPassed, true, "128-bit exact arithmetic, overflow, wide division, compare, and U64.mix vectors pass")
