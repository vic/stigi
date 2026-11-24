// prompt: read an extern val from its name token: its name, a colon and its type.
def Stigi.Syntax.Parse.readExternVal {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress, isPublic: Bool, abi: Stigi.Syntax.Text) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Built =
  val Progress(token, nodes, lists, pending) = start
  val last: U64 = Stigi.Syntax.Parse.nameLast(tables, token, Stigi.Limit.maxUnitBytes)
  val colon: U64 = Stigi.Syntax.Parse.nextToken(last)
  if !Stigi.Syntax.Parse.isWord(tables, token) || !(Stigi.Syntax.Parse.kindAt(tables, colon) is TokColon) then Failed(UnexpectedToken, token) else
    Stigi.Syntax.Parse.readType(tables, Progress(Stigi.Syntax.Parse.nextToken(colon), nodes, lists, pending), Stigi.Limit.maxParseDepth) match
      case Failed(fault, at) => Failed(fault, at)
      case Built(declared, after) =>
        Stigi.Syntax.Parse.claimOver(after, declared, declared, declared) match
          case Unclaimed(fault) => Failed(fault, token)
          case Claimed(at, typed, _, _) => Stigi.Syntax.Parse.place(tables, at, ExternVal(at, isPublic, abi, Stigi.Syntax.Parse.spanText(tables, token, last), typed), after)
