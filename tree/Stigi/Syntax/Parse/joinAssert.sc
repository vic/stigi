// prompt: join a subject and a parsed fact group into an assertion.
def Stigi.Syntax.Parse.joinAssert {Stigi.Mut} (tables: Stigi.Syntax.Tables, subject: Stigi.Syntax.NodeAt, facts: Stigi.Syntax.Parse.Built, token: U64) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Built =
  facts match
    case Failed(fault, at) => Failed(fault, at)
    case Built(group, after) =>
      Stigi.Syntax.Parse.claimOver(after, subject, group, group) match
        case Unclaimed(fault) => Failed(fault, token)
        case Claimed(at, asserted, held, _) => Stigi.Syntax.Parse.place(tables, at, AssertStmt(at, asserted, held), after)
