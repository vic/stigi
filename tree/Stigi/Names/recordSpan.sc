// prompt: record the members of every entry of a list span, one entry at a time.
def Stigi.Names.recordSpan {Stigi.Mut} (members: Stigi.Names.Members, tables: Stigi.Syntax.Tables, owner: U64, span: Stigi.Syntax.Span) <below(0, Stigi.Limit.maxUnitNodes), below(0, Stigi.Limit.maxUnitBytes)>: Stigi.Names.Recorded =
  span match
    case Span(first, end) =>
      val Tables(_, _, _, lists, _, _) = tables
      var state: Stigi.Names.Recorded = Recorded(members)
      var index: U64 = first
      var steps: U64 = 0
      while steps < Stigi.Limit.maxUnitNodes && index < end && state is Recorded(_) do
        @<lessOrEqual(index, end)>
        val item: U64 = lists(index)
        state = state match
          case Recorded(current) =>
            if item < Stigi.Limit.maxUnitNodes then
              Stigi.Syntax.readSlot(tables, item) match
                case MalformedTable(_) => Recorded(current)
                case Fetched(entry) => Stigi.Names.recordEntry(current, tables, owner, entry)
            else Recorded(current)
          case MembersFull => MembersFull
        index = index + 1
        steps = steps + 1
      state
