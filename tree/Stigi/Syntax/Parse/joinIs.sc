// prompt: join a subject and a parsed pattern into an is test.
def Stigi.Syntax.Parse.joinIs {Stigi.Mut} (tables: Stigi.Syntax.Tables, subject: Stigi.Syntax.NodeAt, pattern: Stigi.Syntax.Parse.Built, token: U64) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Built =
  pattern match
    case Failed(fault, at) => Failed(fault, at)
    case Built(tested, after) =>
      Stigi.Syntax.Parse.claimOver(after, subject, tested, tested) match
        case Unclaimed(fault) => Failed(fault, token)
        case Claimed(at, checked, against, _) => Stigi.Syntax.Parse.place(tables, at, IsTest(at, checked, against), after)
