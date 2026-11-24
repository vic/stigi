// prompt: read an operand: a term, or two terms joined by one binary operator.
def Stigi.Syntax.Parse.readOperand {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress) <below(0, Stigi.Limit.maxUnitBytes)>: Stigi.Syntax.Parse.Built =
  Stigi.Syntax.Parse.readTerm(tables, start) match
    case Failed(fault, at) => Failed(fault, at)
    case Built(left, after) =>
      val Progress(next, nodes, lists, pending) = after
      if Stigi.Limit.maxUnitBytes <= next then Built(left, after) else
        Stigi.Syntax.Parse.infixOf(Stigi.Syntax.Parse.kindAt(tables, next)) match
          case Standalone => Built(left, after)
          case Infix(operator) =>
            Stigi.Syntax.Parse.readTerm(tables, Progress(next + 1, nodes, lists, pending)) match
              case Failed(fault, at) => Failed(fault, at)
              case Built(right, afterRight) =>
                Stigi.Syntax.Parse.claimOver(afterRight, left, right, right) match
                  case Unclaimed(fault) => Failed(fault, next)
                  case Claimed(at, leftSide, rightSide, _) => Stigi.Syntax.Parse.place(tables, at, Binary(at, operator, leftSide, rightSide), afterRight)
