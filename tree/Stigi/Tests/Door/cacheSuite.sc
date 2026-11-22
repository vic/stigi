// prompt: verify BLAKE3-keyed S119 verdict cache lookup, insertion, and invalidation.
val Stigi.Tests.Door.cacheSuite {Stigi.Test}: Stigi.Test.Result =
  val zeroByte: U8 = 0
  val zeroWord: U64 = 0
  Array.tryFill{Stigi.Region}(16, zeroByte) match
    case FillFailed => Stigi.Test.expect(false, true, "used alloc failed")
    case Filled(usedSlots) =>
      Array.tryFill{Stigi.Region}(16, zeroWord) match
        case FillFailed => Stigi.Test.expect(false, true, "low alloc failed")
        case Filled(keyLow) =>
          Array.tryFill{Stigi.Region}(16, zeroWord) match
            case FillFailed => Stigi.Test.expect(false, true, "mid alloc failed")
            case Filled(keyMid) =>
              Array.tryFill{Stigi.Region}(16, zeroWord) match
                case FillFailed => Stigi.Test.expect(false, true, "high alloc failed")
                case Filled(keyHigh) =>
                  Array.tryFill{Stigi.Region}(16, zeroWord) match
                    case FillFailed => Stigi.Test.expect(false, true, "top alloc failed")
                    case Filled(keyTop) =>
                      Array.tryFill{Stigi.Region}(16, zeroByte) match
                        case FillFailed => Stigi.Test.expect(false, true, "pass alloc failed")
                        case Filled(verdictPassed) =>
                          val digestOne: Stigi.Hash.Digest = Stigi.Hash.hashBytes{Stigi.Region}(3, "abc", 3)
                          val digestTwo: Stigi.Hash.Digest = Stigi.Hash.hashBytes{Stigi.Region}(3, "abd", 3)
                          val missBefore: Bool = Stigi.Door.lookupCache(16, usedSlots, keyLow, keyMid, keyHigh, keyTop, verdictPassed, digestOne) match
                            case CacheMiss => true
                            case CacheHit(_) => false
                          val insertedOne: Bool = Stigi.Door.insertCache(16, usedSlots, keyLow, keyMid, keyHigh, keyTop, verdictPassed, digestOne, true)
                          val hitAfter: Bool = Stigi.Door.lookupCache(16, usedSlots, keyLow, keyMid, keyHigh, keyTop, verdictPassed, digestOne) match
                            case CacheHit(passed) => passed
                            case CacheMiss => false
                          val missOther: Bool = Stigi.Door.lookupCache(16, usedSlots, keyLow, keyMid, keyHigh, keyTop, verdictPassed, digestTwo) match
                            case CacheMiss => true
                            case CacheHit(_) => false
                          val allPassed: Bool = missBefore && insertedOne && hitAfter && missOther
                          Stigi.Test.expect(allPassed, true, "door BLAKE3 cache lookup and invalidation must pass")
