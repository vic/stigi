// prompt: read one enum case line from its name token: the name, optional fields in parentheses and an optional fact group.
def Stigi.Syntax.Parse.readEnumCase {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Built =
  val Progress(token, nodes, lists, pending) = start
  val last: U64 = Stigi.Syntax.Parse.nameLast(tables, token, Stigi.Limit.maxUnitBytes)
  val afterName: Stigi.Syntax.Parse.Progress = Progress(Stigi.Syntax.Parse.nextToken(last), nodes, lists, pending)
  val Progress(paren, _, _, _) = afterName
  if !Stigi.Syntax.Parse.isWord(tables, token) then Failed(UnexpectedToken, token) else
    Stigi.Syntax.Parse.readValueGroup(tables, afterName) match
      case ListFailed(fault, at) => Failed(fault, at)
      case Listed(fields, afterFields) =>
        val Progress(factToken, _, _, _) = afterFields
        val held: Stigi.Syntax.Parse.Built = if Stigi.Syntax.Parse.kindAt(tables, factToken) is TokLess then Stigi.Syntax.Parse.readFactGroup(tables, afterFields) else Stigi.Syntax.Parse.absent(tables, afterFields)
        held match
          case Failed(fault, at) => Failed(fault, at)
          case Built(facts, after) =>
            Stigi.Syntax.Parse.claimOver(after, facts, facts, facts) match
              case Unclaimed(fault) => Failed(fault, token)
              case Claimed(at, stated, _, _) => Stigi.Syntax.Parse.place(tables, at, EnumCase(at, Stigi.Syntax.Parse.spanText(tables, token, last), Stigi.Syntax.Parse.kindAt(tables, paren) is TokLeftParen, fields, stated), after)
