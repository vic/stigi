// prompt: step a member lookup from a slot to the next member whose text equals a text of the current unit.
def Stigi.Names.nextMember(members: Stigi.Names.Members, tables: Stigi.Syntax.Tables, first: U64, end: U64, slot: U64, remaining: U64) <below(slot, Stigi.Names.tableSlots), lessOrEqual(first, end), lessOrEqual(end, Stigi.Limit.maxUnitBytes), decreases(remaining)>: Stigi.Names.MemberStep =
  if remaining < 1 then MembersEnded else
    members match
      case Members(pool, _, table, count, slots) =>
        slots(slot) match
          case Vacant => MembersEnded
          case Keyed(key) =>
            if key < count then
              table(key) match
                case Member(_, _, memberFirst, memberEnd) =>
                  if memberEnd - memberFirst == end - first && Stigi.Names.poolHasText(pool, memberFirst, tables, first, end, end - first + 1) then MemberHit(key, (slot + 1) % Stigi.Names.tableSlots) else Stigi.Names.nextMember(members, tables, first, end, (slot + 1) % Stigi.Names.tableSlots, remaining - 1)
            else Stigi.Names.nextMember(members, tables, first, end, (slot + 1) % Stigi.Names.tableSlots, remaining - 1)
