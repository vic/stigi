// prompt: place the evidence name before a colon that can lead a fact, or an absent node.
def Stigi.Syntax.Parse.factEvidence {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress) <below(0, Stigi.Limit.maxUnitBytes)>: Stigi.Syntax.Parse.Built =
  val Progress(token, nodes, lists, pending) = start
  if Stigi.Limit.maxUnitBytes <= token || !Stigi.Syntax.Parse.isWord(tables, token) || !(Stigi.Syntax.Parse.kindAt(tables, token + 1) is TokColon) then Stigi.Syntax.Parse.absent(tables, start) else
    Stigi.Syntax.Parse.freeSlot(start) match
      case TableFull => Failed(UnitTooLarge, token)
      case FreeSlot(at) => Stigi.Syntax.Parse.place(tables, at, Name(at, Stigi.Syntax.Parse.spanText(tables, token, token)), Progress(token + 2, nodes, lists, pending))
