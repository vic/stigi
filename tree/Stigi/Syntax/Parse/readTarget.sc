// prompt: read the target of a handler after for or of a given after a colon, or an absent node for a capability or a trait.
def Stigi.Syntax.Parse.readTarget {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress, kind: Stigi.Syntax.Parse.CompoundKind) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Built =
  val Progress(lead, nodes, lists, pending) = start
  val leadKind: Stigi.Lex.TokenKind = Stigi.Syntax.Parse.kindAt(tables, lead)
  val typeStart: Stigi.Syntax.Parse.Progress = Progress(Stigi.Syntax.Parse.nextToken(lead), nodes, lists, pending)
  kind match
    case IsHandler => if leadKind is TokFor then Stigi.Syntax.Parse.readType(tables, typeStart, Stigi.Limit.maxParseDepth) else Failed(UnexpectedToken, lead)
    case IsGiven => if leadKind is TokColon then Stigi.Syntax.Parse.readType(tables, typeStart, Stigi.Limit.maxParseDepth) else Failed(UnexpectedToken, lead)
    case IsCapability | IsTrait => Stigi.Syntax.Parse.absent(tables, start)
