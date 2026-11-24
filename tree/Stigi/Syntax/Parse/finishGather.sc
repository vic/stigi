// prompt: turn the final gathering state of a list into its span in the list table.
def Stigi.Syntax.Parse.finishGather {Stigi.Mut} (tables: Stigi.Syntax.Tables, base: U64, gather: Stigi.Syntax.Parse.Gather, token: U64) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Listed =
  gather match
    case Gathered(done) => Stigi.Syntax.Parse.closeList(tables, base, done)
    case GatherFailed(fault, at) => ListFailed(fault, at)
    case Gathering(_) => ListFailed(UnitTooLarge, token)
