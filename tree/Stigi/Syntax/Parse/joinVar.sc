// prompt: join a var head and its parsed body into a var statement.
def Stigi.Syntax.Parse.joinVar {Stigi.Mut} (tables: Stigi.Syntax.Tables, head: Stigi.Syntax.Parse.Head, init: Stigi.Syntax.Parse.Built, token: U64) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Built =
  head match
    case VarHead(name, invariant, declared, _) =>
      init match
        case Failed(fault, at) => Failed(fault, at)
        case Built(value, after) =>
          Stigi.Syntax.Parse.claimOver(after, invariant, declared, value) match
            case Unclaimed(fault) => Failed(fault, token)
            case Claimed(at, kept, typedAs, assigned) => Stigi.Syntax.Parse.place(tables, at, VarStmt(at, name, kept, typedAs, assigned), after)
    case ValHead(_, _, _) | HandleHead(_, _, _) | AssignHead(_, _) | ValueHead(_) | HeadFailed(_, _) => Failed(UnexpectedToken, token)
