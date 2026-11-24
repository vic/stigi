// prompt: record the members of every entry of a list span, one entry at a time.
def Stigi.Names.recordSpan {Stigi.Mut} (members: Stigi.Names.Members, tables: Stigi.Syntax.Tables, owner: U64, span: Stigi.Syntax.Span) <below(owner, Stigi.Limit.maxUnits)>: Stigi.Names.Recorded =
  span match
    case Span(first, end) => Stigi.Names.recordEntries(members, tables, owner, first, end, end - first + 1)
