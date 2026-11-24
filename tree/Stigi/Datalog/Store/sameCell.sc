// prompt: test structural equality of two Datalog tuple cells.
pub def Stigi.Datalog.Store.sameCell(left: Stigi.Datalog.Store.Cell, right: Stigi.Datalog.Store.Cell): Bool =
  left match
    case SymCell(leftSym) =>
      right match
        case SymCell(rightSym) => leftSym == rightSym
        case NumCell(_) => false
    case NumCell(leftNum) =>
      right match
        case SymCell(_) => false
        case NumCell(rightNum) =>
          leftNum match
            case WideInt(leftLow, leftHigh) =>
              rightNum match
                case WideInt(rightLow, rightHigh) =>
                  leftLow == rightLow && leftHigh == rightHigh
