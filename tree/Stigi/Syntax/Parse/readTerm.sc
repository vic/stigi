// prompt: read a term: a literal, a unit value, a negated literal or a dotted name.
def Stigi.Syntax.Parse.readTerm {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress) <below(0, Stigi.Limit.maxUnitBytes)>: Stigi.Syntax.Parse.Built =
  val Progress(token, nodes, lists, pending) = start
  if Stigi.Limit.maxUnitBytes <= token then Failed(UnexpectedToken, token) else
    Stigi.Syntax.Parse.leadOf(Stigi.Syntax.Parse.kindAt(tables, token)) match
      case StartsLiteral(kind) => Stigi.Syntax.Parse.literal(tables, start, kind, token)
      case StartsName => Stigi.Syntax.Parse.reference(tables, start)
      case StartsParen => if Stigi.Syntax.Parse.kindAt(tables, token + 1) is TokRightParen then Stigi.Syntax.Parse.literal(tables, start, UnitLiteral, token + 1) else Failed(UnexpectedToken, token)
      case StartsPrefix(operator) =>
        Stigi.Syntax.Parse.literalAt(tables, Progress(token + 1, nodes, lists, pending)) match
          case Failed(fault, at) => Failed(fault, at)
          case Built(operand, after) =>
            Stigi.Syntax.Parse.claimOver(after, operand, operand, operand) match
              case Unclaimed(fault) => Failed(fault, token)
              case Claimed(at, child, _, _) => Stigi.Syntax.Parse.place(tables, at, Unary(at, operator, child), after)
      case StartsIf | StartsWhile | StartsVal | StartsVar | StartsHandle | StartsAssert | StartsDecl | StartsNothing => Failed(UnexpectedToken, token)
