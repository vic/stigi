// prompt: join a call with one argument and a parsed value into an element write.
def Stigi.Syntax.Parse.joinIndexWrite {Stigi.Mut} (tables: Stigi.Syntax.Tables, target: Stigi.Syntax.NodeAt, value: Stigi.Syntax.Parse.Built, token: U64) <below(0, Stigi.Limit.maxUnitNodes)>: Stigi.Syntax.Parse.Built =
  value match
    case Failed(fault, at) => Failed(fault, at)
    case Built(assigned, after) =>
      Stigi.Syntax.readSlot(tables, target) match
        case MalformedTable(slot) => Failed(ChildAfterParent, slot)
        case Fetched(found) =>
          Stigi.Syntax.Parse.partsOfCall(found) match
            case PlainNode => Failed(UnexpectedToken, token)
            case CallParts(callee, args) =>
              val Span(first, end) = args
              val Tables(_, _, _, lists, _, _) = tables
              if end <= first || 1 < end - first then Failed(UnexpectedToken, token) else
                val index: U64 = lists(first)
                if Stigi.Limit.maxUnitNodes <= index then Failed(ChildAfterParent, token) else
                  Stigi.Syntax.Parse.claimOver(after, callee, index, assigned) match
                    case Unclaimed(fault) => Failed(fault, token)
                    case Claimed(at, written, slot, stored) => Stigi.Syntax.Parse.place(tables, at, IndexWrite(at, written, slot, stored), after)
