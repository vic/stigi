// prompt: join a prefix operator and its parsed operand.
def Stigi.Syntax.Parse.joinUnary {Stigi.Mut} (tables: Stigi.Syntax.Tables, operator: Stigi.Syntax.UnaryOperator, operand: Stigi.Syntax.Parse.Built, token: U64) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Built =
  operand match
    case Failed(fault, at) => Failed(fault, at)
    case Built(child, after) =>
      Stigi.Syntax.Parse.claimOver(after, child, child, child) match
        case Unclaimed(fault) => Failed(fault, token)
        case Claimed(at, operated, _, _) => Stigi.Syntax.Parse.place(tables, at, Unary(at, operator, operated), after)
