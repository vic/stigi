// prompt: join a handle head, its value arguments and its parsed body into a handle statement.
def Stigi.Syntax.Parse.joinHandle {Stigi.Mut} (tables: Stigi.Syntax.Tables, head: Stigi.Syntax.Parse.Head, args: Stigi.Syntax.Span, body: Stigi.Syntax.Parse.Built, token: U64) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Built =
  head match
    case HandleHead(binder, name, call) =>
      call match
        case CallHeadFailed(fault, at) => Failed(fault, at)
        case CallHead(typeArgs, capArgs, parens, _) =>
          body match
            case Failed(fault, at) => Failed(fault, at)
            case Built(handled, after) =>
              Stigi.Syntax.Parse.claimOver(after, binder, handled, handled) match
                case Unclaimed(fault) => Failed(fault, token)
                case Claimed(at, named, scoped, _) => Stigi.Syntax.Parse.place(tables, at, HandleStmt(at, named, name, typeArgs, capArgs, args, parens, scoped), after)
    case ValHead(_, _, _) | VarHead(_, _, _, _) | AssignHead(_, _) | ValueHead(_) | HeadFailed(_, _) => Failed(UnexpectedToken, token)
