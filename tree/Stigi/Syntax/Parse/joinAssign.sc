// prompt: join an assignment head and its parsed body into an assignment.
def Stigi.Syntax.Parse.joinAssign {Stigi.Mut} (tables: Stigi.Syntax.Tables, head: Stigi.Syntax.Parse.Head, value: Stigi.Syntax.Parse.Built, token: U64) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Built =
  head match
    case AssignHead(name, _) =>
      value match
        case Failed(fault, at) => Failed(fault, at)
        case Built(assigned, after) =>
          Stigi.Syntax.Parse.claimOver(after, assigned, assigned, assigned) match
            case Unclaimed(fault) => Failed(fault, token)
            case Claimed(at, written, _, _) => Stigi.Syntax.Parse.place(tables, at, AssignStmt(at, name, written), after)
    case ValHead(_, _, _) | VarHead(_, _, _, _) | HandleHead(_, _, _) | ValueHead(_) | HeadFailed(_, _) => Failed(UnexpectedToken, token)
