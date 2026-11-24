// prompt: whether two spans of length rows hold equal normal length terms in order.
def Stigi.Types.sameLengths(table: Stigi.Types.TypeTable, left: Stigi.Core.Span, right: Stigi.Core.Span): Bool =
  table match
    case TypeRows(_, _, _, _, _, _, _, _, _, lengthCapacity, lengths, _, _, _, _, _, _) =>
      left match
        case CoreSpan(leftFirst, leftEnd) =>
          right match
            case CoreSpan(rightFirst, rightEnd) =>
              if leftEnd <= lengthCapacity && rightEnd <= lengthCapacity then
                var leftRow: U64 = leftFirst
                var rightRow: U64 = rightFirst
                var scan: Stigi.Types.Scan = Matching
                while leftRow < leftEnd && rightRow < rightEnd && scan is Matching do
                  @<lessOrEqual(leftFirst, leftRow), lessOrEqual(rightFirst, rightRow)>
                  val equalRow: Bool = lengths(leftRow) match
                    case Linear(leftConstant, leftParts) =>
                      lengths(rightRow) match
                        case Linear(rightConstant, rightParts) => leftConstant == rightConstant && Stigi.Types.sameParts(table, leftParts, rightParts)
                  if equalRow then
                    leftRow = leftRow + 1
                    rightRow = rightRow + 1
                  else
                    scan = Differs
                scan is Matching && leftRow == leftEnd && rightRow == rightEnd
              else false
