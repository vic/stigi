// prompt: read one parameter: a name, a colon and a type.
def Stigi.Syntax.Parse.readParam {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Built =
  val Progress(token, nodes, lists, pending) = start
  if Stigi.Limit.maxUnitBytes <= token || !Stigi.Syntax.Parse.isWord(tables, token) || !(Stigi.Syntax.Parse.kindAt(tables, token + 1) is TokColon) then Failed(UnexpectedToken, token) else
    Stigi.Syntax.Parse.readType(tables, Progress(token + 2, nodes, lists, pending), Stigi.Limit.maxParseDepth) match
      case Failed(fault, at) => Failed(fault, at)
      case Built(declared, after) =>
        Stigi.Syntax.Parse.claimOver(after, declared, declared, declared) match
          case Unclaimed(fault) => Failed(fault, token)
          case Claimed(at, typed, _, _) => Stigi.Syntax.Parse.place(tables, at, Param(at, Stigi.Syntax.Parse.spanText(tables, token, token), typed), after)
