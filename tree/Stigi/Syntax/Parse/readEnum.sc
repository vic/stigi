// prompt: read an enum declaration from its name token: its name, type parameters, colon and case lines.
def Stigi.Syntax.Parse.readEnum {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress, isPublic: Bool) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Built =
  val Progress(token, nodes, lists, pending) = start
  val last: U64 = Stigi.Syntax.Parse.nameLast(tables, token, Stigi.Limit.maxUnitBytes)
  if !Stigi.Syntax.Parse.isWord(tables, token) then Failed(UnexpectedToken, token) else
    Stigi.Syntax.Parse.readTypeParams(tables, Progress(Stigi.Syntax.Parse.nextToken(last), nodes, lists, pending)) match
      case ListFailed(fault, at) => Failed(fault, at)
      case Listed(typeParams, afterTypes) =>
        val Progress(colon, typeNodes, typeLists, typePending) = afterTypes
        if !(Stigi.Syntax.Parse.kindAt(tables, colon) is TokColon) then Failed(UnexpectedToken, colon) else
          Stigi.Syntax.Parse.readEnumCases(tables, Progress(Stigi.Syntax.Parse.nextToken(colon), typeNodes, typeLists, typePending)) match
            case ListFailed(fault, at) => Failed(fault, at)
            case Listed(cases, after) =>
              Stigi.Syntax.Parse.freeSlot(after) match
                case TableFull => Failed(UnitTooLarge, token)
                case FreeSlot(at) => Stigi.Syntax.Parse.place(tables, at, EnumDecl(at, isPublic, Stigi.Syntax.Parse.spanText(tables, token, last), typeParams, cases), after)
