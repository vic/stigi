// prompt: join a val head, its parsed body and its else arms into a val statement.
def Stigi.Syntax.Parse.joinVal {Stigi.Mut} (tables: Stigi.Syntax.Tables, head: Stigi.Syntax.Parse.Head, init: Stigi.Syntax.NodeAt, arms: Stigi.Syntax.Parse.Listed, token: U64) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Built =
  head match
    case ValHead(pattern, declared, _) =>
      arms match
        case ListFailed(fault, at) => Failed(fault, at)
        case Listed(elseArms, after) =>
          Stigi.Syntax.Parse.claimOver(after, pattern, declared, init) match
            case Unclaimed(fault) => Failed(fault, token)
            case Claimed(at, bound, typedAs, value) => Stigi.Syntax.Parse.place(tables, at, ValStmt(at, bound, typedAs, value, elseArms), after)
    case VarHead(_, _, _, _) | HandleHead(_, _, _) | AssignHead(_, _) | ValueHead(_) | HeadFailed(_, _) => Failed(UnexpectedToken, token)
