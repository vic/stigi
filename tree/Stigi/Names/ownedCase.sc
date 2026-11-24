// prompt: find the member of an owner that carries a text, following the chain of members with that text.
def Stigi.Names.ownedCase(members: Stigi.Names.Members, tables: Stigi.Syntax.Tables, owner: U64, first: U64, end: U64, step: Stigi.Names.MemberStep, remaining: U64) <lessOrEqual(first, end), lessOrEqual(end, Stigi.Limit.maxUnitBytes), decreases(remaining)>: Stigi.Names.Resolution =
  if remaining < 1 then Unresolved else
    step match
      case MembersEnded => Unresolved
      case MemberHit(member, nextSlot) =>
        members match
          case Members(_, _, table, count, _) =>
            if member < count then
              table(member) match
                case Member(memberOwner, kind, _, _) =>
                  if memberOwner == owner && kind is CaseMember then ToCase(memberOwner, member) else Stigi.Names.ownedCase(members, tables, owner, first, end, Stigi.Names.nextMember(members, tables, first, end, nextSlot, Stigi.Names.tableSlots), remaining - 1)
            else Unresolved
