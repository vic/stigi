// prompt: compare two exact signed wide integers and return their ordering.
pub def Stigi.Exact.compare(left: Stigi.Exact.IntWide, right: Stigi.Exact.IntWide): Stigi.Exact.Ordering =
  left match
    case WideInt(leftLow, leftHigh) =>
      right match
        case WideInt(rightLow, rightHigh) =>
          if leftHigh < rightHigh then
            LessThan
          else if leftHigh > rightHigh then
            GreaterThan
          else if leftLow < rightLow then
            LessThan
          else if leftLow > rightLow then
            GreaterThan
          else
            EqualTo
