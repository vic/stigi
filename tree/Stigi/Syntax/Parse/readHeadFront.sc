// prompt: read the front of a def head: type parameters, the groups before the value parameters, and the value parameters.
def Stigi.Syntax.Parse.readHeadFront {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.HeadFront =
  Stigi.Syntax.Parse.readTypeParams(tables, start) match
    case ListFailed(fault, at) => HeadFrontFailed(fault, at)
    case Listed(typeParams, afterTypes) =>
      Stigi.Syntax.Parse.readGroups(tables, afterTypes) match
        case ListFailed(fault, at) => HeadFrontFailed(fault, at)
        case Listed(preGroups, afterGroups) =>
          val Progress(paren, _, _, _) = afterGroups
          Stigi.Syntax.Parse.readValueGroup(tables, afterGroups) match
            case ListFailed(fault, at) => HeadFrontFailed(fault, at)
            case Listed(params, afterParams) => HeadFront(typeParams, preGroups, Stigi.Syntax.Parse.kindAt(tables, paren) is TokLeftParen, params, afterParams)
