// prompt: join the gathered statements of a layout block into a block node.
def Stigi.Syntax.Parse.joinBlock {Stigi.Mut} (tables: Stigi.Syntax.Tables, statements: Stigi.Syntax.Parse.Listed, token: U64) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Built =
  statements match
    case ListFailed(fault, at) => Failed(fault, at)
    case Listed(lines, after) =>
      Stigi.Syntax.Parse.freeSlot(after) match
        case TableFull => Failed(UnitTooLarge, token)
        case FreeSlot(at) => Stigi.Syntax.Parse.place(tables, at, Block(at, lines), after)
