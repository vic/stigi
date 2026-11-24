// prompt: whether an operation text is an operation member of a capability or trait unit, stepping through the members that share its text.
def Stigi.Names.hasOperation(members: Stigi.Names.Members, tables: Stigi.Syntax.Tables, owner: U64, first: U64, end: U64, step: Stigi.Names.MemberStep, remaining: U64) <lessOrEqual(first, end), lessOrEqual(end, Stigi.Limit.maxUnitBytes), decreases(remaining)>: Bool =
  if remaining < 1 then false else
    step match
      case MembersEnded => false
      case MemberHit(member, nextSlot) =>
        members match
          case Members(_, _, table, count, _) =>
            if member < count then
              table(member) match
                case Member(memberOwner, kind, _, _) =>
                  val operation: Bool = kind match
                    case OperationMember => true
                    case CaseMember | ForCapabilityMember => false
                  if memberOwner == owner && operation then true else Stigi.Names.hasOperation(members, tables, owner, first, end, Stigi.Names.nextMember(members, tables, first, end, nextSlot, Stigi.Names.tableSlots), remaining - 1)
            else false
