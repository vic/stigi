// prompt: check equality of two exact signed wide integers.
def Stigi.Exact.Tests.sameWide(left: Stigi.Exact.IntWide, right: Stigi.Exact.IntWide): Bool =
  left match
    case WideInt(leftLow, leftHigh) =>
      right match
        case WideInt(rightLow, rightHigh) =>
          leftLow == rightLow && leftHigh == rightHigh
