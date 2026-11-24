// prompt: join a pattern, a guard and a parsed body into a match arm.
def Stigi.Syntax.Parse.joinArm {Stigi.Mut} (tables: Stigi.Syntax.Tables, pattern: Stigi.Syntax.NodeAt, guard: Stigi.Syntax.NodeAt, body: Stigi.Syntax.Parse.Built, token: U64) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Built =
  body match
    case Failed(fault, at) => Failed(fault, at)
    case Built(chosen, after) =>
      Stigi.Syntax.Parse.claimOver(after, pattern, guard, chosen) match
        case Unclaimed(fault) => Failed(fault, token)
        case Claimed(at, matched, checked, taken) => Stigi.Syntax.Parse.place(tables, at, Arm(at, matched, checked, taken), after)
