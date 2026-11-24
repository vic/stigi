// prompt: whether two spans of linear parts hold the same coefficients and binders in order.
def Stigi.Types.sameParts(table: Stigi.Types.TypeTable, left: Stigi.Core.Span, right: Stigi.Core.Span): Bool =
  table match
    case TypeRows(_, _, _, _, _, _, _, _, _, _, _, _, _, partCapacity, parts, _, _) =>
      left match
        case CoreSpan(leftFirst, leftEnd) =>
          right match
            case CoreSpan(rightFirst, rightEnd) =>
              if leftEnd <= partCapacity && rightEnd <= partCapacity then
                var leftPosition: U64 = leftFirst
                var rightPosition: U64 = rightFirst
                var scan: Stigi.Types.Scan = Matching
                while leftPosition < leftEnd && rightPosition < rightEnd && scan is Matching do
                  @<lessOrEqual(leftFirst, leftPosition), lessOrEqual(rightFirst, rightPosition)>
                  val equalPart: Bool = parts(leftPosition) match
                    case Part(leftCoefficient, leftBinder) =>
                      parts(rightPosition) match
                        case Part(rightCoefficient, rightBinder) => leftCoefficient == rightCoefficient && leftBinder == rightBinder
                  if equalPart then
                    leftPosition = leftPosition + 1
                    rightPosition = rightPosition + 1
                  else
                    scan = Differs
                scan is Matching && leftPosition == leftEnd && rightPosition == rightEnd
              else false
