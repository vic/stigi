// prompt: parse one alias line from the word alias: a dotted name, an equal sign, and the dotted name it stands for.
def Stigi.Syntax.Parse.parseAlias {Stigi.Mut} (tables: Stigi.Syntax.Tables, start: Stigi.Syntax.Parse.Progress) <below(0, Stigi.Limit.maxUnitNodes), below(0, Stigi.Limit.maxUnitBytes)>: Stigi.Syntax.Parse.Built =
  val Progress(token, nodes, lists, pending) = start
  if Stigi.Limit.maxUnitBytes <= token then Failed(UnexpectedToken, token) else
    Stigi.Syntax.Parse.parseName(tables, Progress(token + 1, nodes, lists, pending)) match
      case Failed(fault, at) => Failed(fault, at)
      case Built(name, named) =>
        val Progress(equalAt, namedNodes, namedLists, namedPending) = named
        if Stigi.Limit.maxUnitBytes <= equalAt || !(Stigi.Syntax.Parse.kindAt(tables, equalAt) is TokAssign) then Failed(UnexpectedToken, equalAt) else
          Stigi.Syntax.Parse.parseName(tables, Progress(equalAt + 1, namedNodes, namedLists, namedPending)) match
            case Failed(fault, at) => Failed(fault, at)
            case Built(target, after) =>
              Stigi.Syntax.Parse.claimOver(after, name, target, target) match
                case Unclaimed(fault) => Failed(fault, equalAt)
                case Claimed(at, claimedName, claimedTarget, _) => Stigi.Syntax.Parse.place(tables, at, AliasLine(at, claimedName, claimedTarget), after)
