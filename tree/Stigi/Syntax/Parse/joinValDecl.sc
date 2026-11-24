// prompt: join a val declaration head and its parsed body into a val declaration.
def Stigi.Syntax.Parse.joinValDecl {Stigi.Mut} (tables: Stigi.Syntax.Tables, isPublic: Bool, head: Stigi.Syntax.Parse.ValDeclHead, body: Stigi.Syntax.Parse.Built, token: U64) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Built =
  head match
    case ValDeclHeadFailed(fault, at) => Failed(fault, at)
    case ValDeclHead(name, caps, declared, _) =>
      body match
        case Failed(fault, at) => Failed(fault, at)
        case Built(value, after) =>
          Stigi.Syntax.Parse.claimOver(after, declared, value, value) match
            case Unclaimed(fault) => Failed(fault, token)
            case Claimed(at, typed, bodied, _) => Stigi.Syntax.Parse.place(tables, at, ValDecl(at, isPublic, name, caps, typed, bodied), after)
