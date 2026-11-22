// prompt: test unit header parsing, reference collection, and canonical print round-trip idempotence.
val Stigi.Tests.FrontEnd.parseAndPrintSuite {Stigi.Test}: Stigi.Test.Result =
  val unitText: Array[U8](63) = "// prompt: demo  \npub def Demo.sum(x: U64): U64 = Stigi.Core.id"
  val parsed: Stigi.Parse.UnitHeaderOutcome = Stigi.Parse.parseUnitHeader(63, unitText)
  val headerValid: Bool = parsed match
    case UnitHeader(kind, isPub, nameStart, nameEnd) =>
      kind match
        case DeclDef => isPub && Stigi.Parse.sliceEquals(63, unitText, nameStart, nameEnd, 8, "Demo.sum")
        case DeclLemma => false
        case DeclVal => false
        case DeclEnum => false
        case DeclType => false
        case DeclCapability => false
        case DeclHandler => false
        case DeclExtern => false
        case DeclRelation => false
        case DeclRule => false
        case DeclUnknown => false
    case UnitHeaderError(_, _) => false
  val badText: Array[U8](28) = "// prompt: bad\nimport Foo.id"
  val badParsed: Stigi.Parse.UnitHeaderOutcome = Stigi.Parse.parseUnitHeader(28, badText)
  val badRejected: Bool = badParsed match
    case UnitHeader(_, _, _, _) => false
    case UnitHeaderError(_, code) => code == 2
  val zeroWord: U64 = 0
  val zeroByte: U8 = 0
  val refAndPrintValid: Bool = Array.tryFill{Stigi.Region}(4, zeroWord) match
    case FillFailed => false
    case Filled(refStarts) =>
      Array.tryFill{Stigi.Region}(4, zeroWord) match
        case FillFailed => false
        case Filled(refEnds) =>
          Array.tryFill{Stigi.Region}(64, zeroByte) match
            case FillFailed => false
            case Filled(outOne) =>
              Array.tryFill{Stigi.Region}(64, zeroByte) match
                case FillFailed => false
                case Filled(outTwo) =>
                  val refsFound: U64 = Stigi.Parse.collectReferences(63, unitText, 35, 4, refStarts, refEnds)
                  val copiedLen: U64 = Stigi.Print.writeSlice(63, unitText, 0, 10, 64, outTwo, 0)
                  val firstLen: U64 = Stigi.Print.canonicalizeUnit(63, unitText, 63, 64, outOne)
                  val secondLen: U64 = Stigi.Print.canonicalizeUnit(64, outOne, firstLen, 64, outTwo)
                  refsFound == 1 && copiedLen == 10 && firstLen == 61 && secondLen == 61 && Stigi.Print.canonicalEquals(64, outOne, 64, outTwo)
  val allPassed: Bool = headerValid && badRejected && refAndPrintValid
  Stigi.Test.expect(allPassed, true, "parser extracts headers and refs and canonical printer is idempotent")
