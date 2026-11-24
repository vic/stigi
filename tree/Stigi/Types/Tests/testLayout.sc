// prompt: test nullable-pointer optimization, tail-padding tag placement, and tag-only enum layout.
def Stigi.Types.Tests.testLayout {region: Stigi.Region, Stigi.Mut} (caseCount: U64) <lessOrEqual(caseCount, 4)>: Bool =
  val padCase: Stigi.Types.LayoutCalc.CaseSpec = CaseInfo(0, 0)
  val padField: Stigi.Types.LayoutCalc.FieldKind = FieldPointer
  Array.tryFill{region}(4, padCase) match
    case FillFailed => false
    case Filled(cases) =>
      Array.tryFill{region}(4, padField) match
        case FillFailed => false
        case Filled(fields) =>
          cases(0) := CaseInfo(0, 0)
          cases(1) := CaseInfo(0, 1)
          fields(0) := FieldPointer
          val chkNullPtr: Bool = Stigi.Types.LayoutCalc.computeEnumLayout{region}(4, cases, 2, 4, fields, 1) match
            case EnumLayoutInfo(byteSize, alignBytes, tagSize, tagOffset, isNullPtr, tagInTail) =>
              byteSize == 8 && alignBytes == 8 && tagSize == 0 && tagOffset == 0 && isNullPtr && !tagInTail
            case EnumLayoutOverflow => false
          cases(0) := CaseInfo(0, 0)
          cases(1) := CaseInfo(0, 2)
          fields(0) := FieldScalar(1, 1)
          fields(1) := FieldScalar(8, 8)
          val chkTailTag: Bool = Stigi.Types.LayoutCalc.computeEnumLayout{region}(4, cases, 2, 4, fields, 2) match
            case EnumLayoutInfo(byteSize, alignBytes, tagSize, tagOffset, isNullPtr, tagInTail) =>
              byteSize == 16 && alignBytes == 8 && tagSize == 1 && tagOffset == 9 && !isNullPtr && tagInTail
            case EnumLayoutOverflow => false
          cases(0) := CaseInfo(0, 0)
          cases(1) := CaseInfo(0, 0)
          cases(2) := CaseInfo(0, 0)
          val chkTagOnly: Bool = Stigi.Types.LayoutCalc.computeEnumLayout{region}(4, cases, caseCount, 4, fields, 0) match
            case EnumLayoutInfo(byteSize, alignBytes, tagSize, tagOffset, isNullPtr, tagInTail) =>
              byteSize == 1 && alignBytes == 1 && tagSize == 1 && tagOffset == 0 && !isNullPtr && !tagInTail
            case EnumLayoutOverflow => false
          chkNullPtr && chkTailTag && chkTagOnly
