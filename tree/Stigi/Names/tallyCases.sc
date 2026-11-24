// prompt: walk the chain of members that carry a text and tally their enum cases by namespace.
def Stigi.Names.tallyCases(image: Stigi.Names.Image, members: Stigi.Names.Members, tables: Stigi.Syntax.Tables, from: U64, first: U64, end: U64, step: Stigi.Names.MemberStep, tally: Stigi.Names.CaseTally, remaining: U64) <below(0, Stigi.Limit.maxUnitBytes), lessOrEqual(first, end), lessOrEqual(end, Stigi.Limit.maxUnitBytes), decreases(remaining)>: Stigi.Names.CaseTally =
  if remaining < 1 then tally else
    step match
      case MembersEnded => tally
      case MemberHit(member, nextSlot) => Stigi.Names.tallyCases(image, members, tables, from, first, end, Stigi.Names.nextMember(members, tables, first, end, nextSlot, Stigi.Names.tableSlots), Stigi.Names.countCase(image, members, from, member, tally), remaining - 1)
