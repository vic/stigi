// prompt: evaluate a comparison operator over two ground cells using exact 128-bit order and symbol ranks.
def Stigi.Datalog.Engine.evalCompare(kind: Stigi.Datalog.Check.CmpKind, left: Stigi.Datalog.Store.Cell, right: Stigi.Datalog.Store.Cell, symCap: U64, ranks: Array[U64](symCap)) <below(0, symCap)>: Bool =
  left match
    case SymCell(leftSym) =>
      right match
        case SymCell(rightSym) =>
          kind match
            case CmpEqual => leftSym == rightSym
            case CmpNotEqual => leftSym != rightSym
            case CmpLess =>
              if leftSym < symCap && rightSym < symCap then ranks(leftSym) < ranks(rightSym) else false
            case CmpLessOrEqual =>
              if leftSym < symCap && rightSym < symCap then ranks(leftSym) <= ranks(rightSym) else false
            case CmpGreater =>
              if leftSym < symCap && rightSym < symCap then ranks(leftSym) > ranks(rightSym) else false
            case CmpGreaterOrEqual =>
              if leftSym < symCap && rightSym < symCap then ranks(leftSym) >= ranks(rightSym) else false
        case NumCell(_) =>
          kind match
            case CmpNotEqual => true
            case CmpEqual => false
            case CmpLess => false
            case CmpLessOrEqual => false
            case CmpGreater => false
            case CmpGreaterOrEqual => false
    case NumCell(leftNum) =>
      right match
        case SymCell(_) =>
          kind match
            case CmpNotEqual => true
            case CmpEqual => false
            case CmpLess => false
            case CmpLessOrEqual => false
            case CmpGreater => false
            case CmpGreaterOrEqual => false
        case NumCell(rightNum) =>
          val ordRes: Stigi.Exact.Ordering = Stigi.Exact.compare(leftNum, rightNum)
          kind match
            case CmpEqual =>
              ordRes match
                case EqualTo => true
                case LessThan => false
                case GreaterThan => false
            case CmpNotEqual =>
              ordRes match
                case EqualTo => false
                case LessThan => true
                case GreaterThan => true
            case CmpLess =>
              ordRes match
                case LessThan => true
                case EqualTo => false
                case GreaterThan => false
            case CmpLessOrEqual =>
              ordRes match
                case LessThan => true
                case EqualTo => true
                case GreaterThan => false
            case CmpGreater =>
              ordRes match
                case GreaterThan => true
                case LessThan => false
                case EqualTo => false
            case CmpGreaterOrEqual =>
              ordRes match
                case GreaterThan => true
                case EqualTo => true
                case LessThan => false
