// prompt: join a parsed local declaration into a statement.
def Stigi.Syntax.Parse.joinLocal {Stigi.Mut} (tables: Stigi.Syntax.Tables, decl: Stigi.Syntax.Parse.Built, token: U64) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Built =
  decl match
    case Failed(fault, at) => Failed(fault, at)
    case Built(declared, after) =>
      Stigi.Syntax.Parse.claimOver(after, declared, declared, declared) match
        case Unclaimed(fault) => Failed(fault, token)
        case Claimed(at, local, _, _) => Stigi.Syntax.Parse.place(tables, at, LocalDecl(at, local), after)
