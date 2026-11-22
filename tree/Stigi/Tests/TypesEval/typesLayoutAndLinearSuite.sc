// prompt: test enum layouts (1-word and tail-padding), linear length normalizer, recursive-type, and extern-forges-facts.
val Stigi.Tests.TypesEval.typesLayoutAndLinearSuite {Stigi.Test}: Stigi.Test.Result =
  val primValid: Bool = Stigi.Types.primitiveSize(Stigi.Types.TypeKind.TypeLong) == 8 && Stigi.Types.primitiveAlign(Stigi.Types.TypeKind.TypeWord) == 4 && Stigi.Types.alignForward(17, 8) == 24
  val nullLayout: Stigi.Types.EnumLayout = Stigi.Types.computeEnumLayout(2, 8, 8, 1, true, 0)
  val nullValid: Bool = nullLayout match
    case LayoutNullablePtr(_, byteSize, byteAlign) => byteSize == 8 && byteAlign == 8
    case LayoutTailTag(_, _, _) => false
    case LayoutTagged(_, _, _) => false
  val tailLayout: Stigi.Types.EnumLayout = Stigi.Types.computeEnumLayout(3, 16, 8, 1, false, 7)
  val tailValid: Bool = tailLayout match
    case LayoutTailTag(byteSize, byteAlign, tagOff) => byteSize == 16 && byteAlign == 8 && tagOff == 9
    case LayoutNullablePtr(_, _, _) => false
    case LayoutTagged(_, _, _) => false
  val taggedLayout: Stigi.Types.EnumLayout = Stigi.Types.computeEnumLayout(3, 8, 8, 0, false, 0)
  val taggedValid: Bool = taggedLayout match
    case LayoutTagged(byteSize, byteAlign, payloadOff) => byteSize == 16 && byteAlign == 8 && payloadOff == 8
    case LayoutNullablePtr(_, _, _) => false
    case LayoutTailTag(_, _, _) => false
  val zeroWord: U64 = 0
  val zeroSigned: I64 = 0
  val linearAndCycleValid: Bool = Array.tryFill{Stigi.Region}(4, zeroWord) match
    case FillFailed => false
    case Filled(inBinders) =>
      Array.tryFill{Stigi.Region}(4, zeroSigned) match
        case FillFailed => false
        case Filled(inCoeffs) =>
          Array.tryFill{Stigi.Region}(4, zeroWord) match
            case FillFailed => false
            case Filled(outBinders) =>
              Array.tryFill{Stigi.Region}(4, zeroSigned) match
                case FillFailed => false
                case Filled(outCoeffs) =>
                  inBinders(0) := 5
                  inCoeffs(0) := 1
                  inBinders(1) := 2
                  inCoeffs(1) := 3
                  inBinders(2) := 5
                  inCoeffs(2) := -1
                  val countNorm: U64 = Stigi.Types.normalizeLinear(4, inBinders, inCoeffs, 3, 4, outBinders, outCoeffs)
                  val offCheck: Stigi.Types.OffsetOutcome = Stigi.Types.linearConstantOffset(10, 4, outBinders, outCoeffs, countNorm, 3, 4, outBinders, outCoeffs, countNorm)
                  val offValid: Bool = offCheck match
                    case OffsetExact(delta) => delta == 7
                    case OffsetDistinct => false
                  Array.tryFill{Stigi.Region}(2, zeroWord) match
                    case FillFailed => false
                    case Filled(edgeFrom) =>
                      Array.tryFill{Stigi.Region}(2, zeroWord) match
                        case FillFailed => false
                        case Filled(edgeTo) =>
                          edgeFrom(0) := 0
                          edgeTo(0) := 1
                          edgeFrom(1) := 1
                          edgeTo(1) := 0
                          val cycDetected: Bool = Stigi.Types.hasRecursiveCycle{Stigi.Region}(2, 2, edgeFrom, edgeTo)
                          edgeTo(1) := 2
                          val acycClean: Bool = !Stigi.Types.hasRecursiveCycle{Stigi.Region}(3, 2, edgeFrom, edgeTo)
                          countNorm == 1 && outBinders(0) == 2 && outCoeffs(0) == 3 && offValid && cycDetected && acycClean
  val forgeBlocked: Bool = Stigi.Types.externForgesFacts(false, true, false, true) && !Stigi.Types.externForgesFacts(false, true, false, false)
  Stigi.Types.alignedLowerBound(17, 24, 64)
  val allPassed: Bool = primValid && nullValid && tailValid && taggedValid && linearAndCycleValid && forgeBlocked
  Stigi.Test.expect(allPassed, true, "Types phase checks 1-word/tail-tag layouts, linear normalization, recursive cycles, and extern-forges-facts")
