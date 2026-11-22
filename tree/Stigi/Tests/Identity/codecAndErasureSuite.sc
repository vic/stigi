// prompt: test strict LEB128/zigzag codec, post-erasure identity invariance, and canonical SCC ordering.
val Stigi.Tests.Identity.codecAndErasureSuite {Stigi.Test}: Stigi.Test.Result =
  val zeroByte: U8 = 0
  val lebValid: Bool = Array.tryFill{Stigi.Region}(16, zeroByte) match
    case FillFailed => false
    case Filled(buf) =>
      val endMax: U64 = Stigi.Identity.encodeUnsignedLeb(0xffffffffffffffff, 16, buf, 0)
      val decodedMax: Bool = Stigi.Identity.decodeUnsignedLeb(16, buf, 16, 0) match
        case LebDecoded(valOut, nextOff) => valOut == 0xffffffffffffffff && nextOff == 10
        case LebError(_, _) => false
      buf(0) := 0x80
      buf(1) := 0x00
      val nonMinRejected: Bool = Stigi.Identity.decodeUnsignedLeb(16, buf, 2, 0) match
        case LebDecoded(_, _) => false
        case LebError(_, reason) =>
          reason match
            case ReasonNonMinimalNumber => true
            case ReasonUnknownTag => false
            case ReasonTruncated => false
            case ReasonTrailingBytes => false
            case ReasonNumberTooLarge => false
            case ReasonUnboundLocal => false
            case ReasonMemberOutOfRange => false
            case ReasonNonCanonicalGroupOrder => false
            case ReasonWrongSchema => false
      val truncRejected: Bool = Stigi.Identity.decodeUnsignedLeb(16, buf, 1, 0) match
        case LebDecoded(_, _) => false
        case LebError(_, reason) =>
          reason match
            case ReasonTruncated => true
            case ReasonUnknownTag => false
            case ReasonTrailingBytes => false
            case ReasonNonMinimalNumber => false
            case ReasonNumberTooLarge => false
            case ReasonUnboundLocal => false
            case ReasonMemberOutOfRange => false
            case ReasonNonCanonicalGroupOrder => false
            case ReasonWrongSchema => false
      Stigi.Identity.lebOffsetMonotone(0, endMax, 16)
      endMax == 10 && decodedMax && nonMinRejected && truncRejected
  val zigzagZero: Bool = Stigi.Identity.zigzagEncode(0) == 0 && Stigi.Identity.zigzagDecode(0) == 0
  val zigzagNegOne: Bool = Stigi.Identity.zigzagEncode(-1) == 1 && Stigi.Identity.zigzagDecode(1) == -1
  val zigzagNegBig: Bool = Stigi.Identity.zigzagDecode(Stigi.Identity.zigzagEncode(-1000000)) == -1000000
  val bodyBytes: Array[U8](8) = "bodyCode"
  val emptyDeps: Array[U8](1) = "x"
  val factsOne: Array[U8](6) = "below1"
  val factsTwo: Array[U8](6) = "below2"
  val runtimeOne: Stigi.Hash.Digest = Stigi.Identity.hashUnitPostErasure{Stigi.Region}(1, 2, 8, bodyBytes, 1, emptyDeps, 0)
  val runtimeTwo: Stigi.Hash.Digest = Stigi.Identity.hashUnitPostErasure{Stigi.Region}(1, 2, 8, bodyBytes, 1, emptyDeps, 0)
  val verdictOne: Stigi.Hash.Digest = Stigi.Identity.hashVerdictKey{Stigi.Region}(runtimeOne, runtimeOne, 1000000, 6, factsOne)
  val verdictTwo: Stigi.Hash.Digest = Stigi.Identity.hashVerdictKey{Stigi.Region}(runtimeOne, runtimeOne, 1000000, 6, factsTwo)
  val erasureValid: Bool = Stigi.Hash.digestsEqual(runtimeOne, runtimeTwo) && !Stigi.Hash.digestsEqual(verdictOne, verdictTwo)
  val shapeAlpha: Array[U8](6) = "shapeA"
  val shapeBeta: Array[U8](6) = "shapeB"
  val cmpOrder: Bool = Stigi.Identity.compareSlices(6, shapeAlpha, 6, shapeBeta) < 0
  val orderFwd: Stigi.Identity.GroupOrderOutcome = Stigi.Identity.hashGroupPair{Stigi.Region}(6, shapeAlpha, 6, shapeBeta)
  val orderRev: Stigi.Identity.GroupOrderOutcome = Stigi.Identity.hashGroupPair{Stigi.Region}(6, shapeBeta, 6, shapeAlpha)
  val pairSymmetric: Bool = orderFwd match
    case GroupAmbiguous => false
    case GroupOrdered(digFwd, swapFwd) =>
      orderRev match
        case GroupAmbiguous => false
        case GroupOrdered(digRev, swapRev) => !swapFwd && swapRev && Stigi.Hash.digestsEqual(digFwd, digRev)
  val ambigRejected: Bool = Stigi.Identity.hashGroupPair{Stigi.Region}(6, shapeAlpha, 6, shapeAlpha) match
    case GroupAmbiguous => true
    case GroupOrdered(_, _) => false
  val allPassed: Bool = lebValid && zigzagZero && zigzagNegOne && zigzagNegBig && erasureValid && cmpOrder && pairSymmetric && ambigRejected
  Stigi.Test.expect(allPassed, true, "Identity codec, post-erasure vs verdict keys, and canonical SCC ordering pass")
