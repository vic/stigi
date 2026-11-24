// prompt: join a condition and its parsed body into a loop.
def Stigi.Syntax.Parse.joinWhile {Stigi.Mut} (tables: Stigi.Syntax.Tables, condition: Stigi.Syntax.NodeAt, body: Stigi.Syntax.Parse.Built, token: U64) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Built =
  body match
    case Failed(fault, at) => Failed(fault, at)
    case Built(looped, after) =>
      Stigi.Syntax.Parse.claimOver(after, condition, looped, looped) match
        case Unclaimed(fault) => Failed(fault, token)
        case Claimed(at, tested, repeated, _) => Stigi.Syntax.Parse.place(tables, at, WhileLoop(at, tested, repeated), after)
