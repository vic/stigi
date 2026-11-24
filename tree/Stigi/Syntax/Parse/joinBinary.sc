// prompt: join a left operand and a parsed right operand by the binary operator at a token.
def Stigi.Syntax.Parse.joinBinary {Stigi.Mut} (tables: Stigi.Syntax.Tables, left: Stigi.Syntax.NodeAt, token: U64, right: Stigi.Syntax.Parse.Built) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Built =
  right match
    case Failed(fault, at) => Failed(fault, at)
    case Built(rightNode, after) =>
      Stigi.Syntax.Parse.infixOf(Stigi.Syntax.Parse.kindAt(tables, token)) match
        case Standalone => Failed(UnexpectedToken, token)
        case Infix(operator) =>
          Stigi.Syntax.Parse.claimOver(after, left, rightNode, rightNode) match
            case Unclaimed(fault) => Failed(fault, token)
            case Claimed(at, leftSide, rightSide, _) => Stigi.Syntax.Parse.place(tables, at, Binary(at, operator, leftSide, rightSide), after)
