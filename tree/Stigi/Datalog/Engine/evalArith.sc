// prompt: evaluate a checked 128-bit arithmetic operator over two terms, returning ArithErr on overflow or divide by zero.
def Stigi.Datalog.Engine.evalArith(varCap: U64, varVals: Array[Stigi.Datalog.Store.Cell](varCap), kind: Stigi.Datalog.Check.ArithKind, leftTerm: Stigi.Datalog.Check.Term, rightTerm: Stigi.Datalog.Check.Term) <below(0, varCap)>: Stigi.Datalog.Engine.ArithEval =
  val leftCell: Stigi.Datalog.Store.Cell = Stigi.Datalog.Engine.resolveTerm(varCap, varVals, leftTerm)
  val rightCell: Stigi.Datalog.Store.Cell = Stigi.Datalog.Engine.resolveTerm(varCap, varVals, rightTerm)
  leftCell match
    case SymCell(_) => ArithErr
    case NumCell(leftNum) =>
      rightCell match
        case SymCell(_) => ArithErr
        case NumCell(rightNum) =>
          kind match
            case ArithAdd =>
              Stigi.Exact.add(leftNum, rightNum) match
                case ExactValue(resNum) => ArithVal(NumCell(resNum))
                case ExactOverflow => ArithErr
                case ExactDivideByZero => ArithErr
            case ArithSub =>
              Stigi.Exact.subtract(leftNum, rightNum) match
                case ExactValue(resNum) => ArithVal(NumCell(resNum))
                case ExactOverflow => ArithErr
                case ExactDivideByZero => ArithErr
            case ArithMul =>
              Stigi.Exact.multiply(leftNum, rightNum) match
                case ExactValue(resNum) => ArithVal(NumCell(resNum))
                case ExactOverflow => ArithErr
                case ExactDivideByZero => ArithErr
            case ArithDiv =>
              Stigi.Exact.quotientRemainder(leftNum, rightNum) match
                case DivModValue(quotNum, _) => ArithVal(NumCell(quotNum))
                case DivModOverflow => ArithErr
                case DivModDivideByZero => ArithErr
            case ArithRem =>
              Stigi.Exact.quotientRemainder(leftNum, rightNum) match
                case DivModValue(_, remNum) => ArithVal(NumCell(remNum))
                case DivModOverflow => ArithErr
                case DivModDivideByZero => ArithErr
            case ArithMin =>
              Stigi.Exact.compare(leftNum, rightNum) match
                case GreaterThan => ArithVal(NumCell(rightNum))
                case LessThan => ArithVal(NumCell(leftNum))
                case EqualTo => ArithVal(NumCell(leftNum))
            case ArithMax =>
              Stigi.Exact.compare(leftNum, rightNum) match
                case LessThan => ArithVal(NumCell(rightNum))
                case GreaterThan => ArithVal(NumCell(leftNum))
                case EqualTo => ArithVal(NumCell(leftNum))
