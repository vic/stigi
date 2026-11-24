// prompt: read a relation declaration from its name token: its name and its columns in parentheses.
def Stigi.Syntax.Parse.readRelation {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress, isPublic: Bool) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Built =
  val Progress(token, nodes, lists, pending) = start
  val last: U64 = Stigi.Syntax.Parse.nameLast(tables, token, Stigi.Limit.maxUnitBytes)
  if !Stigi.Syntax.Parse.isWord(tables, token) then Failed(UnexpectedToken, token) else
    Stigi.Syntax.Parse.readValueGroup(tables, Progress(Stigi.Syntax.Parse.nextToken(last), nodes, lists, pending)) match
      case ListFailed(fault, at) => Failed(fault, at)
      case Listed(columns, after) =>
        Stigi.Syntax.Parse.freeSlot(after) match
          case TableFull => Failed(UnitTooLarge, token)
          case FreeSlot(at) => Stigi.Syntax.Parse.place(tables, at, RelationDecl(at, isPublic, Stigi.Syntax.Parse.spanText(tables, token, last), columns), after)
