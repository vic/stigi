// prompt: verify Stigi.Types.integerInfoOf and Stigi.Types.widthBits for all eight L0 primitive types.
val Stigi.Types.Tests.integerInfoSuite {Stigi.Test}: Stigi.Test.Result =
  val unsignedBytePassed: Bool = Stigi.Types.integerInfoOf(UnsignedByte) match
    case IntSpec(WideInt(lowWord, lowHigh), WideInt(highWord, highHigh), WidthEight) =>
      lowWord == 0 && lowHigh == 0 && highWord == 255 && highHigh == 0 && Stigi.Types.widthBits(WidthEight) == 8
    case IntSpec(_, _, WidthThirtyTwo | WidthSixtyFour) | NonInteger => false
  val unsignedWordPassed: Bool = Stigi.Types.integerInfoOf(UnsignedWord) match
    case IntSpec(WideInt(lowWord, lowHigh), WideInt(highWord, highHigh), WidthThirtyTwo) =>
      lowWord == 0 && lowHigh == 0 && highWord == 4294967295 && highHigh == 0 && Stigi.Types.widthBits(WidthThirtyTwo) == 32
    case IntSpec(_, _, WidthEight | WidthSixtyFour) | NonInteger => false
  val unsignedLongPassed: Bool = Stigi.Types.integerInfoOf(UnsignedLong) match
    case IntSpec(WideInt(lowWord, lowHigh), WideInt(highWord, highHigh), WidthSixtyFour) =>
      lowWord == 0 && lowHigh == 0 && highWord == 18446744073709551615 && highHigh == 0 && Stigi.Types.widthBits(WidthSixtyFour) == 64
    case IntSpec(_, _, WidthEight | WidthThirtyTwo) | NonInteger => false
  val signedWordPassed: Bool = Stigi.Types.integerInfoOf(SignedWord) match
    case IntSpec(WideInt(lowWord, lowHigh), WideInt(highWord, highHigh), WidthThirtyTwo) =>
      lowWord == U64.wrappingSubtract(0, 2147483648) && lowHigh == -1 && highWord == 2147483647 && highHigh == 0
    case IntSpec(_, _, WidthEight | WidthSixtyFour) | NonInteger => false
  val signedLongPassed: Bool = Stigi.Types.integerInfoOf(SignedLong) match
    case IntSpec(WideInt(lowWord, lowHigh), WideInt(highWord, highHigh), WidthSixtyFour) =>
      lowWord == 9223372036854775808 && lowHigh == -1 && highWord == 9223372036854775807 && highHigh == 0
    case IntSpec(_, _, WidthEight | WidthThirtyTwo) | NonInteger => false
  val nonIntPassed: Bool = Stigi.Types.integerInfoOf(Boolean) is NonInteger && Stigi.Types.integerInfoOf(UnitValue) is NonInteger && Stigi.Types.integerInfoOf(NeverValue) is NonInteger
  Stigi.Test.expect(unsignedBytePassed && unsignedWordPassed && unsignedLongPassed && signedWordPassed && signedLongPassed && nonIntPassed, true, "integerInfoOf and widthBits must cover all eight primitives exactly")
