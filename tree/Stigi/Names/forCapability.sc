// prompt: find the capability name member of a handler entry by scanning the member table from one index; handler declarations are rare, so the scan costs at most the member count per handle statement.
def Stigi.Names.forCapability(members: Stigi.Names.Members, owner: U64, at: U64, remaining: U64) <decreases(remaining)>: Stigi.Names.ForRead =
  if remaining < 1 then MissingFor else
    members match
      case Members(_, _, table, count, _) =>
        if at < count then
          table(at) match
            case Member(memberOwner, kind, first, end) =>
              if memberOwner == owner && kind is ForCapabilityMember then ForText(first, end) else Stigi.Names.forCapability(members, owner, at + 1, remaining - 1)
        else MissingFor
