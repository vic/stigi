// prompt: join a callee, its call head and its parsed value arguments into a call.
def Stigi.Syntax.Parse.joinCall {Stigi.Mut} (tables: Stigi.Syntax.Tables, callee: Stigi.Syntax.NodeAt, head: Stigi.Syntax.Parse.CallHead, args: Stigi.Syntax.Parse.Listed, token: U64) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Built =
  head match
    case CallHeadFailed(fault, at) => Failed(fault, at)
    case CallHead(typeArgs, capArgs, parens, _) =>
      args match
        case ListFailed(fault, at) => Failed(fault, at)
        case Listed(values, after) =>
          Stigi.Syntax.Parse.claimOver(after, callee, callee, callee) match
            case Unclaimed(fault) => Failed(fault, token)
            case Claimed(at, called, _, _) => Stigi.Syntax.Parse.place(tables, at, Call(at, called, typeArgs, capArgs, values, parens), after)
