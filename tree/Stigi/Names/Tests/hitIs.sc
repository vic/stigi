// prompt: whether a hit of a member lookup is a member of the given owner and kind.
def Stigi.Names.Tests.hitIs(members: Stigi.Names.Members, hit: U64, owner: U64, wantCase: Bool) <below(0, Stigi.Limit.maxUnitBytes)>: Bool =
  members match
    case Members(_, _, table, count, _) =>
      if hit < count then
        table(hit) match
          case Member(memberOwner, kind, _, _) => memberOwner == owner && kind is CaseMember == wantCase
      else false
