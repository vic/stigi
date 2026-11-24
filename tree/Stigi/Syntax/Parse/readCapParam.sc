// prompt: read one capability parameter: an optional name and colon, and a type.
def Stigi.Syntax.Parse.readCapParam {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Built =
  val Progress(token, _, _, _) = start
  if Stigi.Limit.maxUnitBytes <= token then Failed(UnexpectedToken, token) else
    val labeled: Stigi.Syntax.Parse.Built = if Stigi.Syntax.Parse.isWord(tables, token) && Stigi.Syntax.Parse.kindAt(tables, token + 1) is TokColon then Stigi.Syntax.Parse.parseName(tables, start) else Stigi.Syntax.Parse.absent(tables, start)
    labeled match
      case Failed(fault, at) => Failed(fault, at)
      case Built(label, afterLabel) =>
        val Progress(typeToken, labelNodes, labelLists, labelPending) = afterLabel
        val typeStart: U64 = if Stigi.Syntax.Parse.kindAt(tables, typeToken) is TokColon then Stigi.Syntax.Parse.nextToken(typeToken) else typeToken
        Stigi.Syntax.Parse.readType(tables, Progress(typeStart, labelNodes, labelLists, labelPending), Stigi.Limit.maxParseDepth) match
          case Failed(fault, at) => Failed(fault, at)
          case Built(declared, after) =>
            Stigi.Syntax.Parse.claimOver(after, label, declared, declared) match
              case Unclaimed(fault) => Failed(fault, token)
              case Claimed(at, named, typed, _) => Stigi.Syntax.Parse.place(tables, at, CapParam(at, named, typed), after)
