// prompt: join a condition, its then body and its parsed else body into a conditional.
def Stigi.Syntax.Parse.joinIf {Stigi.Mut} (tables: Stigi.Syntax.Tables, condition: Stigi.Syntax.NodeAt, whenTrue: Stigi.Syntax.NodeAt, whenFalse: Stigi.Syntax.Parse.Built, token: U64) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Built =
  whenFalse match
    case Failed(fault, at) => Failed(fault, at)
    case Built(otherwise, after) =>
      Stigi.Syntax.Parse.claimOver(after, condition, whenTrue, otherwise) match
        case Unclaimed(fault) => Failed(fault, token)
        case Claimed(at, tested, chosen, rejected) => Stigi.Syntax.Parse.place(tables, at, IfElse(at, tested, chosen, rejected), after)
