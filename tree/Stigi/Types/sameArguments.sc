// prompt: whether two spans of the argument list hold the same interned type indexes.
def Stigi.Types.sameArguments(table: Stigi.Types.TypeTable, left: Stigi.Core.Span, right: Stigi.Core.Span): Bool =
  table match
    case TypeRows(_, _, _, _, _, listCapacity, typeLists, _, _, _, _, _, _, _, _, _, _) =>
      left match
        case CoreSpan(leftFirst, leftEnd) =>
          right match
            case CoreSpan(rightFirst, rightEnd) =>
              if leftEnd <= listCapacity && rightEnd <= listCapacity then
                var leftPosition: U64 = leftFirst
                var rightPosition: U64 = rightFirst
                var scan: Stigi.Types.Scan = Matching
                while leftPosition < leftEnd && rightPosition < rightEnd && scan is Matching do
                  @<lessOrEqual(leftFirst, leftPosition), lessOrEqual(rightFirst, rightPosition)>
                  if typeLists(leftPosition) == typeLists(rightPosition) then
                    leftPosition = leftPosition + 1
                    rightPosition = rightPosition + 1
                  else
                    scan = Differs
                scan is Matching && leftPosition == leftEnd && rightPosition == rightEnd
              else false
