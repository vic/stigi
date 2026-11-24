// prompt: record the members of the list entries of a span from one entry on, one entry at a time, stopping at the first failure.
def Stigi.Names.recordEntries {Stigi.Mut} (members: Stigi.Names.Members, tables: Stigi.Syntax.Tables, owner: U64, first: U64, end: U64, remaining: U64) <below(owner, Stigi.Limit.maxUnits), lessOrEqual(end, Stigi.Limit.maxUnitNodes), decreases(remaining)>: Stigi.Names.Recorded =
  if remaining < 1 || end <= first then Recorded(members) else
    val Tables(_, _, _, lists, _, _) = tables
    val item: U64 = lists(first)
    if item < Stigi.Limit.maxUnitNodes then
      Stigi.Syntax.readSlot(tables, item) match
        case MalformedTable(_) => MembersMalformed
        case Fetched(entry) =>
          val recorded: Stigi.Names.Recorded = Stigi.Names.recordEntry(members, tables, owner, entry)
          recorded match
            case Recorded(grown) => Stigi.Names.recordEntries(grown, tables, owner, first + 1, end, remaining - 1)
            case MembersFull | MembersMalformed => recorded
    else MembersMalformed
